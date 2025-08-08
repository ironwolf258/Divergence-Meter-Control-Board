#pragma once
#define DEBUG_USB   // uncomment to enable logs

#ifdef DEBUG_USB
  #include <Arduino.h>
  #include <stdarg.h>
  #include <stdio.h>
  static inline void DBG(const char* fmt, ...) {
    char buf[192];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    Serial.print(buf);
  }
#else
  #define DBG(...) do{}while(0)
#endif
