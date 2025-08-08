#include "Display.h"

void Display::begin(uint32_t baud) { ((HardwareSerial&) _nix).begin(baud); /* no-op for wrapper */ }
void Display::clear() { uint8_t b[8]; for (uint8_t i=0;i<8;i++) b[i]=10; _nix.setTime8(b); }
void Display::showDigits8(const uint8_t d[8]) { _nix.setTime8(d); }

void Display::showMenuIndex(uint8_t idx) {
  uint8_t d[8]={10,10,10,10,10,10,10,10};
  d[0] = (idx<=9)?idx:10;
  _nix.setTime8(d);
}

void Display::showTimeDigits(uint8_t h, uint8_t m, uint8_t s, bool mode24) {
  uint8_t d[8]={10,10,10,10,10,10,10,10};
  uint8_t hh=h; if(!mode24){ hh=h%12; if(!hh) hh=12; }
  d[0]=(hh/10)?(hh/10):10; d[1]=hh%10;
  d[3]=m/10; d[4]=m%10;
  d[6]=s/10; d[7]=s%10;
  _nix.setTime8(d);
}

void Display::showEditTimeBlink(uint8_t h, uint8_t m, uint8_t s, uint8_t field) {
  static uint32_t last=0; static bool on=false;
  uint32_t now=millis(); if(now-last>400){ on=!on; last=now; }
  uint8_t d[8]={10,10,10,10,10,10,10,10};
  d[0]=(h/10)?(h/10):10; d[1]=h%10;
  d[3]=m/10; d[4]=m%10;
  d[6]=s/10; d[7]=s%10;
  if(on){
    if(field==0){ d[0]=10; d[1]=10; }
    else if(field==1){ d[3]=10; d[4]=10; }
    else { d[6]=10; d[7]=10; }
  }
  _nix.setTime8(d);
}
