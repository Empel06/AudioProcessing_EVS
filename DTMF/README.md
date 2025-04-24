# DTMF Sender/Receiver on Zynq Platform

## Overview

This project implements a complete **Dual-Tone Multi-Frequency (DTMF)** sender and receiver system on a **Xilinx Zynq**-based embedded platform. The application supports toggling between 'send' and 'receive' modes using a GPIO switch, accepts character input via UART, and uses I2S for audio input/output. FFT-based frequency detection is performed using the ARM CMSIS DSP library.

---

## Hardware Peripherals

| Peripheral        | Purpose                                         |
|-------------------|--------------------------------------------------|
| GPIO              | Toggle between send and receive modes           |
| UART              | Receive key input from terminal                 |
| I2S               | Audio input and output                          |
| Timer (SCU)       | Periodic interrupt for audio sample generation  |
| Interrupt Controller (GIC) | Handle timer interrupts               |

---

## Key Constants

| Constant                | Value     | Description                             |
|-------------------------|-----------|-----------------------------------------|
| `SAMPLE_RATE`           | 48000     | Audio sampling rate in Hz               |
| `FFT_SIZE`              | 1024      | Number of samples per FFT computation   |
| `MAX_AMPLITUDE`         | 0x7FFFFF  | Max amplitude for 24-bit audio signal   |

---

## DTMF Tone Definitions

Each key is defined as a combination of two distinct frequencies:

```c
typedef struct {
    char key;
    float f1;
    float f2;
} DTMF_Tone;
```

Example:
- Key `'5'` generates frequencies 770 Hz and 1336 Hz.

---

## Send Mode

In **send** mode:
- Characters are received over UART.
- Each valid key (0–9, *, #) triggers tone generation.
- Input `'x'` mutes the output.

### Tone Generation
The tone is composed of two sine waves:

```c
sample = 0.5 * (sin(2πf1t) + sin(2πf2t))
```

The sample is scaled to a 24-bit value and transmitted over I2S to both left and right audio channels.

---

## Receive Mode

In **receive** mode:
- A block of audio samples is captured from I2S.
- FFT is applied using CMSIS DSP (`arm_cfft_f32`).
- Two dominant frequencies (low and high) are extracted.
- These are matched against known DTMF frequency pairs.

### Frequency Detection

- Low range: 650–1050 Hz
- High range: 1100–1700 Hz

If the detected pair matches a valid key, the key is printed via `xil_printf`.

---

## Interrupt Routine: `Timer_ISR`

This function generates audio samples at each timer interrupt based on global `freq1` and `freq2`. The routine ensures phase continuity and outputs the scaled sample via I2S.

---

## Key Functions

| Function                  | Description                                       |
|---------------------------|---------------------------------------------------|
| `Init_UART()`             | Initializes UART peripheral                       |
| `DetectDTMFFrequency()`   | Collects samples, performs FFT, detects tones     |
| `matchDTMF()`             | Matches detected frequencies to a key             |
| `process_received_char()` | Parses UART character and sets tone frequencies   |
| `Timer_Intr_Setup()`      | Configures the interrupt controller and timer     |

---

## Mode Switching

The GPIO switch toggles the current mode. On mode change:
- Switching **to receive** mutes tone output.
- Switching **to send** enables UART-based tone triggering.

```c
isReceiveMode = (switchValue & 0x1) != 0;
```

---

## UART Command Summary

| Input | Action                          |
|--------|----------------------------------|
| `0–9`, `*`, `#` | Plays corresponding DTMF tone |
| `x`    | Mutes the tone                  |
| Others | Displays an invalid key message |

---

## Sample Output

```text
Input: 5 -> Frequencies: 770 Hz & 1336 Hz
Detected Key: 5 (error: 6 Hz)
Switched mode to: Receive
SHHHH
```

---

## Notes

- The **CMSIS DSP library** is critical for FFT and signal processing.
- Audio is configured using external functions: `AudioPllConfig()`, `LineinLineoutConfig()`, etc.
- Ensure proper BSP setup including support for `xgpio`, `xuartps`, `xscutimer`, and `xscugic`.

---
