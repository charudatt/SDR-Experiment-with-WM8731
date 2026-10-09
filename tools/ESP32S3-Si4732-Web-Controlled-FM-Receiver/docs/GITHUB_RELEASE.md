# Suggested GitHub Release

**Release title:** Stage 1B — Standalone Si4732 FM Receiver with Web + Rotary Control

**Suggested tag:** `stage1b-web-control`

## Summary

This release adds a lightweight web control page to the standalone ESP32-S3 + Si4732 FM receiver validation sketch while retaining physical rotary encoder tuning.

## Validated

- Si4732 detection and initialization at I²C address `0x11`
- FM tune/readback
- Rotary encoder tuning
- Wi-Fi connection and HTTP server startup
- Browser page access
- Frequency entry/SET and STEP + / STEP − controls
- Browser frequency display follows physical encoder changes

## Hardware

ESP32-S3; I²C SDA GPIO8, SCL GPIO9; Si4732 RST GPIO19; rotary encoder A/B/SW GPIO1/2/3.

## Limitations

This is a standalone FM control test. It does not integrate WM8731 audio, Si5351, TFT/OLED, or the FT8 decoder. An antenna was not connected during the reported test, so RF reception and sensitivity have not been validated. The HTTP interface is intended for a trusted local network only and has no authentication.
