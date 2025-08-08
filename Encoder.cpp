#include "Encoder.h"
#include <Wire.h>

bool Encoder::begin(uint8_t addr, uint8_t btnPin) {
  _addr = addr; _btnPin = btnPin;
  if (!_ss.begin(_addr)) { DBG("[ENC] seesaw not found\r\n"); return false; }
  _ss.pinMode(_btnPin, INPUT_PULLUP);
  _ss.enableEncoderInterrupt();
  _posPrev = _ss.getEncoderPosition();
  _raw = _deb = (_ss.digitalRead(_btnPin) != 0);
  _edgeMs = millis();
  DBG("[ENC] ok addr=0x%02X pos=%ld btn=%s\r\n", _addr, (long)_posPrev, _deb?"HIGH":"LOW");
  return true;
}

void Encoder::poll() {
  // encoder
  int32_t p = _ss.getEncoderPosition();
  int32_t d = p - _posPrev;
  if (d != 0) { _posPrev = p; _stepOut = (d > 0) ? +1 : -1; }
  else _stepOut = 0;

  // button (active LOW)
  bool rawPressed = (_ss.digitalRead(_btnPin) == 0);
  bool rawStateReleased = !rawPressed;
  uint32_t now = millis();

  if (rawStateReleased != _raw) { _raw = rawStateReleased; _edgeMs = now; }
  _pressEdge = _relEdge = false;

  if ((now - _edgeMs) >= DEBOUNCE_MS) {
    if (_deb != _raw) {
      _deb = _raw;
      bool pressed = !_deb;
      _pressEdge = pressed;
      _relEdge   = !pressed;
    }
  }
}

int8_t Encoder::step() { return _stepOut; }
bool   Encoder::pressedEdge()  { return _pressEdge; }
bool   Encoder::releasedEdge() { return _relEdge;  }
