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


## FT8 Decoder Milestone — REV-D5

The project has now reached a major validated FT8 receive milestone using the ESP32-S3 and WM8731 audio path.

### REV-D5 — Native FT8 decoder

REV-D5 is the validated live FT8 decoder stage built on the proven WM8731 audio chain.

The validated processing path is:

```text
WebSDR / PC audio
        ↓
WM8731 LINE IN
        ↓
WM8731 ADC
        ↓
ESP32-S3 I2S RX
        ↓
12 kHz audio acquisition
        ↓
15-second FT8 capture
        ↓
PSRAM capture buffering
        ↓
Native embedded FT8 decoder
        ↓
Candidate detection
        ↓
LDPC / CRC validation
        ↓
Decoded FT8 messages
        ↓
ILI9341 TFT display
```

REV-D5 uses the supplied embedded FT8 decoder library rather than a separate custom FT8 decoding implementation. The decoder operates on the complete FT8 15-second receive window and is integrated with the existing ESP32-S3 + WM8731 platform.

### REV-D5 display features

The validated TFT interface provides:

- decoded FT8 messages in a compact scrolling/list display
- decoded frequency information
- total decoded-message PASS count
- CQ messages highlighted in yellow
- configurable number of displayed message lines
- optional CQ-only display filtering

The CQ display filter is controlled in the project header:

```cpp
#define FT8_DISPLAY_CQ_ONLY 0
```

```text
0 = display all successfully decoded messages
1 = display only CQ messages
```

The `PASS:n` counter always represents the **total number of successfully decoded messages**, including valid non-CQ messages that are hidden when CQ-only display is enabled.

### FT8 validation status

REV-D5 has been **live tested and confirmed working** with the WM8731 receive path and WebSDR audio input.

The FT8 decoder milestone demonstrates that the existing hardware platform can be used as a practical digital Amateur Radio receive platform without disturbing the previously validated WM8731 audio chain.

The earlier REV-S7 through REV-S13 validation stages remain frozen references. REV-D5 is a separate application-level milestone built on those proven hardware and audio stages.

### FT8 decoder tools

The validated decoder implementation is also packaged under the repository `tools/` area as:

```text
tools/
└── ESP32S3_FT8_Decoder/
    ├── ESP32S3_FT8_Decoder.ino
    ├── pins.h
    ├── wm8731.h
    ├── ft8_engine.h
    ├── ft8_engine.cpp
    ├── README.md
    └── src/
        ├── common/
        ├── fft/
        └── ft8/
```

This package is intended to provide a reproducible starting point for further FT8 experiments on the ESP32-S3 platform.

### Development rule

The successful REV-D5 implementation should be treated as a **frozen validated milestone**.

Future FT8 experiments should create a new revision rather than modifying the validated REV-D5 implementation in place.

## Hardware

The main board combines an ESP32-S3 module, WM8731 codec, Si5351 synthesizer, ILI9341 TFT/touch interface and supporting Amateur Radio test circuitry.

Hardware photographs and the schematic will be maintained separately from the validation source code.

## Software

Development is based on the Arduino IDE and the ESP32 Arduino core.

Libraries used by REV-S7 are documented in the individual tool documentation.

## Status

**REV-S7: FROZEN / VALIDATED**  
**REV-D5: FROZEN / VALIDATED FT8 DECODER MILESTONE**

Future development should not modify REV-S7. New functionality should be introduced as a new validation revision and documented independently.

## Author / project notes

This repository records the development and validation journey of an Amateur Radio SDR hardware platform. The emphasis is on reproducible experiments, clear hardware documentation and preserving known-good test stages.
