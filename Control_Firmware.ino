/*
  RP2040 Nixie Controller (drop-in)
  - Seesaw STEMMA QT rotary encoder (I2C 0x36), button active-LOW
  - DS3231 RTC via RTClib
  - Sends display updates to Mega over UART using NixieUART (SYNC 0xA5 0x5A, VER 0x01, CRC8)
  - UI:
      * SHOW_TIME: displays HH _ MM _ SS (12/24 per setting)
      * MENU:      tube0 shows 1 or 2
          1 -> toggle 12/24h
          2 -> enter SET_TIME
      * SET_TIME:  edit H->M->S with encoder, short press = next field, long press = save & return to MENU
  - Enters MENU immediately when the button is PRESSED during SHOW_TIME
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_seesaw.h>
#include <RTClib.h>
#include "NixieUART.h"

// ---------------- Debug over USB Serial ----------------
#define DEBUG_RP2040   // comment out to silence logs
#ifdef DEBUG_RP2040
  #include <stdarg.h>
  #include <stdio.h>
  static void DBG(const char* fmt, ...) {
    char buf[192];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    Serial.print(buf);
  }
#else
  #define DBG(...) do{}while(0)
#endif
// -------------------------------------------------------

// ===== UART to Mega (Nixie board) =====
static NixieUART nix(Serial1);
static const uint32_t LINK_BAUD = 115200;
// If needed for your Feather wiring, set pins before begin():
// Serial1.setTX(8);
// Serial1.setRX(9);

// ===== I2C Peripherals =====
static Adafruit_seesaw ss;
static RTC_DS3231 rtc;

static const uint8_t SEESAW_ADDR    = 0x36;
static const uint8_t SEESAW_BTN_PIN = 24;   // seesaw on-board button pin

// ===== UI State =====
enum UiState : uint8_t { UI_SHOW_TIME=0, UI_MENU=1, UI_SET_TIME=2 };
static UiState ui = UI_SHOW_TIME;

// settings
static bool mode24h = true;
static uint8_t menuIndex = 1; // 1..2

// set-time buffer
static uint8_t editH=12, editM=0, editS=0;
static uint8_t editField = 0; // 0=H,1=M,2=S

// encoder / button raw
static int32_t enc_prev = 0;

// debounced button state (active-LOW on hardware)
static const uint16_t DEBOUNCE_MS   = 30;
static const uint16_t LONG_PRESS_MS = 1200;
static bool     raw_state      = true;    // true = released (HIGH)
static bool     debounced      = true;    // true = released (HIGH)
static uint32_t lastEdgeTimeMs = 0;
static uint32_t btnPressMs     = 0;

// refresh timers
static uint32_t lastClockPushMs = 0;
static const uint16_t CLOCK_PUSH_MS = 250; // push time to display ~4Hz

// ===== helpers =====
static inline void sendDigits8(const uint8_t d[8]) { nix.setTime8(d); }

static void showMenuScreen() {
  uint8_t d[8] = {10,10,10,10,10,10,10,10};
  d[0] = (menuIndex <= 9 ? menuIndex : 10);
  sendDigits8(d);
  DBG("[UI] MENU %u\r\n", menuIndex);
}

static void showTimeDigits(uint8_t h, uint8_t m, uint8_t s, bool is24) {
  // Layout: H H _ M M _ S S ; 10 == blank
  uint8_t d[8] = {10,10,10,10,10,10,10,10};
  uint8_t hh = h;
  if (!is24) { hh = h % 12; if (hh == 0) hh = 12; }
  d[0] = (hh / 10) ? (hh / 10) : 10;
  d[1] = (hh % 10);
  d[2] = 10;
  d[3] = (m / 10);
  d[4] = (m % 10);
  d[5] = 10;
  d[6] = (s / 10);
  d[7] = (s % 10);
  sendDigits8(d);
}

static void showEditTimeScreenBlink() {
  static uint32_t lastBlink = 0;
  static bool blink = false;
  uint32_t now = millis();
  if (now - lastBlink > 400) { blink = !blink; lastBlink = now; }

  uint8_t d[8] = {10,10,10,10,10,10,10,10};
  d[0] = (editH/10) ? (editH/10) : 10; d[1] = (editH%10);
  d[2] = 10;
  d[3] = (editM/10);                   d[4] = (editM%10);
  d[5] = 10;
  d[6] = (editS/10);                   d[7] = (editS%10);

  if (blink) {
    if (editField == 0) { d[0]=10; d[1]=10; }
    else if (editField == 1) { d[3]=10; d[4]=10; }
    else { d[6]=10; d[7]=10; }
  }
  sendDigits8(d);
}

// ===== UI transitions =====
static void enterMenu() {
  ui = UI_MENU;
  menuIndex = 1;
  DBG("[UI] -> MENU\r\n");
  showMenuScreen();
}
static void exitMenuToTime() {
  ui = UI_SHOW_TIME;
  DBG("[UI] MENU -> SHOW_TIME\r\n");
}
static void enterSetTime() {
  ui = UI_SET_TIME;
  DateTime now = rtc.now();
  editH = now.hour(); editM = now.minute(); editS = now.second();
  editField = 0;
  DBG("[UI] -> SET_TIME %02u:%02u:%02u\r\n", editH, editM, editS);
  showEditTimeScreenBlink();
}
static void saveSetTimeAndReturnToMenu() {
  rtc.adjust(DateTime(2025, 1, 1, editH, editM, editS)); // arbitrary date
  DBG("[RTC] Saved %02u:%02u:%02u\r\n", editH, editM, editS);
  ui = UI_MENU;
  showMenuScreen();
}

// ===== Inputs =====
static void onEncoderDelta(int step) {
  if (step == 0) return;
  DBG("[ENC] step=%d\r\n", step);

  switch (ui) {
    case UI_MENU:
      if (step > 0) { if (menuIndex < 2) menuIndex++; }
      else          { if (menuIndex > 1) menuIndex--; }
      showMenuScreen();
      break;

    case UI_SET_TIME:
      if (editField == 0) {
        int v = (int)editH + (step > 0 ? 1 : -1);
        if (v < 0) v = 23; if (v > 23) v = 0; editH = (uint8_t)v;
      } else if (editField == 1) {
        int v = (int)editM + (step > 0 ? 1 : -1);
        if (v < 0) v = 59; if (v > 59) v = 0; editM = (uint8_t)v;
      } else {
        int v = (int)editS + (step > 0 ? 1 : -1);
        if (v < 0) v = 59; if (v > 59) v = 0; editS = (uint8_t)v;
      }
      DBG("[EDIT] H=%02u M=%02u S=%02u field=%u\r\n", editH, editM, editS, editField);
      showEditTimeScreenBlink();
      break;

    case UI_SHOW_TIME:
    default: break;
  }
}

static void onButtonEvent(bool pressed) {
  uint32_t now = millis();

  if (pressed) {
    // PRESS edge
    btnPressMs = now;
    DBG("[BTN] PRESS\r\n");

    if (ui == UI_SHOW_TIME) {
      // Enter menu immediately when showing time
      enterMenu();
      return;
    }
    return;
  }

  // RELEASE edge
  uint32_t held = now - btnPressMs;
  DBG("[BTN] RELEASE held=%ums\r\n", held);

  if (held >= LONG_PRESS_MS) {
    if (ui == UI_SET_TIME) {
      saveSetTimeAndReturnToMenu();
    } else if (ui == UI_MENU) {
      exitMenuToTime();
    }
  } else {
    if (ui == UI_MENU) {
      if (menuIndex == 1) {
        mode24h = !mode24h;
        DBG("[UI] Toggle mode -> %s\r\n", mode24h ? "24h" : "12h");
        showMenuScreen();
      } else if (menuIndex == 2) {
        enterSetTime();
      }
    } else if (ui == UI_SET_TIME) {
      editField = (uint8_t)((editField + 1) % 3);
      DBG("[EDIT] Next field -> %u\r\n", editField);
      showEditTimeScreenBlink();
    }
  }
}

// Debounced polling for encoder + button
static void pollInputs() {
  // Encoder
  int32_t pos = ss.getEncoderPosition();
  int32_t delta = pos - enc_prev;
  if (delta != 0) {
    enc_prev = pos;
    onEncoderDelta((delta > 0) ? +1 : -1);
  }

  // Button (active-LOW on seesaw)
  uint32_t now = millis();
  bool rawPressed = (ss.digitalRead(SEESAW_BTN_PIN) == 0); // true if pressed (LOW)
  bool new_raw_state = !rawPressed; // true = released (HIGH), false = pressed (LOW)

  if (new_raw_state != raw_state) {      // raw edge detected
    raw_state = new_raw_state;
    lastEdgeTimeMs = now;                 // reset debounce timer
  }

  if ((now - lastEdgeTimeMs) >= DEBOUNCE_MS) {
    if (debounced != raw_state) {
      // debounced edge -> emit event with "pressed" bool
      debounced = raw_state;
      bool pressed = !debounced;          // debounced false -> pressed
      onButtonEvent(pressed);
    }
  }
}

// ===== Arduino setup/loop =====
void setup() {
#ifdef DEBUG_RP2040
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0 < 1000)) { delay(10); }
  DBG("\r\nRP2040 Nixie Controller (debug on)\r\n");
#endif

  // UART to Mega
  // Serial1.setTX(8); Serial1.setRX(9); // if your Feather variant needs explicit pins
  Serial1.begin(LINK_BAUD);
  DBG("[LINK] Serial1 @ %lu\r\n", (unsigned long)LINK_BAUD);

  // I2C
  Wire.begin();
  DBG("[I2C] Wire.begin()\r\n");

  // Seesaw
  if (!ss.begin(SEESAW_ADDR)) {
    DBG("[ERR] Seesaw not found at 0x%02X\r\n", SEESAW_ADDR);
  } else {
    ss.pinMode(SEESAW_BTN_PIN, INPUT_PULLUP);
    ss.enableEncoderInterrupt();
    enc_prev = ss.getEncoderPosition();

    // Initialize button states from hardware
    bool initialRawPressed = (ss.digitalRead(SEESAW_BTN_PIN) == 0);
    raw_state  = !initialRawPressed;   // true=released (HIGH)
    debounced  = raw_state;
    lastEdgeTimeMs = millis();

    DBG("[OK] Seesaw @0x%02X enc_init=%ld btn=%s\r\n",
        SEESAW_ADDR, (long)enc_prev, initialRawPressed?"PRESSED":"RELEASED");
  }

  // RTC
  if (!rtc.begin()) {
    DBG("[ERR] DS3231 not found\r\n");
  } else {
    if (rtc.lostPower()) {
      rtc.adjust(DateTime(2025,1,1,12,0,0));
      DBG("[RTC] Lost power -> set default\r\n");
    } else {
      DateTime n = rtc.now();
      DBG("[RTC] Now %02u:%02u:%02u\r\n", n.hour(), n.minute(), n.second());
    }
  }

  // Clear display at start
  uint8_t blanks[8]; for (uint8_t i=0;i<8;i++) blanks[i]=10;
  nix.setTime8(blanks);
  DBG("[DISP] Cleared\r\n");

  ui = UI_SHOW_TIME;
}

void loop() {
  // Inputs
  pollInputs();

  // State-driven display
  if (ui == UI_SHOW_TIME) {
    uint32_t nowMs = millis();
    if (nowMs - lastClockPushMs > CLOCK_PUSH_MS) {
      lastClockPushMs = nowMs;
      DateTime t = rtc.now();
      showTimeDigits(t.hour(), t.minute(), t.second(), mode24h);

      // log once per second
      static uint32_t lastLog = 0;
      if (nowMs - lastLog > 1000) {
        lastLog = nowMs;
        DBG("[DISP] %s %02u:%02u:%02u\r\n",
            mode24h ? "24h" : "12h", t.hour(), t.minute(), t.second());
      }
    }
  } else if (ui == UI_MENU) {
    // maintain menu indicator
    showMenuScreen();
    delay(50);
  } else if (ui == UI_SET_TIME) {
    showEditTimeScreenBlink();
    delay(50);
  }
}
