#pragma once
#include <RTClib.h>
#include "Debug.h"

struct TimeHMS { uint8_t h,m,s; };

class RtcClock {
public:
  bool begin();
  TimeHMS now() ;
  void set(const TimeHMS& t); // 24h values

private:
  RTC_DS3231 _rtc;
};
