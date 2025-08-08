#pragma once
#include <Adafruit_seesaw.h>
#include "Debug.h"

class Encoder {
public:
  bool begin(uint8_t addr=0x36, uint8_t btnPin=24);
  void poll();

  // returns -1, 0, +1 per poll tick (coalesced)
  int8_t step();

  // button edges
  bool pressedEdge();
  bool releasedEdge();

private:
  Adafruit_seesaw _ss;
  uint8_t _addr = 0x36, _btnPin = 24;

  // encoder
  int32_t _posPrev = 0;
  int8_t  _stepOut = 0;

  // debounce
  static constexpr uint16_t DEBOUNCE_MS = 30;
  bool _raw = true, _deb = true; // true = released (HIGH)
  uint32_t _edgeMs = 0;

  bool _pressEdge = false, _relEdge = false;
};
