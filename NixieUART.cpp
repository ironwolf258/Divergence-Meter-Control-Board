#include "NixieUART.h"

static inline void writeU8(Stream& io, uint8_t v) { io.write(&v, 1); }

/* Dallas/Maxim CRC-8 (poly 0x31, init 0x00), MSB-first — matches tube firmware */
uint8_t NixieUART::crc8_maxim(const uint8_t* data, uint8_t len) {
  uint8_t crc = 0x00;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) {
      if (crc & 0x80) crc = (uint8_t)((crc << 1) ^ 0x31);
      else            crc <<= 1;
    }
  }
  return crc;
}

bool NixieUART::sendPacket(uint8_t cmd, const uint8_t* payload, uint8_t payloadLen) {
  // LEN = bytes from CMD through last PAYLOAD byte (CRC not included)
  const uint8_t len = (uint8_t)(1 /*CMD*/ + payloadLen);

  // Build header/body for CRC: [VER][LEN][CMD][PAYLOAD...]
  uint8_t buf[1 + 1 + 1 + 32]; // VER + LEN + CMD + payload (payload <= 32 here)
  buf[0] = VER;
  buf[1] = len;
  buf[2] = cmd;
  for (uint8_t i = 0; i < payloadLen; i++) buf[3 + i] = payload ? payload[i] : 0;

  const uint8_t crc = crc8_maxim(buf, (uint8_t)(3 + payloadLen));

  // Write frame: [SYNC0][SYNC1][VER][LEN][CMD][PAYLOAD...][CRC]
  writeU8(io, SYNC0);
  writeU8(io, SYNC1);
  io.write(buf, 3 + payloadLen);
  writeU8(io, crc);
  io.flush();
  return true;
}

/* ====== Public API ====== */

bool NixieUART::setDigit(uint8_t tubeIndex, uint8_t digit) {
  if (tubeIndex > 7 || digit > 10) return false;
  uint8_t p[2] = { tubeIndex, digit };
  return sendPacket(CMD_SET_DIGIT, p, 2);
}

bool NixieUART::setMulti(const uint8_t* tubes, const uint8_t* digits, uint8_t count) {
  if (!tubes || !digits || count == 0 || count > 8) return false;
  // Payload: [count][(tube,digit)*count]
  uint8_t payload[1 + 8*2];
  payload[0] = count;
  for (uint8_t i = 0; i < count; i++) {
    payload[1 + i*2 + 0] = tubes[i];
    payload[1 + i*2 + 1] = digits[i];
  }
  return sendPacket(CMD_SET_MULTI, payload, (uint8_t)(1 + count*2));
}

bool NixieUART::setTime8(const uint8_t d[8]) {
  // Payload: [d0..d7]; each 0..10 (10 = blank)
  return sendPacket(CMD_SET_TIME8, d, 8);
}

bool NixieUART::ping(uint8_t nonce) {
  return sendPacket(CMD_PING, &nonce, 1);
}

bool NixieUART::setMode(uint8_t mode) {
  // 0 = normal, 1 = blank (firmware blanks immediately on 1)
  mode = (mode ? 1 : 0);
  return sendPacket(CMD_MODE, &mode, 1);
}
