#include "IDotMatrixProtocol.h"

#include <cstring>

void IDotMatrixProtocol::setScreenType(uint8_t screenType) {
  screenType_ = (screenType == 0x01 || screenType == 0x03 || screenType == 0x04)
    ? screenType
    : 0x01;
}

void IDotMatrixProtocol::onConnected() {
  // The verified standalone emulator treats a new app connection as screen ON.
  events_.onScreenPower(true);
}

void IDotMatrixProtocol::makeDeviceInfoReply(IDotMatrixReply& reply) const {
  const uint8_t response[] = {
    0x09, 0x00, 0x01, 0x80, 0x04, 0x0E, 0x01, screenType_, 0x00
  };
  memcpy(reply.data, response, sizeof(response));
  reply.length = sizeof(response);
}

bool IDotMatrixProtocol::processFA02(
  const uint8_t* data,
  size_t length,
  IDotMatrixReply& reply
) {
  reply.length = 0;
  if (!hasValidLength(data, length) || length < 4) return false;

  const uint8_t command = data[2];
  const uint8_t subcommand = data[3];

  // Device information request.
  if (length == 4 && command == 0x01 && subcommand == 0x80) {
    makeDeviceInfoReply(reply);
    return true;
  }

  // App time synchronization. Time storage/rendering will be added later.
  if (length == 11 && command == 0x01 && subcommand == 0x80) {
    makeCommandAck(command, subcommand, reply);
    return true;
  }

  // Confirmed matrix power command: 05 00 07 01 STATE.
  if (length == 5 && command == 0x07 && subcommand == 0x01) {
    events_.onScreenPower(data[4] != 0x00);
    makeCommandAck(command, subcommand, reply);
    return true;
  }

  // Confirmed brightness command: 05 00 04 80 PERCENT (clamped to 0..100).
  if (length == 5 && command == 0x04 && subcommand == 0x80) {
    const uint8_t percent = data[4] > 100 ? 100 : data[4];
    events_.onBrightnessPercent(percent);
    makeCommandAck(command, subcommand, reply);
    return true;
  }

  // Confirmed full-screen RGB command: 07 00 02 02 R G B.
  if (length == 7 && command == 0x02 && subcommand == 0x02) {
    events_.onSolidColor(data[4], data[5], data[6]);
    makeCommandAck(command, subcommand, reply);
    return true;
  }

  // Confirmed DIY mode command: 05 00 04 01 STATE.
  if (length == 5 && command == 0x04 && subcommand == 0x01) {
    events_.onGraffitiMode(data[4] != 0x00);
    makeCommandAck(command, subcommand, reply);
    return true;
  }

  // Confirmed graffiti update:
  // LENlo LENhi 05 01 UNKNOWN R G B X0 Y0 X1 Y1 ...
  // The reference accepts complete coordinate pairs and ignores an unmatched
  // trailing byte. Pixel updates intentionally have no FA03 acknowledgement.
  if (length >= 10 && command == 0x05 && subcommand == 0x01) {
    events_.onGraffitiPixels(data[5], data[6], data[7], data + 8, length - 8);
    return true;
  }

  return false;
}

bool IDotMatrixProtocol::hasValidLength(const uint8_t* data, size_t length) {
  if (data == nullptr || length < 2 || length > 0xFFFFu) return false;
  const uint16_t declaredLength = uint16_t(data[0]) | (uint16_t(data[1]) << 8);
  return declaredLength == length;
}

void IDotMatrixProtocol::makeCommandAck(
  uint8_t command,
  uint8_t subcommand,
  IDotMatrixReply& reply
) {
  const uint8_t response[] = {0x05, 0x00, command, subcommand, 0x01};
  memcpy(reply.data, response, sizeof(response));
  reply.length = sizeof(response);
}
