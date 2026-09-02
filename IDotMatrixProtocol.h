#pragma once

#include <cstddef>
#include <cstdint>

class IDotMatrixProtocolEvents {
public:
  virtual ~IDotMatrixProtocolEvents() = default;
  virtual void onScreenPower(bool on) = 0;
  virtual void onBrightnessPercent(uint8_t percent) = 0;
  virtual void onSolidColor(uint8_t red, uint8_t green, uint8_t blue) = 0;
  virtual void onGraffitiMode(bool enter) = 0;
  virtual void onGraffitiPixels(
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    const uint8_t* coordinates,
    size_t coordinateBytes
  ) = 0;
};

struct IDotMatrixReply {
  static constexpr size_t MAX_SIZE = 16;

  uint8_t data[MAX_SIZE]{};
  uint8_t length = 0;

  bool available() const { return length != 0; }
};

class IDotMatrixProtocol {
public:
  explicit IDotMatrixProtocol(IDotMatrixProtocolEvents& events) : events_(events) {}

  void setScreenType(uint8_t screenType);
  void onConnected();
  void makeDeviceInfoReply(IDotMatrixReply& reply) const;
  bool processFA02(const uint8_t* data, size_t length, IDotMatrixReply& reply);

private:
  static bool hasValidLength(const uint8_t* data, size_t length);
  static void makeCommandAck(uint8_t command, uint8_t subcommand, IDotMatrixReply& reply);

  IDotMatrixProtocolEvents& events_;
  uint8_t screenType_ = 0x01;
};
