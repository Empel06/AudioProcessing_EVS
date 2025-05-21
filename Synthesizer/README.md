# PYNQ-Z2 Audio Synthesizer

This project implements a simple **audio synthesizer** with support for **tone generation**, **echo**, and **reverb** effects using the **PYNQ-Z2 board** and **Vitis Classic 2024.1**.

It utilizes **audio I/O**, **GPIO buttons/switches**, **AXI GPIO-controlled LEDs**, and a **ScuTimer-based interrupt routine** to synthesize and process sound in real-time.

---

## Hardware/Software Requirements

### Hardware:

* **PYNQ-Z2 board**
* Audio peripherals (line-in/line-out)
* 4 GPIO pushbuttons
* 2 GPIO switches
* 4 LEDs (AXI GPIO)
* Rotary Encoder (3-bit output via AXI GPIO)
* Zynq-7000 SoC

### Software:

* **Vitis Classic 2024.1**
* Vitis BSP with proper AXI GPIO and I2S (audio codec) configuration
* Audio libraries (`audio.h` and I2C configuration files)
* GPIO en interrupt drivers (xgpio.h, xscutimer.h, xscugic.h)
---

## Features

* Tone generator (A4, C5, E5, G5) via buttons
* Real-time echo effect (switch 0)
* Real-time reverb effect (switch 1)
* LED visualization for active tones/effects
* UART terminal status reporting
* Timer ISR for sample-accurate audio processing
* Rotary encoder to change the volume (0–100%)

---

## User Controls

### Buttons (Tone Select):

| Button | Tone | Frequency |
| ------ | ---- | --------- |
| BTN0   | A4   | 440.00 Hz |
| BTN1   | C5   | 523.25 Hz |
| BTN2   | E5   | 659.25 Hz |
| BTN3   | G5   | 783.99 Hz |

### Switches (Effects):

| Switch | Effect |
| ------ | ------ |
| SW0    | Echo   |
| SW1    | Reverb |

### Rotary Encoder
* Turn right: increase volume
* Turn left: decrease volume

### LEDs (Visual Feedback):

| LED     | Function                         |
| ------- | -------------------------------- |
| LD0–LD3 | Tone selection (if no FX active) |
| LD0+LD1 | Echo ON                          |
| LD2+LD3 | Reverb ON                        |

---

## System Architecture

```
          +------------------+
          |  GPIO Buttons    |----> Tone Select
          +------------------+
                  |
          +------------------+
          |  GPIO Switches   |----> FX Control
          +------------------+
                  |
          +------------------+
          | Rotary Encoder   |----> Volume Control
          +------------------+
                  |
    +--------------------------+
    |      Timer Interrupt     |---+--------------------+
    +--------------------------+   |                    |
                                   ↓                    ↓
                            +------------+      +---------------+
                            | Tone Synth |<-----| Echo/Reverb   |
                            +------------+      +---------------+
                                   |
                            +------------+
                            | Audio Out  |
                            +------------+
```

---

## Code Breakdown With Examples

### 1. Tone Generation

```c
void GenerateTone(int32_t* outputBufferL, int32_t* outputBufferR, int bufferSize, float frequency, float* phase) {
    const float amplitude = 1000000.0f;
    const float increment = 2.0f * PI * frequency / SAMPLE_RATE;

    for (int i = 0; i < bufferSize; i++) {
        float sample = sinf(*phase);
        int32_t value = (int32_t)(sample * amplitude);
        outputBufferL[i] = outputBufferR[i] = value;

        *phase += increment;
        if (*phase >= 2.0f * PI) *phase -= 2.0f * PI;
    }
}
```

### 2. Echo Effect

```c
void EchoEffect(int32_t* inputL, int32_t* inputR, int32_t* outputL, int32_t* outputR) {
    *outputL = (*inputL + echoBufferL[delayIndex]) / 2;
    *outputR = (*inputR + echoBufferR[delayIndex]) / 2;
    echoBufferL[delayIndex] = *outputL;
    echoBufferR[delayIndex] = *outputR;
}
```

### 3. Reverb Effect

```c
void ReverbEffect(int32_t* inputL, int32_t* inputR, int32_t* outputL, int32_t* outputR) {
    *outputL = (*inputL * 3 + reverbBufferL[delayIndex]) / 4;
    *outputR = (*inputR * 3 + reverbBufferR[delayIndex]) / 4;
    reverbBufferL[delayIndex] = *outputL;
    reverbBufferR[delayIndex] = *outputR;
}
```

### 4. Timer Interrupt Service Routine (ISR)

```c
static void Timer_ISR(void *CallBackRef) {
    ...
    inputL = (int32_t)Xil_In32(I2S_DATA_RX_L_REG);
    inputR = (int32_t)Xil_In32(I2S_DATA_RX_R_REG);

    switch (buttonStatus) {
        case 1: GenerateTone(&outputL, &outputR, 1, 440.0f, &phase1); break;
        case 2: GenerateTone(&outputL, &outputR, 1, 523.25f, &phase2); break;
        ...
    }

    if (switchStatus & 0x1) EchoEffect(&outputL, &outputR, &outputL, &outputR);
    if (switchStatus & 0x2) ReverbEffect(&outputL, &outputR, &outputL, &outputR);

    Xil_Out32(I2S_DATA_TX_L_REG, (u32)outputL);
    Xil_Out32(I2S_DATA_TX_R_REG, (u32)outputR);
}
```

### 5. LED Visualization Logic

```c
void DisplayStatusOnLED(u32 buttonStatus, u32 switchStatus) {
    uint8_t ledValue = 0x00;

    if ((switchStatus & 0x3) == 0) {
        switch (buttonStatus) {
            case 1: ledValue |= 0x01; break;
            case 2: ledValue |= 0x02; break;
            ...
        }
    }

    if (switchStatus & 0x1) ledValue |= 0x03;
    if (switchStatus & 0x2) ledValue |= 0x0C;

    XGpio_DiscreteWrite(&GpioLed, 1, ledValue);
}
```

### 6. Status Logging (UART Print)

```c
void PrintStatusIfChanged(u32 buttonStatus, u32 switchStatus) {
    if (buttonStatus != prevButtonStatus || switchStatus != prevSwitchStatus) {
        xil_printf("Status: ");
        ...
        xil_printf("| Echo %s | Reverb %s
",
            (switchStatus & 0x1) ? "ON" : "OFF",
            (switchStatus & 0x2) ? "ON" : "OFF");

        prevButtonStatus = buttonStatus;
        prevSwitchStatus = switchStatus;
    }
}
```

### 7. Rotary Encoder Singleton
```c
Rotary_enc* Rotary_enc_instance() {
    static int initialized = 0;
    if (!initialized) {
        XGpio_Initialize(&rotary_singleton.rotary, XPAR_AXI_GPIO_0_DEVICE_ID);
        XGpio_SetDataDirection(&rotary_singleton.rotary, 1, 0xFFFFFFFF); // input
        rotary_singleton.PS = Rotary_enc_GetValue(&rotary_singleton);
        initialized = 1;
    }
    return &rotary_singleton;
}

float volumeFactor = (float)volume / 100.0f;
outputL = (int32_t)(outputL * volumeFactor);
outputR = (int32_t)(outputR * volumeFactor);


```

---

## How to Build and Run

### 1. Hardware Design

* Use Vivado to configure the block design with:

  * `AXI GPIO` for buttons, switches, LEDs
  * `I2S Audio Codec IP` or connections to Digilent Pmod I2S2
  * `Zynq Processing System` with appropriate interfaces

### 2. Software Build

* Open Vitis Classic 2024.1
* Create a new application targeting the exported hardware platform
* Import all required source files (`main.c`, `audio.h`, platform config)
* Build and run the application via UART
* `peripheral.h` (for rotary encoder)

### 3. Interact

* Press buttons to generate tones
* Flip switches to enable/disable echo and reverb
* Observe LED feedback and UART printout´
* Turn the rotary encoder to change the volume

---

## Technical Details

* **Sampling Rate**: 48 kHz
* **Timer Interrupt**: Configured to trigger once per audio sample
* **Delay Buffers**:

  * Echo: simple feedback buffer (0.1s max)
  * Reverb: mixing input with previous samples
* **Phase Handling**: Each tone has its own phase accumulator
* **Volume**: 0-100% via rotary (start on 50%)

---

## Cleanup

* Stop the timer with `XScuTimer_Stop()`
* Call `cleanup_platform()` before exiting

---

## File List

| File                                  | Description                    |
| ------------------------------------- | ------------------------------ |
| `main.c`                              | Main program logic             |
| `audio.h`                             | Audio config header (I2C, I2S) |
| `xparameters.h`                       | Auto-generated hardware params |
| `xgpio.h`, `xscutimer.h`, `xscugic.h` | Xilinx drivers                 |
| `peripheral.h`                        | Rotary encoder functions       |

---