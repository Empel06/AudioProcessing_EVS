#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xscutimer.h"
#include "xscugic.h"
#include "xuartps.h"
#include "audio.h"
#include "arm_math.h"
#include "math.h"
#include <stdbool.h>
#include "xgpio.h"

XGpio GpioSwitches;

int switchValue;
bool isReceiveMode;

#define TIMER_DEVICE_ID     XPAR_XSCUTIMER_0_DEVICE_ID
#define INTC_DEVICE_ID      XPAR_SCUGIC_SINGLE_DEVICE_ID
#define TIMER_IRPT_INTR     XPAR_SCUTIMER_INTR
#define SAMPLE_RATE         48000
#define PI                  3.14159265358979f
#define MAX_AMPLITUDE       0x7FFFFF
#define UINT32_MAX_AS_FLOAT 4294967295.0f

#define FFT_SIZE 1024
#define SAMPLE_BLOCK_SIZE (FFT_SIZE * 2)

#define UART_DEVICE_ID      XPAR_XUARTPS_0_DEVICE_ID

float32_t sampleBuffer[SAMPLE_BLOCK_SIZE];

XUartPs Uart_Ps;

typedef struct {
    char key;
    float f1;
    float f2;
} DTMF_Tone;

DTMF_Tone dtmf_tones[] = {
    {'1', 697.0f, 1209.0f}, {'2', 697.0f, 1336.0f}, {'3', 697.0f, 1477.0f},
    {'4', 770.0f, 1209.0f}, {'5', 770.0f, 1336.0f}, {'6', 770.0f, 1477.0f},
    {'7', 852.0f, 1209.0f}, {'8', 852.0f, 1336.0f}, {'9', 852.0f, 1477.0f},
    {'*', 941.0f, 1209.0f}, {'0', 941.0f, 1336.0f}, {'#', 941.0f, 1477.0f}
};

// Globale toonparameters
float freq1 = 0.0f;
float freq2 = 0.0f;
float phase1 = 0.0f;
float phase2 = 0.0f;

static void Timer_ISR(void *CallBackRef) {
    XScuTimer *timerInstancePtr = (XScuTimer *)CallBackRef;
    XScuTimer_ClearInterruptStatus(timerInstancePtr);

    float step1 = 2.0f * PI * freq1 / SAMPLE_RATE;
    float step2 = 2.0f * PI * freq2 / SAMPLE_RATE;

    phase1 += step1;
    phase2 += step2;

    if (phase1 > 2.0f * PI) phase1 -= 2.0f * PI;
    if (phase2 > 2.0f * PI) phase2 -= 2.0f * PI;

    float sample = 0.5f * (arm_sin_f32(phase1) + arm_sin_f32(phase2));
    uint32_t scaled_sample = (uint32_t)(((sample + 1.0f) / 2.0f) * MAX_AMPLITUDE);

    Xil_Out32(I2S_DATA_TX_L_REG, scaled_sample);
    Xil_Out32(I2S_DATA_TX_R_REG, scaled_sample);
}

static int Timer_Intr_Setup(XScuGic * IntcInstancePtr, XScuTimer *TimerInstancePtr, u16 TimerIntrId) {
    int Status;
    XScuGic_Config *IntcConfig;
    IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
    Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig, IntcConfig->CpuBaseAddress);

    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, IntcInstancePtr);
    Status = XScuGic_Connect(IntcInstancePtr, TimerIntrId, (Xil_ExceptionHandler)Timer_ISR, (void *)TimerInstancePtr);
    XScuGic_Enable(IntcInstancePtr, TimerIntrId);

    XScuTimer_EnableInterrupt(TimerInstancePtr);
    Xil_ExceptionEnable();
    return XST_SUCCESS;
}

// UART setup
static void Init_UART() {
    XUartPs_Config *Config = XUartPs_LookupConfig(UART_DEVICE_ID);
    XUartPs_CfgInitialize(&Uart_Ps, Config, Config->BaseAddress);
    XUartPs_SetBaudRate(&Uart_Ps, 115200);
}

char lastDetectedKey = '?';

char matchDTMF(float f1, float f2, float *errorOut)
{
    char bestKey = '?';
    float minError = 1e6;

    for (int i = 0; i < sizeof(dtmf_tones)/sizeof(DTMF_Tone); i++) {
        float df1 = fabsf(f1 - dtmf_tones[i].f1) + fabsf(f2 - dtmf_tones[i].f2);
        float df2 = fabsf(f2 - dtmf_tones[i].f1) + fabsf(f1 - dtmf_tones[i].f2);
        float err = (df1 < df2) ? df1 : df2;

        if (err < minError) {
            minError = err;
            bestKey = dtmf_tones[i].key;
        }
    }

    if (errorOut) *errorOut = minError;
    return bestKey;
}

void DetectDTMFFrequency()
{
    arm_status status;
    float32_t mag[FFT_SIZE];

    for (int i = 0; i < FFT_SIZE; i++) {
        int32_t raw = (int32_t)(Xil_In32(I2S_DATA_RX_L_REG) << 8) >> 8;
        float32_t sample = (float32_t)raw / 8388608.0f;
        sampleBuffer[2*i] = sample;
        sampleBuffer[2*i+1] = 0.0f;
        usleep(20);
    }

    arm_cfft_instance_f32 fft_inst;
    status = arm_cfft_init_f32(&fft_inst, FFT_SIZE);
    uint8_t ifftFlag = 0;
    uint8_t doBitReverse = 1;
    arm_cfft_f32(&fft_inst, sampleBuffer, ifftFlag, doBitReverse);

    arm_cmplx_mag_f32(sampleBuffer, mag, FFT_SIZE);

    float maxLow = 0, maxHigh = 0;
    int lowIdx = -1, highIdx = -1;

    for (int i = 1; i < FFT_SIZE / 2; i++) {
        float freq = (float)i * SAMPLE_RATE / FFT_SIZE;
        if (freq >= 650 && freq <= 1050) {
            if (mag[i] > maxLow) {
                maxLow = mag[i];
                lowIdx = i;
            }
        } else if (freq >= 1100 && freq <= 1700) {
            if (mag[i] > maxHigh) {
                maxHigh = mag[i];
                highIdx = i;
            }
        }
    }

    static bool lastWasSilence = false;

    if (lowIdx < 0 || highIdx < 0 || maxLow < 0.02f || maxHigh < 0.02f || (maxHigh / maxLow) < 0.3f) {
        if (!lastWasSilence) {
            xil_printf("SHHHH\r\n");
            lastWasSilence = true;
        }
        return;
    } else {
        lastWasSilence = false;
    }

    float freq1 = lowIdx * SAMPLE_RATE / FFT_SIZE;
    float freq2 = highIdx * SAMPLE_RATE / FFT_SIZE;

    float error;
    char key = matchDTMF(freq1, freq2, &error);
    if (error < 45) {
        if (key != lastDetectedKey) {
            xil_printf("Detected Key: %c (error: %d Hz)\r\n", key, (int)error);
            lastDetectedKey = key;
        }
    }

    usleep(200000);
    return;
}

void process_received_char(char c) {
    if (c == '\n' || c == '\r') return;

    if (c == 'x') {
        freq1 = 0.0f;
        freq2 = 0.0f;
        xil_printf("Toon gedempt.\n\r");
        return;
    }

    for (int i = 0; i < sizeof(dtmf_tones)/sizeof(DTMF_Tone); i++) {
        if (dtmf_tones[i].key == c) {
            freq1 = dtmf_tones[i].f1;
            freq2 = dtmf_tones[i].f2;
            xil_printf("Invoer: %c -> Frequencies: %d Hz & %d Hz\n\r", c, (int)freq1, (int)freq2);
            return;
        }
    }

    xil_printf("Ongeldige toets: %c\n\r", c);
}

int main() {
    int Status;
    init_platform();

    XGpio_Initialize(&GpioSwitches, XPAR_AXI_GPIO_1_DEVICE_ID);
    XGpio_SetDataDirection(&GpioSwitches, 2, 0xF); // Poort 2: switches als input

    IicConfig(XPAR_XIICPS_0_DEVICE_ID);
    AudioPllConfig();
    AudioConfigureJacks();
    LineinLineoutConfig();

    xil_printf("DTMF Sender/Receiver DEMO actief. Schakel tussen modi met GPIO-switch.\n\r");

    XScuTimer Scu_Timer;
    XScuTimer_Config *Scu_ConfigPtr;
    XScuGic IntcInstance;

    Scu_ConfigPtr = XScuTimer_LookupConfig(XPAR_PS7_SCUTIMER_0_DEVICE_ID);
    Status = XScuTimer_CfgInitialize(&Scu_Timer, Scu_ConfigPtr, Scu_ConfigPtr->BaseAddr);
    Status = Timer_Intr_Setup(&IntcInstance, &Scu_Timer, XPS_SCU_TMR_INT_ID);

    XScuTimer_LoadTimer(&Scu_Timer, (XPAR_PS7_CORTEXA9_0_CPU_CLK_FREQ_HZ / 2) / SAMPLE_RATE);
    XScuTimer_EnableAutoReload(&Scu_Timer);
    XScuTimer_Start(&Scu_Timer);

    Init_UART();

    bool lastMode = false; // false = Send, true = Receive
    while (1) {
        // Lees huidige switchstatus
        switchValue = XGpio_DiscreteRead(&GpioSwitches, 2);
        isReceiveMode = (switchValue & 0x1) != 0;

        // Toon mode alleen bij verandering
        if (isReceiveMode != lastMode) {
            xil_printf("Modus gewisseld naar: %s\n\r", isReceiveMode ? "Receive" : "Send");
            if (isReceiveMode) {
                freq1 = 0.0f;
                freq2 = 0.0f; // Dempt output bij schakeling naar receive
            }
            lastMode = isReceiveMode;
        }

        if (isReceiveMode) {
            DetectDTMFFrequency();
        } else {
            u8 RecvChar;
            if (XUartPs_Recv(&Uart_Ps, &RecvChar, 1) == 1) {
                process_received_char((char)RecvChar);
            }
        }
    }

    cleanup_platform();
    return 0;
}
