/*
 * ESP32-S3 N16R8 FT8 Decoder
 * ==========================
 * Audio: PC WebSDR speaker/line-out  →  cable  →  WM8731 LLINEIN
 *        ESP32 reads I2S ADCDAT @ 12 kHz, runs kgoba/ft8_lib
 *
 * Libraries: TFT_eSPI, WiFiManager (tzapu), Si5351Arduino
 * Station: VU2UPX / MK69
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <time.h>
#include <Wire.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <si5351.h>
#include "driver/i2s.h"

#include "pins.h"
#include "wm8731.h"
#include "ft8_engine.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

TFT_eSPI tft = TFT_eSPI();
Si5351 si5351;
WM8731  codec;
FT8Engine ft8;

volatile bool wifiConnected = false;
char timeStr[32] = "--:--:--";
char dateStr[16] = "--/--/----";
char ipStr[20]   = "0.0.0.0";
int levelL = 0, levelR = 0;

#define MSG_LINES FT8_DISPLAY_MESSAGE_LINES
String msgBuf[MSG_LINES];
int msgIdx = 0;
static int passDecodedCount = 0;
static int splashY = 0;
static volatile bool decoding = false;
static volatile bool captureStarted = false;
static volatile uint32_t captureCount = 0;
static volatile int16_t* activeCapture = nullptr;
static volatile int16_t* readyCapture = nullptr;
static volatile bool readySlot = false;
static portMUX_TYPE captureMux = portMUX_INITIALIZER_UNLOCKED;

static constexpr size_t CAPTURE_SAMPLES = FT8_SAMPLES_PER_SLOT;
static int16_t* captureBufA = nullptr;
static int16_t* captureBufB = nullptr;


void showSplash();
void drawMainScreen();
void updateMeters();
void updateTimeDisplay();
void addDecodedMessage(const char* msg);
void drawDecodeHeader();
void renderDecodedMessages();
void setupWiFi();
void setupTime();
void setupSi5351();
void setupI2S();
void audioI2STask(void* param);
void serviceFt8Slot();
void startCaptureAtNextSlot();
void onFt8Message(const char* text, float snr, float freq_hz, float time_sec);
void wm8731StatusCb(const char* name, uint8_t addr, uint16_t val, bool ok);

void setup() {
  const uint32_t splashStartMs = millis();

  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== ESP32-S3 FT8 Decoder REV-D4A ===");
  Serial.printf("Reset reason: %d\n", (int)esp_reset_reason());
  Serial.printf("Free heap: %u  Free PSRAM: %u\n", ESP.getFreeHeap(), ESP.getFreePsram());
  Serial.println("Audio: WM8731 LLINEIN (WebSDR via PC line-out)");
  Serial.printf("Station: %s  %s\n", MY_CALLSIGN, MY_GRID);

  pinMode(PIN_PTT, INPUT_PULLUP);
  pinMode(PIN_RX_TX, INPUT);
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  pinMode(PIN_KEY_A, INPUT_PULLUP);
  pinMode(PIN_KEY_B, INPUT_PULLUP);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);

  showSplash();
  setupSi5351();   // 12.288 MHz MCLK on CLK2 → WM8731
  delay(50);

  splashY = 150;
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("WM8731 registers:", 6, splashY);
  splashY += 12;
  bool ok = codec.begin(wm8731StatusCb);
  Serial.printf("WM8731: %s\n", ok ? "OK" : "FAIL");

  // Hold the complete startup / WM8731 debug splash for at least 4 seconds.
  const uint32_t splashElapsedMs = millis() - splashStartMs;
  if (splashElapsedMs < SPLASH_HOLD_MS) {
    delay(SPLASH_HOLD_MS - splashElapsedMs);
  }

  setupI2S();

  // Two 15-second raw mono buffers live in OPI PSRAM. This keeps acquisition
  // continuous while the native FT8 decoder works on the previous slot.
  const size_t bytesPerCapture = CAPTURE_SAMPLES * sizeof(int16_t);
  captureBufA = (int16_t*)heap_caps_malloc(bytesPerCapture, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  captureBufB = (int16_t*)heap_caps_malloc(bytesPerCapture, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  Serial.printf("Capture buffers: A=%s B=%s (%u bytes each)\n",
                captureBufA ? "OK" : "FAIL",
                captureBufB ? "OK" : "FAIL",
                (unsigned)bytesPerCapture);

  if (!captureBufA || !captureBufB) {
    Serial.println("FATAL: dual capture buffer allocation failed");
    while (true) {
      delay(1000);
    }
  }

  setupWiFi();
  setupTime();

  if (!ft8.begin(AUDIO_SAMPLE_RATE, FT8_FMIN_HZ, FT8_FMAX_HZ)) {
    Serial.println("FT8 engine init FAILED");
  }

  TaskHandle_t audioTaskHandle = NULL;
  BaseType_t taskOk = xTaskCreatePinnedToCore(audioI2STask, "audio", 8192, NULL, 2, &audioTaskHandle, 0);
  if (taskOk != pdPASS) {
    Serial.println("ERROR: audio task creation FAILED");
  } else {
    Serial.println("Audio task started (8 KB stack)");
  }

  drawMainScreen();
  Serial.println("Setup complete – decoding from LLINEIN");
}

void loop() {
  static uint32_t lastUi = 0, lastTime = 0;
  uint32_t now = millis();

  if (now - lastTime >= 250) {
    lastTime = now;
    updateTimeDisplay();

    struct tm ti;
    if (getLocalTime(&ti, 20)) {
      const int sec = ti.tm_sec;

      // Arm the continuous double-buffer acquisition exactly once at a UTC
      // slot boundary. Thereafter the two 15-second buffers alternate by
      // sample count, so decoding never stops the producer and no new timing
      // edge is introduced after every slot.
      if ((sec % 15) == 0 && !captureStarted) {
        startCaptureAtNextSlot();
      }
    }
  }

  serviceFt8Slot();

  if (now - lastUi >= 100) {
    lastUi = now;
    updateMeters();
  }
  delay(2);
}

void onFt8Message(const char* text, float snr, float freq_hz, float time_sec) {
  // UI-only D4A change: do not touch the TFT while the native decoder is
  // processing candidates. Store the decoded line and render it only after
  // decodeBuffer() has completely finished.
  (void)snr;
  (void)time_sec;
  // Apply the optional CQ-only display filter to the decoded message text
  // itself, before frequency formatting. This avoids the old fixed-column
  // parsing problem with 3-digit frequencies. The decoder's total message
  // count is updated separately from the display filter.
#if FT8_DISPLAY_CQ_ONLY
  const char* p = text;
  while (p && *p == ' ') ++p;
  if (!p || strncmp(p, "CQ", 2) != 0 ||
      (p[2] != '\0' && p[2] != ' ')) {
    return;
  }
#endif

  char line[48];
  snprintf(line, sizeof(line), "%4.0f %s", freq_hz, text);
  addDecodedMessage(line);
}

// ---------------------------------------------------------------------------
// I2S RX from WM8731 – push samples into FT8 engine
// ---------------------------------------------------------------------------
void audioI2STask(void* param) {
  (void)param;
  const size_t frames = 256;
  int16_t buffer[frames * 2];
  size_t bytesRead = 0;

  while (true) {
    if (i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytesRead, portMAX_DELAY) == ESP_OK) {
      const int n = bytesRead / 4;
      int32_t sumL = 0, sumR = 0;

      for (int i = 0; i < n; ++i) {
        const int16_t L = buffer[i * 2];
        const int16_t R = buffer[i * 2 + 1];
        sumL += abs((int)L);
        sumR += abs((int)R);

        // Capture continuously. Decoding never stops the audio producer.
        if (captureStarted && activeCapture && captureCount < CAPTURE_SAMPLES) {
          activeCapture[captureCount++] = L;
        }
      }

      if (n > 0) {
        levelL = constrain((sumL / n) / 160, 0, 100);
        levelR = constrain((sumR / n) / 160, 0, 100);
      }

      // The audio task owns the raw buffer completion event. The main loop
      // consumes the completed buffer while this task immediately switches to
      // the other PSRAM buffer, so no samples are intentionally discarded.
      if (captureStarted && captureCount >= CAPTURE_SAMPLES) {
        portENTER_CRITICAL(&captureMux);
        if (!readySlot && readyCapture == nullptr) {
          readyCapture = activeCapture;
          readySlot = true;

          if (activeCapture == captureBufA) activeCapture = captureBufB;
          else activeCapture = captureBufA;

          captureCount = 0;
        }
        portEXIT_CRITICAL(&captureMux);
      }
    }
    taskYIELD();
  }
}

void startCaptureAtNextSlot() {
  if (!captureBufA || !captureBufB) return;

  portENTER_CRITICAL(&captureMux);
  if (!readySlot) {
    activeCapture = (activeCapture == captureBufA) ? captureBufB : captureBufA;
    if (!activeCapture) activeCapture = captureBufA;
    captureCount = 0;
    captureStarted = true;
  }
  portEXIT_CRITICAL(&captureMux);

  Serial.println("FT8 CAPTURE: UTC slot boundary, capture offset = 0 ms");
}

void serviceFt8Slot() {
  int16_t* buf = nullptr;

  portENTER_CRITICAL(&captureMux);
  if (readySlot && readyCapture) {
    buf = (int16_t*)readyCapture;
    readyCapture = nullptr;
    readySlot = false;
  }
  portEXIT_CRITICAL(&captureMux);

  if (!buf || decoding) return;

  decoding = true;
  Serial.println("=== FT8 15 s NATIVE LIBRARY DECODE ===");
  Serial.printf("CAPTURE: %u samples, UTC-aligned slot, offset 0 ms\n",
                (unsigned)CAPTURE_SAMPLES);

  // Start a fresh 15-second decode pass on the display.
  passDecodedCount = 0;
  msgIdx = 0;
  for (int i = 0; i < MSG_LINES; ++i) msgBuf[i] = "";
  tft.fillRect(0, 74, 240, 246, TFT_BLACK);
  drawDecodeHeader();

  int n = ft8.decodeBuffer(buf, CAPTURE_SAMPLES, onFt8Message);
  passDecodedCount = n;

  // UI-only D4A: render the complete pass only after the decoder has returned.
  // No TFT drawing occurs from the decoder callback.
  renderDecodedMessages();
  drawDecodeHeader();
  Serial.printf("FT8 MESSAGES: %d\n", n);

  // The buffer is now free for reuse by the producer. The decoder has already
  // copied all required samples into its own monitor/waterfall storage.
  decoding = false;
}

void setupI2S() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = AUDIO_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = true,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 12288000
  };
  i2s_pin_config_t pins = {
    .mck_io_num   = I2S_PIN_NO_CHANGE,  // MCLK from Si5351
    .bck_io_num   = PIN_I2S_BCLK,
    .ws_io_num    = PIN_I2S_LRC,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = PIN_I2S_ADCDAT
  };
  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
  Serial.println("I2S RX @ 12 kHz (WM8731 LLINEIN)");
}

// ---------------------------------------------------------------------------
void wm8731StatusCb(const char* name, uint8_t addr, uint16_t val, bool ok) {
  char line[40];
  snprintf(line, sizeof(line), "%-9s %02Xh=%03Xh %s", name, addr, val, ok ? "ACK" : "NACK");
  Serial.println(line);
  if (splashY < 300) {
    tft.setTextColor(ok ? TFT_GREEN : TFT_RED, TFT_BLACK);
    tft.setTextSize(1);
    tft.drawString(line, 6, splashY);
    splashY += 11;
  }
}

void showSplash() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("ESP32-S3 FT8", 20, 10);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Audio: WM8731 LLINEIN", 20, 40);
  tft.drawString("(WebSDR via PC line-out)", 20, 54);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(MY_CALLSIGN "  " MY_GRID, 20, 74);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("Init Si5351 / WM8731...", 20, 100);
}

void drawMainScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, 240, 22, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextSize(1);
  tft.drawString(MY_CALLSIGN " FT8", 4, 6);
  tft.drawString(MY_GRID, 200, 6);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("L", 4, 28);
  tft.drawRect(16, 26, 90, 12, TFT_DARKGREY);
  tft.drawString("R", 120, 28);
  tft.drawRect(132, 26, 90, 12, TFT_DARKGREY);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(ipStr, 4, 44);
  tft.drawString(dateStr, 110, 44);
  tft.drawString(timeStr, 180, 44);

  tft.drawFastHLine(0, 58, 240, TFT_DARKGREY);
  tft.fillRect(0, 60, 240, 14, TFT_BLACK);
  drawDecodeHeader();
  tft.fillRect(0, 74, 240, 246, TFT_BLACK);
}

void updateMeters() {
  int wL = constrain(levelL, 0, 100) * 88 / 100;
  int wR = constrain(levelR, 0, 100) * 88 / 100;
  tft.fillRect(17, 27, 88, 10, TFT_BLACK);
  if (wL) tft.fillRect(17, 27, wL, 10, TFT_GREEN);
  tft.fillRect(133, 27, 88, 10, TFT_BLACK);
  if (wR) tft.fillRect(133, 27, wR, 10, TFT_GREEN);
}

void updateTimeDisplay() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 50)) {
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    strftime(dateStr, sizeof(dateStr), "%d-%m-%Y", &timeinfo);
  }
  if (wifiConnected) {
    strncpy(ipStr, WiFi.localIP().toString().c_str(), sizeof(ipStr) - 1);
  }
  tft.fillRect(0, 42, 240, 14, TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString(ipStr, 4, 44);
  tft.drawString(dateStr, 110, 44);
  tft.drawString(timeStr, 180, 44);
}

void drawDecodeHeader() {
  tft.fillRect(0, 60, 240, 14, TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("FREQ Hz  FT8 MESSAGE", 4, 62);

  char countStr[16];
  snprintf(countStr, sizeof(countStr), "PASS:%d", passDecodedCount);
  tft.drawRightString(countStr, 236, 62, 1);
}

void addDecodedMessage(const char* msg) {
  // Decoder callback must remain display-free. This is intentionally only
  // a small RAM update; TFT rendering is deferred until the decode pass ends.
  msgBuf[msgIdx] = String(msg);
  msgIdx = (msgIdx + 1) % MSG_LINES;
  ++passDecodedCount;
  Serial.printf("FT8: %s\n", msg);
}

void renderDecodedMessages() {
  tft.fillRect(0, 74, 240, 246, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);

  int y = 76;
  for (int i = 0; i < MSG_LINES; i++) {
    int idx = (msgIdx + i) % MSG_LINES;
    if (msgBuf[idx].length()) {
      // CQ messages are highlighted yellow; all other decoded messages
      // retain the normal white display colour. The decoder itself is
      // completely unaffected by this UI-only classification.
      const char* line = msgBuf[idx].c_str();
      // The stored display line is always formatted as "%4.0f %s".
      // Therefore the message text starts at character 5.  Using strchr()
      // here incorrectly treats the leading padding space of 3-digit
      // frequencies as the separator, which caused some CQ messages to
      // remain white.
      const char* msg = (strlen(line) >= 5) ? (line + 5) : line;
      const bool isCQ = (strncmp(msg, "CQ", 2) == 0 &&
                         (msg[2] == '\0' || msg[2] == ' '));
      tft.setTextColor(isCQ ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
      tft.drawString(msgBuf[idx], 4, y);
      y += 14;
      if (y > 310) break;
    }
  }
}

void setupWiFi() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("WiFi connecting...", 20, 40);

  WiFi.mode(WIFI_STA);
  bool connected = false;

  if (strlen(WIFI_SSID) > 0) {
    Serial.printf("Trying SSID: %s\n", WIFI_SSID);
    tft.drawString(WIFI_SSID, 20, 60);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) delay(500);
    if (WiFi.status() == WL_CONNECTED) connected = true;
    else {
      tft.setTextColor(TFT_ORANGE, TFT_BLACK);
      tft.drawString("SSID fail -> portal", 20, 80);
      WiFi.disconnect(true);
      delay(200);
    }
  }

  if (!connected) {
    WiFiManager wm;
    wm.setConfigPortalTimeout(180);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("Portal: ESP32-FT8-Setup", 20, 100);
    connected = wm.autoConnect("ESP32-FT8-Setup");
  }

  if (!connected) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("WiFi FAIL - OFFLINE", 20, 120);
    Serial.println("WiFi unavailable - continuing in OFFLINE mode");
    wifiConnected = false;
    return;
  }

  wifiConnected = true;
  strncpy(ipStr, WiFi.localIP().toString().c_str(), sizeof(ipStr) - 1);
  Serial.printf("WiFi OK %s\n", ipStr);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("WiFi OK", 20, 120);
  tft.drawString(ipStr, 20, 140);
  delay(600);
}

void setupTime() {
  configTime(5 * 3600 + 30 * 60, 0, "pool.ntp.org", "time.nist.gov");
  struct tm ti;
  for (int i = 0; i < 50; i++) {
    if (getLocalTime(&ti, 100)) break;
    delay(100);
  }
}

void setupSi5351() {
  if (!si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0)) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("Si5351 FAIL", 20, 120);
    return;
  }
  si5351.set_freq(1228800000ULL, SI5351_CLK2);
  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_4MA);
  si5351.output_enable(SI5351_CLK2, 1);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("Si5351 OK 12.288 MHz", 20, 120);
}
