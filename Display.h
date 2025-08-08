#pragma once
#include "NixieUART.h"

class Display {
public:
  explicit Display(Stream& uart) : _nix(uart) {}
  void begin(uint32_t baud);
  void clear();
  void showDigits8(const uint8_t d[8]);
  void showMenuIndex(uint8_t idx);
  void showTimeDigits(uint8_t h, uint8_t m, uint8_t s, bool mode24);
  void showEditTimeBlink(uint8_t h, uint8_t m, uint8_t s, uint8_t field);

private:
  NixieUART _nix;
};
