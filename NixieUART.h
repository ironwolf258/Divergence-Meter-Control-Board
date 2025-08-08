#pragma once
#include <Arduino.h>

class NixieUART {
public:
  enum : uint8_t {
    SYNC0 = 0xA5,
    SYNC1 = 0x5A,
    VER   = 0x01,

    CMD_SET_DIGIT = 0x01,
    CMD_SET_MULTI = 0x02,
    // 0x03 reserved (dots, if you add later)
    CMD_SET_TIME8 = 0x04,

    CMD_PING = 0x20,
    CMD_PONG = 0x21, // device -> host
    CMD_MODE = 0x30  // 0 = normal, 1 = blank
  };

  explicit NixieUART(Stream& s) : io(s) {}

  // ===== High-level API matching the firmware =====
  // Set a single tube to a digit (0..9, 10=blank)
  bool setDigit(uint8_t tubeIndex, uint8_t digit);

  // Set multiple tubes at once:
  //  - 'count' pairs; tubes[i] is tube index, digits[i] is digit for that tube
  bool setMulti(const uint8_t* tubes, const uint8_t* digits, uint8_t count);

  // Set all eight tubes in one shot
  bool setTime8(const uint8_t digits[8]);

  // Send a ping (device will respond with PONG carrying the same nonce)
  bool ping(uint8_t nonce = 0x42);

  // 0 = normal, 1 = blank
  bool setMode(uint8_t mode);

private:
  Stream& io;

  bool sendPacket(uint8_t cmd, const uint8_t* payload, uint8_t payloadLen);
  static uint8_t crc8_maxim(const uint8_t* data, uint8_t len);
};
