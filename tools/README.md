# Validation Tools

This directory contains **independently validated experimental tools** used while developing the main SDR platform.

These are not necessarily components of the final SDR application. They exist to prove individual hardware and software functions before those functions are integrated into the main project.

## Rules for validation tools

1. Keep each validated revision unchanged after it has been confirmed on hardware.
2. Document the exact hardware function being tested.
3. Record important pin assignments, library requirements and configuration values.
4. If a change is required after validation, create the next revision instead of silently modifying the old one.
5. Each tool gets its own `doc/` directory.

## Current tools

| Tool | Purpose | Status |
|---|---|---|
| [REV-S7](REV-S7/) | WM8731 MIC → ADC → ESP32-S3 I2S → DAC audio loopback, with validated 1 kHz tone and touch UI | **FROZEN / VALIDATED** |

## Revision naming

Validation revisions are sequential. For example, a future modification of REV-S7 should become **REV-S8**, leaving REV-S7 unchanged.
