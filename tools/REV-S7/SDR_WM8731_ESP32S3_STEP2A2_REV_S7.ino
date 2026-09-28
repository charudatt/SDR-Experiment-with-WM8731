#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <si5351.h>
#include <ESP_I2S.h>

#include "config.h"

TFT_eSPI tft = TFT_eSPI();
Si5351 si5351;
I2SClass I2S;

struct RegResult {
  uint8_t reg;
  uint16_t data;
  bool attempted;
  bool ack;
};

static RegResult results[] = {
  {15, 0x000, false, false},
  { 0, 0x017, false, false},
  { 1, 0x017, false, false},
  { 2, 0x079, false, false},
  { 3, 0x079, false, false},
  { 4, 0x012, false, false},
  { 5, 0x000, false, false},
  { 6, 0x000, false, false},
  { 7, 0x002, false, false},
  { 8, 0x020, false, false},
  { 9, 0x001, false, false}
};

static const uint8_t RESULT_COUNT =
    sizeof(results) / sizeof(results[0]);

// ------------------------------------------------------------
// CLEAN PORTRAIT DISPLAY
// 240 x 320
//
// Nothing is redrawn over the register rows.
// Each register has its own fixed row.
// ------------------------------------------------------------

static void drawHeader()
{
  tft.fillScreen(TFT_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(8, 6);
  tft.print("WM8731 DIAGNOSTIC");

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(8, 29);
  tft.print("0x1A   I2C 100kHz   PORTRAIT");

  tft.drawFastHLine(5, 44, 230, TFT_DARKGREY);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(8, 51);
  tft.print("I2C:");

  tft.setCursor(122, 51);
  tft.print("Si5351:");

  tft.setCursor(8, 67);
  tft.print("WM8731 PROBE:");

  tft.drawFastHLine(5, 82, 230, TFT_DARKGREY);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(8, 87);
  tft.print("REG");

  tft.setCursor(60, 87);
  tft.print("DATA");

  tft.setCursor(145, 87);
  tft.print("RESULT");
}

static void showRegister(uint8_t index)
{
  if (index >= RESULT_COUNT)
    return;

  // 11 rows, 17 pixels apart: 101 ... 271
  const int y = 101 + ((int)index * 17);

  // Clear ONLY this row.
  tft.fillRect(5, y, 230, 14, TFT_BLACK);

  char regText[8];
  char dataText[8];

  snprintf(regText, sizeof(regText),
           "R%02X", results[index].reg);

  snprintf(dataText, sizeof(dataText),
           "D%03X", results[index].data);

  tft.setTextSize(1);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(8, y);
  tft.print(regText);

  tft.setCursor(60, y);
  tft.print(dataText);

  if (!results[index].attempted)
  {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(145, y);
    tft.print("---");
  }
  else if (results[index].ack)
  {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(145, y);
    tft.print("ACK");
  }
  else
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(145, y);
    tft.print("NACK");
  }
}

static void showStatus(const char *text, uint16_t color)
{
  // Dedicated bottom status area.
  tft.fillRect(5, 291, 230, 25, TFT_BLACK);
  tft.drawFastHLine(5, 289, 230, TFT_DARKGREY);

  tft.setTextSize(1);
  tft.setTextColor(color, TFT_BLACK);
  tft.setCursor(8, 300);
  tft.print(text);
}

static bool probeWM8731()
{
  Wire.beginTransmission(CONFIG_WM8731_I2C_ADDR);
  return Wire.endTransmission() == 0;
}

static bool writeRegister(uint8_t index)
{
  const uint8_t reg = results[index].reg;
  const uint16_t data = results[index].data;

  // WM8731 2-wire format:
  // first byte  = register[6:0] + data[8]
  // second byte = data[7:0]
  uint16_t word =
      ((uint16_t)(reg & 0x7F) << 9) |
      (data & 0x01FF);

  Wire.beginTransmission(CONFIG_WM8731_I2C_ADDR);
  Wire.write((uint8_t)(word >> 8));
  Wire.write((uint8_t)(word & 0xFF));

  const uint8_t result = Wire.endTransmission();

  results[index].attempted = true;
  results[index].ack = (result == 0);

  showRegister(index);

  return result == 0;
}

static void runDiagnostic()
{
  drawHeader();

  // ----------------------------------------------------------
  // I2C
  // ----------------------------------------------------------
  Wire.begin(CONFIG_I2C_SDA_PIN, CONFIG_I2C_SCL_PIN);
  Wire.setClock(100000UL);
  delay(100);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(35, 51);
  tft.print("OK");

  // ----------------------------------------------------------
  // Si5351
  // ----------------------------------------------------------
  bool siOK = si5351.init(
      SI5351_CRYSTAL_LOAD_8PF,
      CONFIG_SI5351_XTAL_HZ,
      CONFIG_SI5351_CORRECTION);

  tft.setTextColor(siOK ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.setCursor(170, 51);
  tft.print(siOK ? "OK" : "FAIL");

  if (siOK)
  {
    si5351.output_enable(SI5351_CLK1, 0);

    si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
    si5351.set_freq(
        (uint64_t)CONFIG_CLK0_HZ * SI5351_FREQ_MULT,
        SI5351_CLK0);
    si5351.output_enable(SI5351_CLK0, 1);

    si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_2MA);
    si5351.set_freq(
        (uint64_t)CONFIG_CLK2_MCLK_HZ * SI5351_FREQ_MULT,
        SI5351_CLK2);
    si5351.output_enable(SI5351_CLK2, 1);
  }

  // Give MCLK time to settle.
  delay(250);

  // ----------------------------------------------------------
  // WM8731 I2C PROBE
  // ----------------------------------------------------------
  bool found = probeWM8731();

  tft.setTextColor(found ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.setCursor(105, 67);
  tft.print(found ? "ACK" : "NACK");

  // Show all register rows before programming.
  for (uint8_t i = 0; i < RESULT_COUNT; ++i)
    showRegister(i);

  if (!found)
  {
    showStatus("WM8731 NOT FOUND - STOP", TFT_RED);
    return;
  }

  // ----------------------------------------------------------
  // REGISTER PROGRAMMING
  // ----------------------------------------------------------

  // R15 RESET
  if (!writeRegister(0))
  {
    showStatus("FIRST NACK = R0F", TFT_RED);
    return;
  }

  delay(20);

  // R00 through R09
  for (uint8_t i = 1; i < RESULT_COUNT; ++i)
  {
    if (!writeRegister(i))
    {
      char msg[32];
      snprintf(msg, sizeof(msg),
               "FIRST NACK = R%02X",
               results[i].reg);
      showStatus(msg, TFT_RED);
      return;
    }

    delay(2);
  }

  delay(20);
  showStatus("ALL REGISTERS ACK", TFT_GREEN);
}


static void drawNextTestScreen()
{
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(8, 8);
  tft.print("WM8731 TEST MENU");

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(8, 34);
  tft.print("Startup validation COMPLETE");

  const int w = 72;
  const int h = 54;
  const int gap = 6;
  const int x0 = 6;
  const int y = 166;

  const char *labels[4] = {"1 kHz", "LOOP", "ADC TEST", "CODEC"};

  for (int i = 0; i < 4; ++i)
  {
    int x = x0 + i * (w + gap);
    tft.fillRoundRect(x, y, w, h, 6, TFT_NAVY);
    tft.drawRoundRect(x, y, w, h, 6, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.drawCentreString(labels[i], x + w / 2, y + 20, 2);
  }

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawCentreString("Select a test", 160, 100, 2);
}


// ------------------------------------------------------------
// TOUCH STATE
// Exactly one of the four buttons can be active.
// ------------------------------------------------------------

static int activeButton = -1;
static bool touchWasDown = false;

// ------------------------------------------------------------
// BUTTON STATE DRAWING
// Exactly one button may be active at a time.
// ------------------------------------------------------------

static void drawButtonState(int button, bool active)
{
  if (button < 0 || button > 3)
    return;

  const int w = 72;
  const int h = 54;
  const int gap = 6;
  const int x0 = 6;
  const int by = 166;

  const char *labels[4] = {
    "1 kHz", "LOOP", "ADC TEST", "CODEC"
  };

  const int bx = x0 + button * (w + gap);
  const uint16_t fill = active ? TFT_CYAN : TFT_NAVY;
  const uint16_t text = active ? TFT_BLACK : TFT_WHITE;

  tft.fillRoundRect(bx, by, w, h, 6, fill);
  tft.drawRoundRect(bx, by, w, h, 6, TFT_WHITE);
  tft.setTextColor(text, fill);
  tft.drawCentreString(labels[button], bx + w / 2, by + 20, 2);
}

// ------------------------------------------------------------
// AUDIO / I2S
// Proven WM8731 1 kHz tone path.
// I2S is initialized once after the codec diagnostic.
// The 1 kHz tone is started only by the 1 kHz touch button.
// ------------------------------------------------------------

static bool i2sReady = false;
static bool tone1kOn = false;
static float tonePhase = 0.0f;

static uint8_t loopBuffer[512];
static bool loopOn = false;

// WM8731 R4 values:
// 0x12 = normal validated setup (DAC selected, MIC muted, LINE -> ADC)
// 0x14 = LOOP mode (DAC selected, MIC unmuted, MIC -> ADC)
static const uint16_t WM8731_R4_NORMAL = 0x012;
static const uint16_t WM8731_R4_MIC_LOOP = 0x014;

static const float TONE_AMPLITUDE = 3500.0f;
static const uint32_t AUDIO_SAMPLE_RATE = 48000UL;
static int16_t audioBuffer[256];

static bool startI2S()
{
  // BCLK, LRCLK, DOUT, DIN
  I2S.setPins(4, 5, 6, 7);

  if (!I2S.begin(I2S_MODE_STD,
                 AUDIO_SAMPLE_RATE,
                 I2S_DATA_BIT_WIDTH_16BIT,
                 I2S_SLOT_MODE_STEREO))
  {
    return false;
  }

  i2sReady = true;
  return true;
}

static bool writeWM8731RegisterValue(uint8_t reg, uint16_t data)
{
  uint16_t word =
      ((uint16_t)(reg & 0x7F) << 9) |
      (data & 0x01FF);

  Wire.beginTransmission(CONFIG_WM8731_I2C_ADDR);
  Wire.write((uint8_t)(word >> 8));
  Wire.write((uint8_t)(word & 0xFF));
  return Wire.endTransmission() == 0;
}

static void setLoop(bool on)
{
  if (!i2sReady)
    return;

  if (on)
  {
    // Select MIC input for ADC and remove the MIC mute.
    // DAC remains selected for the returned audio.
    if (!writeWM8731RegisterValue(4, WM8731_R4_MIC_LOOP))
      return;

    tone1kOn = false;
    loopOn = true;
  }
  else
  {
    loopOn = false;

    // Restore the exact validated REV-S4 codec state.
    writeWM8731RegisterValue(4, WM8731_R4_NORMAL);
  }
}

static void setTone1k(bool on)
{
  if (!i2sReady)
    return;

  tone1kOn = on;

  if (on)
    tonePhase = 0.0f;
}

static void serviceTone()
{
  if (!tone1kOn)
    return;

  const float phaseStep = 1000.0f / (float)AUDIO_SAMPLE_RATE;

  // 128 stereo frames = 256 int16 samples.
  for (int frame = 0; frame < 128; ++frame)
  {
    float s = sinf(2.0f * PI * tonePhase) * TONE_AMPLITUDE;
    int16_t sample = (int16_t)s;

    audioBuffer[frame * 2]     = sample;
    audioBuffer[frame * 2 + 1] = sample;

    tonePhase += phaseStep;
    if (tonePhase >= 1.0f)
      tonePhase -= 1.0f;
  }

  I2S.write((uint8_t *)audioBuffer, sizeof(audioBuffer));
}

// ------------------------------------------------------------
// LOOPBACK
// WM8731 ADC -> I2S RX -> I2S TX -> WM8731 DAC
// ------------------------------------------------------------

static void serviceLoop()
{
  if (!loopOn || !i2sReady)
    return;

  int availableBytes = I2S.available();
  if (availableBytes <= 0)
    return;

  size_t count = (size_t)availableBytes;
  if (count > sizeof(loopBuffer))
    count = sizeof(loopBuffer);

  // Preserve complete stereo 16-bit frames.
  count &= ~((size_t)3);
  if (count == 0)
    return;

  size_t got = I2S.readBytes((char *)loopBuffer, count);
  if (got == 0)
    return;

  got &= ~((size_t)3);
  if (got == 0)
    return;

  I2S.write(loopBuffer, got);
}

// ------------------------------------------------------------
// TOUCH
// Present hardware uses TFT_eSPI touch on GPIO47.
// Landscape mapping obtained from the validated touch calibration.
// ------------------------------------------------------------

static const int RAW_Y_LEFT   = 3439;
static const int RAW_Y_RIGHT  = 738;
static const int RAW_X_TOP    = 2213;
static const int RAW_X_BOTTOM = 244;

static int touchScreenX(uint16_t rawY)
{
  long v = map((long)rawY,
               RAW_Y_LEFT, RAW_Y_RIGHT,
               0, 319);
  return constrain((int)v, 0, 319);
}

static int touchScreenY(uint16_t rawX)
{
  long v = map((long)rawX,
               RAW_X_TOP, RAW_X_BOTTOM,
               0, 239);
  return constrain((int)v, 0, 239);
}

static int getTestButton(int x, int y)
{
  const int w = 72;
  const int gap = 6;
  const int x0 = 6;
  const int buttonY = 166;
  const int buttonH = 54;

  if (y < buttonY || y >= buttonY + buttonH)
    return -1;

  for (int i = 0; i < 4; ++i)
  {
    int bx = x0 + i * (w + gap);
    if (x >= bx && x < bx + w)
      return i;
  }

  return -1;
}

static void handleTouch()
{
  uint16_t rawX = 0;
  uint16_t rawY = 0;

  if (!tft.getTouchRaw(&rawX, &rawY))
  {
    // Genuine release: allow the next press.
    touchWasDown = false;
    return;
  }

  uint16_t rawZ = tft.getTouchRawZ();

  // REV-M validated idle/no-touch rejection.
  if (rawZ < 80)
  {
    touchWasDown = false;
    return;
  }

  int x = touchScreenX(rawY);
  int y = touchScreenY(rawX);
  int button = getTestButton(x, y);

  // Ignore touches outside the four buttons.
  if (button < 0)
    return;

  // A press is accepted only after the previous press has been released.
  if (touchWasDown)
    return;

  touchWasDown = true;

  // Toggle mode. The previous active button is remembered so that
  // touching the same button again means OFF.
  if (activeButton == button)
  {
    // Second touch of the same button: toggle OFF.
    if (button == 0)
      setTone1k(false);
    else if (button == 1)
      setLoop(false);

    drawButtonState(button, false);
    activeButton = -1;
    return;
  }

  // Selecting another button turns the previous one OFF first.
  if (activeButton >= 0)
    drawButtonState(activeButton, false);

  activeButton = button;
  drawButtonState(activeButton, true);

  if (button == 0)
    setTone1k(true);
  else if (button == 1)
    setLoop(true);
}

void setup()
{
  tft.init();

  // STEP 1: validated WM8731 startup/register diagnostic.
  tft.setRotation(0);
  runDiagnostic();

  // Keep the complete successful register screen visible for 3 seconds.
  delay(3000);

  // Initialize the proven WM8731 I2S output path once.
  startI2S();

  // STEP 2: move to the next test screen.
  tft.setRotation(1);
  drawNextTestScreen();
}

void loop()
{
  // Touch remains the exact validated REV-M method.
  handleTouch();

  if (loopOn)
    serviceLoop();
  else
    serviceTone();

  delay(2);
}
