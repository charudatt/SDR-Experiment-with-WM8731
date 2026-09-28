# REV-S7 — WM8731 Audio Loopback Validation

**Status: FROZEN / VALIDATED**

REV-S7 is a hardware validation tool for the SDR Experiment with WM8731 platform.

## Purpose

The primary test is a digital audio loopback:

```text
MIC IN
  ↓
WM8731 ADC
  ↓
ESP32-S3 I2S RX
  ↓
ESP32-S3 I2S TX
  ↓
WM8731 DAC
  ↓
AUDIO OUT
```

The test confirms that audio can travel from the WM8731 microphone input through the ESP32-S3 digital audio interface and return to the WM8731 DAC output.

## Additional validated functions retained from REV-S4

- TFT display
- TFT_eSPI touch input
- Four-button test menu
- WM8731 I2C detection and register programming
- Si5351 initialisation
- Si5351 CLK0 = 14.000 MHz
- Si5351 CLK2 = 12.288 MHz MCLK
- 1 kHz DAC tone generator

## Buttons

| Button | Function |
|---|---|
| `1 kHz` | Toggle the proven 1 kHz DAC tone ON/OFF |
| `LOOP` | Toggle MIC → ADC → I2S → DAC loopback ON/OFF |
| `ADC TEST` | Reserved / visual only |
| `CODEC` | Reserved / visual only |

Only one button is active at a time.

## WM8731 loopback register

Normal validated state:

```text
R4 = 0x012
```

Loopback state:

```text
R4 = 0x014
```

The loopback value selects the microphone input for the ADC and removes the microphone mute while retaining the DAC selection.

When LOOP is switched OFF, R4 is restored to `0x012`.

## I2S configuration

| Signal | ESP32-S3 GPIO |
|---|---:|
| BCLK | GPIO4 |
| LRCLK / DACLRC | GPIO5 |
| DACDAT | GPIO6 |
| ADCDAT | GPIO7 |

Audio format:

- 48 kHz
- 16-bit
- Stereo
- Standard I2S
- Arduino-ESP32 `ESP_I2S.h` / `I2SClass`

## I2C

| Device | Address |
|---|---:|
| WM8731 | `0x1A` |
| Si5351 | `0x60` |

I2C pins:

- SDA = GPIO8
- SCL = GPIO9
- I2C speed = 100 kHz

## Si5351

- Crystal = 25 MHz
- CLK0 = 14.000 MHz
- CLK2 = 12.288 MHz
- CLK1 disabled
- CLK0 drive = 8 mA
- CLK2 drive = 2 mA
- Current validated correction = `+98700`

## Touch

REV-S7 retains the validated TFT_eSPI raw-touch method and the calibrated landscape mapping used by the project.

Touch CS = GPIO47.

## Important freeze rule

**Do not modify this source after validation.**

If the loopback implementation, UI, codec configuration or I2S behaviour needs to change, create a new validation revision, for example `REV-S8`.
