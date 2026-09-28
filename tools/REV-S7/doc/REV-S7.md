# REV-S7 Technical Validation Record

## Validation status

**FROZEN / VALIDATED ON HARDWARE**

REV-S7 is a reference validation stage for the SDR Experiment with WM8731 project.

## Objective

Validate a complete digital audio path from the WM8731 microphone input through the ESP32-S3 I2S interface and back to the WM8731 DAC output.

## Signal path

```text
Audio / MIC input
        ↓
     WM8731 ADC
        ↓
      ADCDAT
        ↓
   ESP32-S3 GPIO7
        ↓
       I2S RX
        ↓
   ESP32-S3 I2S TX
        ↓
   ESP32-S3 GPIO6
        ↓
      DACDAT
        ↓
     WM8731 DAC
        ↓
   Audio output
```

## I2S clock/data pins

| Function | GPIO |
|---|---:|
| BCLK | 4 |
| LRCLK / DACLRC | 5 |
| DACDAT | 6 |
| ADCDAT | 7 |

The WM8731 DACLRC and ADCLRC are tied together on the validated hardware and use GPIO5.

## Audio format

- Sample rate: 48,000 Hz
- Word length: 16 bit
- Channels: stereo
- Interface: I2S
- ESP32 API: `ESP_I2S.h` / `I2SClass`

## WM8731 configuration

The startup diagnostic programs the validated register sequence. During normal operation R4 is `0x012`.

When LOOP is enabled:

```text
R4 = 0x014
```

This enables the microphone input path required for the test. When LOOP is disabled, R4 returns to `0x012`.

## Other validated hardware

### Si5351

- I2C address: `0x60`
- Crystal: 25 MHz
- CLK0: 14.000 MHz
- CLK2: 12.288 MHz MCLK
- CLK1: disabled

### WM8731

- I2C address: `0x1A`
- MCLK: 12.288 MHz
- I2C: GPIO8/GPIO9

### TFT

- ILI9341
- 320 × 240
- TFT_eSPI
- Touch CS: GPIO47
- Landscape test menu

## Validation result

REV-S7 was tested on the actual hardware and the microphone/audio loopback was confirmed operational.

The 1 kHz generator was also retained from the previously validated REV-S4 stage.

## Reproducibility

Use the supplied `config.h` and source without modification when reproducing the REV-S7 test.

Required library/API notes:

- Arduino IDE
- ESP32 Arduino core with `ESP_I2S.h`
- TFT_eSPI
- Etherkit Si5351 library v2.2.0

## Freeze policy

This document and the associated source represent a completed validation stage. Future development must create a new revision instead of changing REV-S7.
