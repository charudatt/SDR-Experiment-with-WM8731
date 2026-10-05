#pragma once
/*
 * Pin definitions & compile-time parameters
 * ESP32-S3 N16R8 + WM8731 + Si5351 + ILI9341
 *
 * Audio path:
 *   WebSDR (PC speaker / line-out) ──cable──> WM8731 LLINEIN
 *   ESP32 reads ADC via I2S (ADCDAT) and runs FT8 decode
 */

// -------------------- Station identity ----------------------------
#define MY_CALLSIGN     "VU2UPX"
#define MY_GRID         "MK69"

// -------------------- WiFi credentials (optional) -----------------
// Leave blank ("") to force WiFiManager captive portal.
#define WIFI_SSID       ""
#define WIFI_PASSWORD   ""

// FT8 search band within audio baseband (Hz)
#define FT8_FMIN_HZ     100.0f
#define FT8_FMAX_HZ     3100.0f

// -------------------- TFT / Touch (SPI – HSPI) --------------------
#define PIN_TFT_SCLK   12
#define PIN_TFT_MOSI   11
#define PIN_TFT_MISO   13
#define PIN_TFT_CS     10
#define PIN_TFT_DC     14
#define PIN_TFT_RST    21
#define PIN_T_CS       47
#define PIN_T_IRQ      18

// -------------------- I2C (Si5351 + WM8731 control) ---------------
#define PIN_I2C_SDA     8
#define PIN_I2C_SCL     9

// -------------------- I2S (WM8731 audio) --------------------------
#define PIN_I2S_BCLK    4
#define PIN_I2S_LRC     5
#define PIN_I2S_DACDAT  6   // unused for RX-only
#define PIN_I2S_ADCDAT  7   // from WM8731 ADCDAT

// -------------------- Control signals -----------------------------
#define PIN_RX_TX      39   // TX/RX RELAY INPUT
#define PIN_PTT        38   // INTERNAL PULLUP

// -------------------- Rotary encoder (EXTERNAL PULLUP) ------------
#define PIN_ENC_A       1
#define PIN_ENC_B       2
#define PIN_ENC_SW      3

// -------------------- Keyboard (10K PULLUP) -----------------------
#define PIN_KEY_A      40
#define PIN_KEY_B      41

// -------------------- Audio / FT8 ---------------------------------
#define AUDIO_SAMPLE_RATE    12000
#define FT8_SLOT_SEC         15
#define FT8_SAMPLES_PER_SLOT (AUDIO_SAMPLE_RATE * FT8_SLOT_SEC)

// Startup splash must remain visible for this minimum time.
#define SPLASH_HOLD_MS 4000UL

#define TFT_W  240
#define TFT_H  320

// -------------------- FT8 Decode Display --------------------------
// Number of decoded message rows shown on the TFT after each 15-second pass.
// Increase/decrease this value to suit the available screen area.
#define FT8_DISPLAY_MESSAGE_LINES 17

// FT8 display filter
// 0 = display all successfully decoded messages
// 1 = display only CQ messages
// The PASS counter always reports the total number of decoded messages.
#define FT8_DISPLAY_CQ_ONLY 0
