#pragma once
#include <Arduino.h>
#include <SI4735.h>
#include "Si4732_Config.h"

class Si4732Controller {
public:
  bool begin(uint8_t resetPin, uint8_t sdaPin, uint8_t sclPin);
  bool tune(uint16_t frequency10kHz);
  void refreshSignalQuality();

  uint8_t address() const { return _address; }
  uint16_t frequency() const { return _frequency; }
  uint16_t requestedFrequency() const { return _requestedFrequency; }
  uint8_t rssi() const { return _rssi; }
  uint8_t snr() const { return _snr; }
  bool validChannel() const { return _validChannel; }
  bool ready() const { return _ready; }
  uint8_t partNumber() const { return _partNumber; }
  char fwMajor() const { return _fwMajor; }
  char fwMinor() const { return _fwMinor; }
  uint16_t patchId() const { return _patchId; }
  char componentMajor() const { return _componentMajor; }
  char componentMinor() const { return _componentMinor; }
  char chipRevision() const { return _chipRevision; }

private:
  SI4735 _rx;
  uint8_t _address = 0;
  uint16_t _frequency = 0;
  uint16_t _requestedFrequency = SI4732_FM_START_10KHZ;
  uint8_t _rssi = 0;
  uint8_t _snr = 0;
  bool _validChannel = false;
  uint8_t _partNumber = 0;
  char _fwMajor = '-';
  char _fwMinor = '-';
  uint16_t _patchId = 0;
  char _componentMajor = '-';
  char _componentMinor = '-';
  char _chipRevision = '-';
  bool _ready = false;
};
