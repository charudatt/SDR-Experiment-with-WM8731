#pragma once

// REV-SI4732-STAGE1B — standalone Si4732 FM + rotary + web validation.
// This sketch is separate from the host REV-D5 project.
#define SI4732_SDA_PIN 8
#define SI4732_SCL_PIN 9
#define SI4732_RST_PIN 19
#define SI4732_ADDR_LOW 0x11
#define SI4732_ADDR_HIGH 0x63

#define ENC_A_PIN 1
#define ENC_B_PIN 2
#define ENC_SW_PIN 3

// FM frequency units are 10 kHz: 10390 = 103.90 MHz.
#define SI4732_FM_MIN_10KHZ 6400
#define SI4732_FM_MAX_10KHZ 10800
#define SI4732_FM_START_10KHZ 10390
#define SI4732_FM_STEP_10KHZ 10  // 100 kHz per encoder detent

#define SI4732_I2C_CLOCK_HZ 100000
#define SI4732_POWERUP_SETTLE_MS 500
#define SI4732_TUNE_SETTLE_MS 100
#define SWITCH_DEBOUNCE_MS 30
#define LONG_PRESS_MS 700

#define WIFI_PORTAL_TIMEOUT_SECONDS 180
#define WIFI_AP_NAME "Si4732-Radio-Setup"
