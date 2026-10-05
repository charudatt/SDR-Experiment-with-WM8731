#pragma once
/*
 * Minimal WM8731 driver – I2C control only.
 * Configured for:
 *   - Line-in (LLINEIN / RLINEIN)
 *   - Slave mode (ESP32 supplies BCLK + LRC)
 *   - 16-bit I2S
 *   - MCLK = 12.288 MHz from Si5351
 *
 * begin() reports each register write individually via the
 * optional status callback (or Serial if none supplied).
 */

#include <Wire.h>
#include "pins.h"

class WM8731 {
public:
  static const uint8_t I2C_ADDR = 0x1A;   // CSB low → 0x1A

  enum Reg {
    REG_LLINEIN   = 0x00,
    REG_RLINEIN   = 0x01,
    REG_LHEADOUT  = 0x02,
    REG_RHEADOUT  = 0x03,
    REG_ANALOG    = 0x04,
    REG_DIGITAL   = 0x05,
    REG_POWERDOWN = 0x06,
    REG_INTERFACE = 0x07,
    REG_SAMPLING  = 0x08,
    REG_ACTIVE    = 0x09,
    REG_RESET     = 0x0F
  };

  // Optional callback: (regName, regAddr, value, ok)
  typedef void (*StatusCb)(const char* name, uint8_t addr, uint16_t val, bool ok);

  bool begin(StatusCb cb = nullptr) {
    statusCb = cb;
    bool allOk = true;

    // Soft reset
    allOk &= prog("RESET",     REG_RESET,     0x000);
    delay(10);

    // Power down: line-in + ADC on; mic/DAC/out/osc/clkout off
    // bit0 LINEINPD=0, bit1 MICPD=1, bit2 ADCPD=0, bit3 DACPD=1,
    // bit4 OUTPD=1, bit5 OSCPD=1, bit6 CLKOUTPD=1, bit7 POWEROFF=0
    allOk &= prog("POWERDOWN", REG_POWERDOWN, 0x06A);

    // Analogue path: Line-in, mute mic, no boost/sidetone/bypass/DAC
    allOk &= prog("ANALOG",    REG_ANALOG,    0x002);

    // Digital path: HPF on, no de-emphasis
    allOk &= prog("DIGITAL",   REG_DIGITAL,   0x000);

    // Interface: slave, 16-bit, I2S
    allOk &= prog("INTERFACE", REG_INTERFACE, 0x002);

    // Sampling control (external 12.288 MHz MCLK)
    allOk &= prog("SAMPLING",  REG_SAMPLING,  0x020);

    // Line-in volume 0 dB, unmute
    allOk &= prog("LLINEIN",   REG_LLINEIN,   0x017);
    allOk &= prog("RLINEIN",   REG_RLINEIN,   0x017);

    // Activate digital core
    allOk &= prog("ACTIVE",    REG_ACTIVE,    0x001);

    return allOk;
  }

  void setLineVolume(uint8_t vol) {
    if (vol > 31) vol = 31;
    writeReg(REG_LLINEIN, vol);
    writeReg(REG_RLINEIN, vol);
  }

private:
  StatusCb statusCb = nullptr;

  bool prog(const char* name, uint8_t reg, uint16_t val) {
    bool ok = writeReg(reg, val);
    if (statusCb) {
      statusCb(name, reg, val, ok);
    } else {
      Serial.printf("  WM8731 %-10s [0x%02X] = 0x%03X  %s\n",
                    name, reg, val, ok ? "ACK" : "NACK");
    }
    return ok;
  }

  bool writeReg(uint8_t reg, uint16_t val) {
    uint16_t packet = ((uint16_t)reg << 9) | (val & 0x1FF);
    Wire.beginTransmission(I2C_ADDR);
    Wire.write((uint8_t)(packet >> 8));
    Wire.write((uint8_t)(packet & 0xFF));
    return Wire.endTransmission() == 0;
  }
};
