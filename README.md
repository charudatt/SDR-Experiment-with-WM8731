# SDR Experiment with WM8731

Experimental hardware and software validation platform for the **ESP32-S3 + WM8731 audio codec**, developed as a foundation for Amateur Radio SDR work.

This repository is intentionally organised around **small, independently validated stages**. Each validation tool is kept frozen once the associated hardware function has been proven.

## Project purpose

The board is being developed to investigate and validate:

- ESP32-S3 digital audio processing
- WM8731 audio codec operation
- I2C codec configuration
- I2S ADC/DAC audio paths
- Si5351 frequency synthesis
- TFT/touch user-interface hardware
- Future SDR receiver/transceiver functions for Amateur Radio

The repository is a laboratory/experimental project. A validation tool is not automatically part of the final SDR application.

## Repository structure

```text
SDR-Experiment-with-WM8731/
├── README.md
├── docs/
│   └── README.md
├── schematic/
│   └── README.md
├── hardware/
│   └── README.md
└── tools/
    ├── README.md
    └── REV-S7/
        ├── README.md
        ├── config.h
        ├── SDR_WM8731_ESP32S3_STEP2A2_REV_S7.ino
        └── doc/
            └── REV-S7.md
```

## Validation philosophy

The project deliberately progresses through small tests rather than attempting to build the complete SDR in one step.

A validated stage should be treated as a **known-good reference**. If a new experiment changes behaviour, create a new revision rather than modifying the frozen validation stage.

For example:

- REV-S7 = frozen and validated
- REV-S8 = future experiment based on REV-S7

## Current frozen validation

### REV-S7 — WM8731 audio loopback

REV-S7 validates:

```text
WM8731 MIC IN
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

It also retains the validated:

- WM8731 I2C register programming
- Si5351 initialisation
- 14.000 MHz Si5351 CLK0 output
- 12.288 MHz Si5351 CLK2 MCLK
- TFT diagnostic display
- TFT_eSPI touch operation
- 1 kHz DAC tone generator

REV-S7 has been **physically tested and confirmed working**.

## Important REV-S7 codec detail

The normal codec configuration uses WM8731 register R4 = `0x012`.

For microphone loopback, REV-S7 changes R4 to `0x014`, selecting the microphone ADC input and removing the microphone mute while retaining the DAC path.

This detail is documented in `tools/REV-S7/doc/REV-S7.md`.

## Hardware

The main board combines an ESP32-S3 module, WM8731 codec, Si5351 synthesizer, ILI9341 TFT/touch interface and supporting Amateur Radio test circuitry.

Hardware photographs and the schematic will be maintained separately from the validation source code.

## Software

Development is based on the Arduino IDE and the ESP32 Arduino core.

Libraries used by REV-S7 are documented in the individual tool documentation.

## Status

**REV-S7: FROZEN / VALIDATED**

Future development should not modify REV-S7. New functionality should be introduced as a new validation revision and documented independently.

## Author / project notes

This repository records the development and validation journey of an Amateur Radio SDR hardware platform. The emphasis is on reproducible experiments, clear hardware documentation and preserving known-good test stages.
