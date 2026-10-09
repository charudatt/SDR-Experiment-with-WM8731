# ESP32-S3 Si4732 FM Receiver — Web + Rotary Control (Stage 1B)

A standalone validation project for controlling a Si4732 FM receiver from an ESP32-S3 using both a physical rotary encoder and a lightweight browser interface.

**Status: Stage 1B hardware validation completed by the project owner.** The owner confirmed Si4732 detection and initialization, FM tuning/readback, Wi-Fi connection, HTTP page access, encoder tuning, web frequency entry/SET, and STEP + / STEP − controls. The receiver was tested without an antenna, so RF sensitivity and reception quality have not yet been validated.

This repository is a standalone test branch for the ongoing **SDR Experiment with WM8731** work. It is not merged into the FT8 decoder host project.

## Features

- Si4732 detection and initialization using the PU2CLR SI4735 library
- FM frequency tuning and readback
- Rotary encoder tuning
- ESP32 web server with frequency entry and SET
- STEP + / STEP − controls (100 kHz per step in this stage)
- Live status polling without reloading the entire page
- RSSI, SNR, and channel-valid status display
- Wi-Fi connection through WiFiManager; setup AP fallback
- No display required: no OLED or TFT dependency

## Hardware and pin map

| Signal | ESP32-S3 GPIO | Notes |
|---|---:|---|
| I²C SDA | GPIO 8 | Shared I²C bus |
| I²C SCL | GPIO 9 | Shared I²C bus |
| Si4732 RST | GPIO 19 | Receiver reset |
| Encoder A | GPIO 1 | Rotary encoder channel A |
| Encoder B | GPIO 2 | Rotary encoder channel B |
| Encoder switch | GPIO 3 | Push switch |

The tested module responded at I²C address `0x11`.

See [`HARDWARE_CONNECTIONS.txt`](HARDWARE_CONNECTIONS.txt) for the compact wiring list.

## Required software and libraries

- Arduino IDE 2.3.10 (tested setup)
- ESP32 Arduino board support compatible with the ESP32-S3 target
- PU2CLR SI4735 library (`SI4735.h`)
- ESPRotary
- WiFiManager
- ESP32 core libraries: `WiFi.h`, `WebServer.h`; Arduino `Wire`

Install the external libraries through the Arduino IDE Library Manager or their official distribution channels. Use the same ESP32-S3 board and memory settings as the tested hardware setup.

## Build and upload

1. Download this repository ZIP or clone the repository.
2. Open `REV-SI4732-STAGE1B.ino` in Arduino IDE.
3. Select the ESP32-S3 board matching your module and select the appropriate flash/PSRAM options.
4. Compile and upload.
5. Open Serial Monitor at **115200 baud**.
6. Wait for the Wi-Fi status and IP address in the log.
7. Open `http://<ESP32-IP-address>/` in a browser on the same local network.

If saved Wi-Fi credentials are available, WiFiManager attempts to connect to that network. If setup is required, the setup AP is `Si4732-Radio-Setup`. If the setup portal times out, the sketch uses the fallback AP `Si4732-Radio`. Follow the serial output for the active connection details.

## Browser controls

- Enter an FM frequency and press **SET** (Enter submits the field).
- Use **STEP +** and **STEP −** to change frequency in 100 kHz increments.
- Turn the physical encoder to tune; the browser readback updates through periodic status polling.
- The page reports RSSI, SNR, and channel-valid status.

Encoder short/long press remains diagnostic only in this stage. AM/SSB, BFO, bandwidth, volume, mute, memory storage, and persistent settings are outside the Stage 1B scope.

## Validation record

The project owner reported the following successful checks on the target hardware:

- Si4732 detected at `0x11`
- Radio initialization passed
- Initial tune and frequency readback passed at 103.90 MHz
- Rotary encoder advanced the frequency in 100 kHz increments
- Wi-Fi connected and HTTP server started
- Browser control page opened successfully
- Web frequency entry/SET worked
- STEP + and STEP − worked
- Browser frequency display followed physical encoder changes

**Not validated yet:** RF sensitivity, reception quality, RSSI/SNR accuracy with a real antenna, and audio output. The tests were performed without an antenna connected.

## Safety and network note

This is a simple local-network test interface and does not implement authentication. Use it only on a trusted local network or its setup access point. Do not port-forward it or expose it directly to the public internet.

## Scope boundaries

This repository does **not** include or modify the WM8731 audio path, Si5351, TFT/OLED UI, or FT8 decoder. Keep the validated host project files separate. Future audio integration should be a new, independently validated stage.

## Project structure

- `REV-SI4732-STAGE1B.ino` — Arduino entry point
- `Si4732_Config.h` — pins and receiver configuration
- `Si4732_Controller.h/.cpp` — Si4732 initialization, tuning and status
- `WebUI.h/.cpp` — HTTP routes and browser UI
- `HARDWARE_CONNECTIONS.txt` — quick wiring reference
- `CHANGELOG.md` — stage history
- `docs/GITHUB_RELEASE.md` — suggested GitHub release text
- `GITHUB_PUBLISHING_PROMPT.txt` — prompt for assistance publishing this package

## License

No project-level license is included in this package. Choose and add a license before inviting others to reuse or redistribute the project code. Third-party libraries retain their own licenses.
