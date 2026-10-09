/*
  REV-SI4732-STAGE1B — Standalone Si4732 FM + rotary + web control

  This is a separate test sketch for ESP32-S3 N16R8.
  It does NOT include or modify the host REV-D5 project.
  No TFT, OLED, WM8731, Si5351, FT8 or host audio pipeline is initialised.

  Libraries:
    - PU2CLR SI4735 (SI4735.h)
    - ESPRotary
    - WiFiManager
    - ESP32 Arduino core: WiFi.h and WebServer.h
*/

#include <Arduino.h>
#include <Wire.h>
#include <SI4735.h>
#include <ESPRotary.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "Si4732_Config.h"
#include "Si4732_Controller.h"
#include "WebUI.h"

static Si4732Controller radio;
static ESPRotary encoder(ENC_B_PIN, ENC_A_PIN, 4);
static volatile int32_t encoderDelta = 0;
static int32_t lastEncoderCount = 0;
static bool radioReady = false;

static bool lastRawSwitch = HIGH;
static bool stableSwitch = HIGH;
static uint32_t switchEdgeMs = 0;
static uint32_t pressStartMs = 0;
static bool longPressReported = false;

static void onRotateRight(ESPRotary &) { encoderDelta++; }
static void onRotateLeft(ESPRotary &)  { encoderDelta--; }

static void handleEncoderSwitch() {
  const bool raw = digitalRead(ENC_SW_PIN);
  const uint32_t now = millis();
  if (raw != lastRawSwitch) {
    lastRawSwitch = raw;
    switchEdgeMs = now;
  }
  if ((now - switchEdgeMs) >= SWITCH_DEBOUNCE_MS && raw != stableSwitch) {
    stableSwitch = raw;
    if (stableSwitch == LOW) {
      pressStartMs = now;
      longPressReported = false;
      Serial.println("ENCODER SW: pressed");
    } else {
      const uint32_t held = now - pressStartMs;
      Serial.printf("ENCODER SW: %s press (%lu ms); no action assigned in Stage 1B.\n",
                    held >= LONG_PRESS_MS ? "long" : "short", (unsigned long)held);
    }
  }
  if (stableSwitch == LOW && !longPressReported && (now - pressStartMs) >= LONG_PRESS_MS) {
    longPressReported = true;
    Serial.println("ENCODER SW: long-press threshold reached");
  }
}

static void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT_SECONDS);
  Serial.println("Connecting with WiFiManager; if no saved credentials, join the setup AP.");
  Serial.printf("Setup AP name: %s\n", WIFI_AP_NAME);
  const bool connected = wm.autoConnect(WIFI_AP_NAME);
  if (connected) {
    Serial.printf("Wi-Fi connected. SSID=%s IP=%s\n",
                  WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    return;
  }

  Serial.println("Wi-FiManager timed out; starting receiver access point.");
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Si4732-Radio");
  Serial.printf("Receiver AP IP=%s\n", WiFi.softAPIP().toString().c_str());
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("REV-SI4732-STAGE1B");
  Serial.println("Standalone Si4732 FM + rotary + web control");
  Serial.println("No TFT/OLED, WM8731, Si5351 or FT8 host code is included.");
  Serial.printf("I2C SDA GPIO%u, SCL GPIO%u, Si4732 RST GPIO%u\n",
                SI4732_SDA_PIN, SI4732_SCL_PIN, SI4732_RST_PIN);
  Serial.printf("Encoder A/B/SW GPIO%u/%u/%u\n", ENC_A_PIN, ENC_B_PIN, ENC_SW_PIN);

  pinMode(ENC_A_PIN, INPUT_PULLUP);
  pinMode(ENC_B_PIN, INPUT_PULLUP);
  pinMode(ENC_SW_PIN, INPUT_PULLUP);
  encoder.setRightRotationHandler(onRotateRight);
  encoder.setLeftRotationHandler(onRotateLeft);

  radioReady = radio.begin(SI4732_RST_PIN, SI4732_SDA_PIN, SI4732_SCL_PIN);
  if (!radioReady) {
    Serial.println("RADIO INIT: FAIL. Web page will still report unavailable radio.");
  } else {
    Serial.println("RADIO INIT: PASS");
    Serial.printf("I2C address: 0x%02X\n", radio.address());
    Serial.printf("Firmware part: 0x%02X  firmware %c.%c  patch 0x%04X  component %c.%c  chip %c\n",
                  radio.partNumber(), radio.fwMajor(), radio.fwMinor(), radio.patchId(),
                  radio.componentMajor(), radio.componentMinor(), radio.chipRevision());
    const bool tuneOK = radio.tune(SI4732_FM_START_10KHZ);
    Serial.printf("INITIAL TUNE %u.%02u MHz: %s\n",
                  SI4732_FM_START_10KHZ / 100, SI4732_FM_START_10KHZ % 100,
                  tuneOK ? "PASS" : "FAIL");
    Serial.printf("READBACK %u.%02u MHz RSSI=%u SNR=%u VALID=%s\n",
                  radio.frequency() / 100, radio.frequency() % 100,
                  radio.rssi(), radio.snr(), radio.validChannel() ? "YES" : "NO");
  }

  connectWiFi();
  webUIBegin(radio);

  Serial.println("Open the printed IP address in a phone/computer browser.");
  Serial.println("Encoder and web buttons tune the same FM receiver.");
}

void loop() {
  encoder.loop();
  handleEncoderSwitch();

  int32_t delta;
  noInterrupts();
  delta = encoderDelta;
  encoderDelta = 0;
  interrupts();

  if (radioReady && delta != 0) {
    lastEncoderCount += delta;
    int32_t next = (int32_t)radio.requestedFrequency() + delta * SI4732_FM_STEP_10KHZ;
    if (next < SI4732_FM_MIN_10KHZ) next = SI4732_FM_MAX_10KHZ;
    if (next > SI4732_FM_MAX_10KHZ) next = SI4732_FM_MIN_10KHZ;
    const bool ok = radio.tune((uint16_t)next);
    Serial.printf("ENCODER delta=%ld count=%ld tune=%s requested=%ld.%02ld readback=%u.%02u\n",
                  (long)delta, (long)lastEncoderCount, ok ? "PASS" : "FAIL",
                  (long)(next / 100), (long)(next % 100),
                  radio.frequency() / 100, radio.frequency() % 100);
  }

  webUILoop();
  delay(1);
}
