#include <Arduino.h>
#include <Wire.h>
#include "Debug.h"
#include "Encoder.h"
#include "RtcClock.h"
#include "Display.h"
#include "Menu.h"
#include "NixieUART.h"

// UART to Mega (pins optional)
// Serial1.setTX(8); Serial1.setRX(9);
static NixieUART nix(Serial1);
static Display   display(Serial1); // wraps nix internally
static Encoder   enc;
static RtcClock  rtc;
static Menu      menu(display, rtc);

void setup() {
#ifdef DEBUG_USB
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis()-t0 < 1000) { delay(10); }
  DBG("\r\nRP2040 Nixie Controller (modular)\r\n");
#endif

  // Link to Mega
  Serial1.begin(115200);
  display.clear();

  // I2C
  Wire.begin();

  // Peripherals
  enc.begin(0x36, 24);
  rtc.begin();

  // UI
  menu.begin(true); // start 24h
}

void loop() {
  menu.update(enc);
  menu.render();
}
