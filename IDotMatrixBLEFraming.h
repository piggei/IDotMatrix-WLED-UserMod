#pragma once
#include <cstddef>
#include <cstdint>

inline bool idotStartsAudioFrame(const uint8_t* data, size_t length) {
  if (data == nullptr || length < 4) return false;
  return (data[0] == 0x06 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x02) ||
    (data[0] == 0x21 && data[1] == 0x00 && data[2] == 0x01 && data[3] == 0x02);
}

inline bool idotStartsKnownNonAudioFrame(const uint8_t* data, size_t length) {
  if (data == nullptr || length < 4) return false;
  const uint16_t declared = uint16_t(data[0]) | (uint16_t(data[1]) << 8);
  if (declared < 4 || declared > 8192) return false;
  const uint8_t command = data[2], subcommand = data[3];
  // Bulk begin/chunk/end envelopes and compact inline PNG.
  if (subcommand == 0x00 && (command == 0x01 || command == 0x02 || command == 0x03)) return true;
  if (command == 0x00 && subcommand == 0x00) return true;
  // Captured normal FA02 command families, including protocol reset and
  // Carousel control that must remain routable after an audio session.
  return (command == 0x01 && subcommand == 0x80) ||
    (command == 0x03 && subcommand == 0x80) ||
    (command == 0x02 && subcommand == 0x01) ||
    (command == 0x0A && subcommand == 0x01) ||
    (command == 0x00 && subcommand == 0x80) ||
    (command == 0x07 && (subcommand == 0x80 || subcommand == 0x01)) ||
    (command == 0x05 && (subcommand == 0x80 || subcommand == 0x01)) ||
    (command == 0x04 && (subcommand == 0x80 || subcommand == 0x01)) ||
    (command == 0x02 && subcommand == 0x02) ||
    (command == 0x03 && subcommand == 0x02) ||
    (command == 0x06 && subcommand == 0x01) ||
    (command == 0x08 && subcommand == 0x80) ||
    (command == 0x09 && subcommand == 0x80) ||
    (command == 0x0A && subcommand == 0x80);
}
