# ESP32-S3 FT8 Decoder

**Project:** ESP32-S3 FT8 Decoder  
**Current revision:** REV-D5  
**Platform:** ESP32-S3 N16R8  
**Display:** ILI9341 240×320 portrait TFT  
**Audio codec:** WM8731  
**FT8 decoder:** native `ft8_lib`-derived decoder integrated directly into the project

This project decodes live FT8 audio received from a WebSDR through a PC audio output and the WM8731 line input. The ESP32-S3 captures the audio, builds a complete 15-second FT8 slot, and performs the native FT8 candidate, LDPC, CRC and message-decoding process locally.

---

## 1. Project overview

```text
WebSDR
  │
  │ PC speaker / line-out
  ▼
WM8731 LLINEIN
  │
  │ ADC / ADCDAT
  ▼
ESP32-S3 I2S @ 12 kHz
  │
  ▼
15-second FT8 capture
120,000 mono samples
  │
  ▼
native FT8 monitor / waterfall
  │
  ▼
FT8 candidate detection
  │
  ▼
soft-decision decoding
  │
  ▼
LDPC
  │
  ▼
CRC validation
  │
  ▼
FT8 message decoding
  │
  ▼
ILI9341 TFT
```

Wi-Fi is used for NTP time synchronization and FT8 slot alignment. The audio itself is **not** streamed over Wi-Fi.

---

## 2. Main features

### FT8 decoding

- Native FT8 decoder path based on the supplied `ft8_lib` implementation.
- Complete 15-second FT8 slot processing.
- 12 kHz audio sample rate.
- 120,000 mono samples per FT8 slot.
- Native FT8 waterfall generation.
- Candidate detection.
- LDPC decoding.
- CRC validation.
- FT8 message extraction.
- Up to 80 candidate messages considered.
- Minimum candidate score: 10.
- LDPC iteration limit: 25.
- Native 2× time and 2× frequency oversampling.
- Decoder frequency and time reporting includes candidate sub-bin/sub-symbol information.

### Continuous acquisition / stability

- Dual PSRAM capture buffers allow audio acquisition to continue while the previous 15-second slot is being decoded.
- Large FFT/decoder memory is kept away from the small FreeRTOS task stack.
- Decoder and allocation failures are handled explicitly rather than allowing uncontrolled memory access.
- Wi-Fi failure does not force an ESP32 reboot; the decoder can continue in offline mode.
- Startup reset/heap/PSRAM diagnostics are available through Serial output.

### Startup

- WM8731 initialization and register ACK/debug information are shown during startup.
- Startup splash/debug display is held for approximately 4 seconds using `SPLASH_HOLD_MS`.
- Si5351 CLK2 supplies the 12.288 MHz WM8731 MCLK.

### TFT display

- Portrait 240×320 UI.
- Callsign and Maidenhead grid are configurable.
- Date/time display.
- Date format: `dd-mm-yyyy`.
- FT8 decoded messages displayed in columns:

```text
FREQ Hz  FT8 MESSAGE                 PASS:n
--------------------------------------------
 972     CQ JA1ABC PM95
2244     CQ OK2XYZ JN99
2122     DL1ABC G4XYZ IO91
```

- CQ messages are displayed in yellow.
- Other decoded messages are displayed in white.
- Number of displayed message rows is configurable.
- Decoded messages are collected during the decoder operation and rendered after decoding completes, keeping TFT activity out of the critical decode loop.
- Message display is refreshed for each FT8 pass.
- The `PASS:n` counter reports **all successfully decoded messages**, even when the display is filtered to CQ messages only.

---

## 3. Display configuration

The following parameters are in `pins.h`.

### Number of displayed messages

```cpp
#define FT8_DISPLAY_MESSAGE_LINES 17
```

Increase or decrease this value to suit the available TFT space.

### CQ-only display filter

```cpp
#define FT8_DISPLAY_CQ_ONLY 0
```

Values:

| Value | Behaviour |
|---:|---|
| `0` | Display all successfully decoded messages |
| `1` | Display only messages beginning with `CQ` |

**Important:** `PASS:n` always counts **all valid decoded messages**, regardless of this display filter.

---

## 4. Station configuration

Edit `pins.h`:

```cpp
#define MY_CALLSIGN "VU2UPX"
#define MY_GRID     "MK69"
```

Replace these with the station callsign and Maidenhead locator required for the installation.

---

## 5. Hardware

### ESP32-S3

- ESP32-S3-WROOM-1-N16R8
- OPI PSRAM required/enabled

### WM8731

Used as the receive audio ADC.

| WM8731 function | ESP32-S3 |
|---|---:|
| BCLK | GPIO 4 |
| LRCLK / LRC | GPIO 5 |
| DACDAT | GPIO 6 |
| ADCDAT | GPIO 7 |
| I2C SDA | GPIO 8 |
| I2C SCL | GPIO 9 |
| MCLK | External Si5351 CLK2 |

Audio input:

**PC/WebSDR line-out → WM8731 LLINEIN**

### Si5351

- I2C controlled.
- 25 MHz reference crystal.
- CLK2 generates **12.288 MHz MCLK** for the WM8731.
- CLK2 drive strength is configured for 4 mA.

### ILI9341 TFT

| TFT signal | ESP32-S3 |
|---|---:|
| SCLK | GPIO 12 |
| MOSI | GPIO 11 |
| MISO | GPIO 13 |
| CS | GPIO 10 |
| DC | GPIO 14 |
| RESET | GPIO 21 |
| Touch CS | GPIO 47 |
| Touch IRQ | GPIO 18 |

The TFT is configured as a **240×320 portrait display**.

### Other project pins

| Function | ESP32-S3 |
|---|---:|
| RX/TX relay input | GPIO 39 |
| PTT | GPIO 38 |
| Rotary A | GPIO 1 |
| Rotary B | GPIO 2 |
| Rotary switch | GPIO 3 |
| Key A | GPIO 40 |
| Key B | GPIO 41 |

The FT8 receive implementation primarily uses the WM8731 I2S receive path and TFT. Other project pins are retained in the common hardware configuration.

---

## 6. I2S audio configuration

The WM8731 receive stream is read by the ESP32-S3 as:

- Sample rate: **12,000 Hz**
- Sample format: **16-bit**
- Stereo I2S input from the codec
- FT8 processing uses the selected mono receive stream
- FT8 slot length: **15 seconds**
- Samples per slot: **120,000**

The WM8731 MCLK is supplied externally by the Si5351 at **12.288 MHz**.

---

## 7. FT8 slot timing

FT8 processing is synchronized to UTC 15-second boundaries:

```text
00 seconds
15 seconds
30 seconds
45 seconds
```

The decoder processes a complete 15-second capture window. NTP synchronization is used to maintain the UTC clock required for slot alignment.

---

## 8. Wi-Fi / NTP

Wi-Fi is required only for time synchronization and related setup functions.

If fixed credentials are not supplied, the firmware can use WiFiManager to configure the connection.

The decoder can continue in offline mode when Wi-Fi is unavailable.

The audio path does not depend on network streaming.

---

## 9. Required Arduino libraries

The project uses the following Arduino libraries/components:

### TFT_eSPI

Used for the ILI9341 TFT display.

The TFT pin configuration is provided through the normal TFT_eSPI `User_Setup` configuration.

### WiFiManager by tzapu

Used for Wi-Fi credential configuration through the ESP32 captive portal when credentials are not provided directly in `pins.h`.

### Si5351Arduino / Etherkit Si5351 library

Used to control the Si5351 frequency synthesizer and generate the WM8731 MCLK on CLK2.

### ESP32 Arduino core

The project uses the ESP32 Arduino environment and its native:

- WiFi support
- I2S driver
- FreeRTOS tasks
- PSRAM/heap capabilities
- ESP system diagnostics

### Wire / SPI / time

The following are supplied by the Arduino/ESP32 environment:

- `Wire.h` — I2C control for WM8731 and Si5351.
- `SPI.h` — TFT SPI interface.
- `time.h` — UTC/NTP time handling.

### FT8 decoder sources included in this repository

The project includes the FT8 decoding source required by the application, including:

- FT8 monitor
- FFT / KISS FFT
- candidate detection
- CRC
- LDPC
- message decoding
- FT8 constants and support code

These sources are stored in:

```text
src/common/
src/fft/
src/ft8/
```

They are part of this project package and are compiled with the application rather than being fetched at runtime.

> Library version numbers are intentionally not claimed here where the source package does not embed an authoritative version string. Use the versions installed and validated with the project in Arduino IDE.

---

## 10. Arduino IDE setup

Recommended project environment:

- Arduino IDE 2.x
- ESP32-S3 board package
- ESP32-S3 N16R8 / OPI PSRAM enabled
- OPI PSRAM enabled in the Arduino board options
- TFT_eSPI configured for the project's ILI9341 display and pins

Open:

```text
ESP32S3_FT8_Decoder.ino
```

and compile/upload the complete project directory.

---

## 11. WebSDR test setup

1. Connect PC/WebSDR audio output to WM8731 `LLINEIN` and common ground.
2. Open a WebSDR capable of receiving the required amateur band.
3. Tune to an FT8 frequency appropriate for the band being tested.
4. Set PC audio level high enough for a healthy signal but below clipping.
5. Power the ESP32-S3.
6. Allow Wi-Fi/NTP synchronization when available.
7. Wait for an FT8 15-second boundary.
8. Observe the TFT decode results.

The audio level should be healthy without clipping. The project has previously been validated with live WebSDR audio through the WM8731 receive chain.

---

## 12. Decode display behaviour

The display uses a fixed column arrangement:

```text
FREQ Hz  FT8 MESSAGE                 PASS:n
```

The frequency column is padded so both 3-digit and 4-digit frequencies remain aligned.

CQ detection is performed on the decoded message text rather than on the formatted display line. This prevents the leading-space formatting of 3-digit frequencies from affecting CQ detection.

Example:

```text
 972     CQ JA1ABC PM95
2244     CQ OK2XYZ JN99
```

Both are recognized as CQ messages.

---

## 13. Decoder/display separation

A deliberate design decision in the current revision is that decoded messages are **stored during FT8 processing and rendered to the TFT after the decoder completes**.

This prevents TFT drawing operations from unnecessarily interfering with the time-critical FT8 candidate/LDPC/CRC processing.

This separation should be preserved in future revisions unless there is a specific reason to change it.

---

## 14. Revision history

### REV-D1 — Stability / startup

- Removed large FFT temporary arrays from the FreeRTOS audio-task stack.
- Moved large decoder memory to heap/PSRAM as required.
- Added allocation checks.
- Reduced LDPC stack pressure.
- Added reset/heap/PSRAM diagnostics.
- Added 4-second startup splash hold.
- Removed automatic reboot on Wi-Fi failure.

### REV-D2 — Native decode path

- Complete 15-second / 120,000-sample capture.
- UTC 15-second slot alignment.
- Dual PSRAM capture buffers.
- Native FT8 monitor/candidate/LDPC/CRC path.
- Correct candidate frequency/time reporting.

### REV-D3 — Date display

- Date changed to `dd-mm-yyyy`.

### REV-D4 — Decode display columns

- Added decoded-message column headings.
- Added PASS decode counter.
- Added formatted frequency/message columns.

### REV-D4A — UI isolation

- Decoded messages are buffered during decoding.
- TFT rendering occurs after decoder completion.
- Decoder/audio/timing path unchanged.

### REV-D4B — CQ highlighting

- CQ messages displayed in yellow.
- Other messages displayed in white.

### REV-D4C — CQ detection / message rows

- CQ detection corrected for both 3-digit and 4-digit frequency formatting.
- Configurable display row count:

```cpp
#define FT8_DISPLAY_MESSAGE_LINES 17
```

### REV-D5 — CQ-only display filter

Added:

```cpp
#define FT8_DISPLAY_CQ_ONLY 0
```

- `0` = all decoded messages displayed.
- `1` = only CQ messages displayed.
- `PASS:n` always counts all valid decoded messages.

REV-D5 is the current validated FT8 decoder/display baseline.

---

## 15. Current validated baseline

The current REV-D5 implementation has been validated on the target ESP32-S3 hardware with live WebSDR audio through the WM8731 receive chain.

The native decoder successfully produces FT8 decodes. The current project stage is therefore considered a **working FT8 receive/decoder/display platform**.

The decoder/audio path should be treated as frozen while future project work is developed around it.

---

## 16. Repository contents

```text
ESP32S3_FT8_Decoder/
│
├── ESP32S3_FT8_Decoder.ino
├── pins.h
├── wm8731.h
├── ft8_engine.h
├── ft8_engine.cpp
│
└── src/
    ├── common/
    │   ├── common.h
    │   ├── monitor.c
    │   └── monitor.h
    │
    ├── fft/
    │   ├── _kiss_fft_guts.h
    │   ├── kiss_fft.c
    │   ├── kiss_fft.h
    │   ├── kiss_fftr.c
    │   └── kiss_fftr.h
    │
    └── ft8/
        ├── constants.c
        ├── constants.h
        ├── crc.c
        ├── crc.h
        ├── debug.h
        ├── decode.c
        ├── decode.h
        ├── encode.c
        ├── encode.h
        ├── ldpc.c
        ├── ldpc.h
        ├── message.c
        ├── message.h
        ├── text.c
        └── text.h
```

---

## 17. Notes for future development

Please preserve the following proven interfaces when extending the project:

- WM8731 receive configuration.
- 12 kHz I2S audio path.
- 15-second UTC slot acquisition.
- Dual PSRAM capture buffering.
- Native FT8 candidate → LDPC → CRC decode chain.
- Decoder/UI separation.
- `FT8_DISPLAY_MESSAGE_LINES` configuration.
- `FT8_DISPLAY_CQ_ONLY` configuration.

Changes to the proven decoder path should be introduced as a new revision and validated independently.

---

## 18. Credits / upstream basis

The FT8 decoder implementation is based on the supplied `ft8_lib`-derived source used during development of this project. The project retains the FT8 monitor, FFT, candidate detection, LDPC, CRC and message decoding components required by the application.

This repository package is intended to document and preserve the ESP32-S3 implementation and its validated hardware integration.
