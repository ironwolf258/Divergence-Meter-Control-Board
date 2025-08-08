#pragma once
#include "Encoder.h"
#include "RtcClock.h"
#include "Display.h"

class Menu {
public:
  enum State : uint8_t { SHOW_TIME=0, MENU=1, SET_TIME=2 };

  Menu(Display& disp, RtcClock& rtc) : _disp(disp), _rtc(rtc) {}

  void begin(bool mode24=true);
  void update(Encoder& enc);    // feed inputs
  void render();                // draw current screen

private:
  Display& _disp;
  RtcClock& _rtc;

  State  _state = SHOW_TIME;
  bool   _mode24 = true;
  uint8_t _menuIdx = 1; // 1..2

  // edit buffer
  uint8_t _eh=12,_em=0,_es=0,_field=0;

  // time push
  uint32_t _lastPush=0;
  static constexpr uint16_t PUSH_MS=250;

  void enterMenu();
  void exitToTime();
  void enterSetTime();
  void saveTimeAndBackToMenu();
};
