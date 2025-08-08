#include "NixieUART.h"

// On Feather RP2040, use Serial1 for the UART link to the tube board
// If your board needs explicit pin selection, uncomment and set below in setup():
//   Serial1.setTX(8);
//   Serial1.setRX(9);

NixieUART nix(Serial1);

void setup() {
  Serial.begin(115200);
  delay(100);

  // Start hardware UART to the tube control board
  Serial1.begin(115200);
  Serial.println("Nixie UART demo (protocol-aligned)");

  // Optional: quick link test
  nix.ping(0x7A);

  // 1) Sanity check: blank all 8 tubes (digit=10)
  uint8_t blanks[8];
  for (uint8_t i = 0; i < 8; i++) blanks[i] = 10;
  nix.setTime8(blanks);

  // 2) Set tubes 0, 2, 4, 6 with one packet (SET_MULTI)
  //    (example digits 1,2,3,4 on those tubes)
  const uint8_t tubes[]  = {0, 2, 4, 6};
  const uint8_t digits[] = {1, 2, 3, 4};
  nix.setMulti(tubes, digits, 4);

  // 3) Show a time across 8 tubes: HH MM SS with blanks for separators
  //    Example: 21:07:30 -> [2,1,blank,0,7,blank,3,0]
  uint8_t timeDigits[8] = {2, 1, 10, 0, 7, 10, 3, 0};
  nix.setTime8(timeDigits);

  // 4) Make sure we’re in normal (not-blanked) mode
  nix.setMode(0); // 0=normal, 1=blank
}

void loop() {
  // Spin a little counter on the rightmost tube using single-tube calls
  static uint32_t last = 0;
  static uint8_t v = 0;
  if (millis() - last > 1000) {
    last = millis();
    v = (uint8_t)((v + 1) % 10);
    nix.setDigit(7, v);  // tube index 7 shows v (0–9)
  }
}
