#include "RtcClock.h"

bool RtcClock::begin() {
  if (!_rtc.begin()) { DBG("[RTC] not found\r\n"); return false; }
  if (_rtc.lostPower()) {
    _rtc.adjust(DateTime(2025,1,1,12,0,0));
    DBG("[RTC] lost power -> default\r\n");
  }
  DateTime n = _rtc.now();
  DBG("[RTC] %02u:%02u:%02u\r\n", n.hour(), n.minute(), n.second());
  return true;
}

TimeHMS RtcClock::now()  {
  DateTime n = _rtc.now();
  return TimeHMS{ (uint8_t)n.hour(), (uint8_t)n.minute(), (uint8_t)n.second() };
}

void RtcClock::set(const TimeHMS& t) {
  _rtc.adjust(DateTime(2025,1,1,t.h,t.m,t.s)); // date arbitrary
  DBG("[RTC] set %02u:%02u:%02u\r\n", t.h,t.m,t.s);
}
