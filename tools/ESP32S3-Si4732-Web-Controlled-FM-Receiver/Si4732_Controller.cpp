#include "Si4732_Controller.h"
#include "Si4732_Config.h"
#include <Wire.h>

static const uint8_t FM_FUNCTION = 0;

bool Si4732Controller::begin(uint8_t resetPin, uint8_t sdaPin, uint8_t sclPin) {
  _ready = false;
  _address = 0;
  _frequency = 0;
  _requestedFrequency = SI4732_FM_START_10KHZ;

  pinMode(resetPin, OUTPUT);
  digitalWrite(resetPin, HIGH);
  Wire.begin(sdaPin, sclPin);
  Wire.setClock(SI4732_I2C_CLOCK_HZ);

  _rx.setI2CFastModeCustom(SI4732_I2C_CLOCK_HZ);
  _rx.setMaxDelayPowerUp(SI4732_POWERUP_SETTLE_MS);
  _rx.setMaxDelaySetFrequency(SI4732_TUNE_SETTLE_MS);

  const int16_t detectedAddress = _rx.getDeviceI2CAddress(resetPin);
  if (detectedAddress == 0) {
    Serial.println("SI4732 address detection: FAIL");
    return false;
  }
  _address = (uint8_t)detectedAddress;
  Serial.printf("SI4732 address detection: PASS (0x%02X)\n", _address);

  _rx.setup(resetPin, FM_FUNCTION);
  delay(SI4732_POWERUP_SETTLE_MS);
  _rx.getFirmware();

  _partNumber = _rx.getFirmwarePN();
  _fwMajor = (char)_rx.getFirmwareFWMAJOR();
  _fwMinor = (char)_rx.getFirmwareFWMINOR();
  _patchId = ((uint16_t)_rx.getFirmwarePATCHH() << 8) | _rx.getFirmwarePATCHL();
  _componentMajor = (char)_rx.getFirmwareCMPMAJOR();
  _componentMinor = (char)_rx.getFirmwareCMPMINOR();
  _chipRevision = (char)_rx.getFirmwareCHIPREV();

  _requestedFrequency = SI4732_FM_START_10KHZ;
  _rx.setFM(SI4732_FM_MIN_10KHZ, SI4732_FM_MAX_10KHZ,
            SI4732_FM_START_10KHZ, SI4732_FM_STEP_10KHZ);
  delay(SI4732_TUNE_SETTLE_MS);
  _frequency = _rx.getFrequency();
  refreshSignalQuality();
  _ready = (_frequency != 0);
  Serial.printf("SI4732 library initialization: %s\n",
                _ready ? "PASS" : "INIT OK, FREQUENCY READBACK UNCONFIRMED");
  return true;
}

bool Si4732Controller::tune(uint16_t frequency10kHz) {
  if (!_address) return false;
  if (frequency10kHz < SI4732_FM_MIN_10KHZ ||
      frequency10kHz > SI4732_FM_MAX_10KHZ) return false;

  _requestedFrequency = frequency10kHz;
  _rx.setFrequency(frequency10kHz);
  delay(SI4732_TUNE_SETTLE_MS);
  const uint16_t readback = _rx.getFrequency();
  _frequency = readback;
  refreshSignalQuality();
  _ready = (readback != 0);
  return (readback == frequency10kHz);
}

void Si4732Controller::refreshSignalQuality() {
  if (!_address) return;
  _rx.getCurrentReceivedSignalQuality();
  _rssi = _rx.getCurrentRSSI();
  _snr = _rx.getCurrentSNR();
  _validChannel = _rx.getCurrentValidChannel();
}
