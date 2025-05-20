# Audio Synthesizer - Playback Sequence Guide

This document explains the structured playback sequence of tones and audio effects in the PYNQ-Z2 Audio Synthesizer project.

---

## Audio Playback Sequence

The audio synthesizer plays tones in the following structured sequence. This allows users (and listeners) to clearly identify the effect of **Echo** and **Reverb**:

### 1. Base Tones (No Effects)

These are pure sine wave tones, each corresponding to a button:

| Button | Tone | Frequency |
|--------|------|-----------|
| BTN0   | A4   | 440.00 Hz |
| BTN1   | C5   | 523.25 Hz |
| BTN2   | E5   | 659.25 Hz |
| BTN3   | G5   | 783.99 Hz |

### 2. Echo Tones (Switch 0 ON)

Each of the four tones above is now replayed with the **Echo** effect enabled by setting **Switch 0** (SW0) to ON.

This simulates a delay where each tone reflects shortly after playing, creating a "bouncing" audio effect.

### 3. Reverb Tones (Switch 1 ON)

All four tones are played again, but now only with the **Reverb** effect enabled via **Switch 1** (SW1 ON).

This makes the tone sound "roomy" or "ambient", emulating space and resonance.

### 4. Echo + Reverb (SW0 and SW1 ON)

Finally, all tones are played with **both** effects active simultaneously. This results in a more immersive and layered sound, combining the delay from Echo with the spatial enhancement of Reverb.

---

## Playback Order Summary

```
1. A4 (dry)
2. C5 (dry)
3. E5 (dry)
4. G5 (dry)

5. A4 + Echo
6. C5 + Echo
7. E5 + Echo
8. G5 + Echo

9. A4 + Reverb
10. C5 + Reverb
11. E5 + Reverb
12. G5 + Reverb

13. A4 + Echo + Reverb
14. C5 + Echo + Reverb
15. E5 + Echo + Reverb
16. G5 + Echo + Reverb
```

---

## How to Reproduce in Real-Time

To manually cycle through this sequence on the PYNQ-Z2:

1. **Turn off all switches (SW0, SW1 = 0)**  
   → Press BTN0–BTN3 to play tones without effects.

2. **Turn on Echo (SW0 = 1, SW1 = 0)**  
   → Repeat tones to hear Echo added.

3. **Turn on Reverb only (SW0 = 0, SW1 = 1)**  
   → Play tones again to hear Reverb effect.

4. **Enable both Echo + Reverb (SW0 = 1, SW1 = 1)**  
   → Press each button once more to hear combined effect.

---
