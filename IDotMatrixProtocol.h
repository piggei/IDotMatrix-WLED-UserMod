#pragma once

#include <cstddef>
#include <cstdint>

struct IDotMatrixClockSettings {
  uint8_t style = 0;
  bool use24Hour = false;
  bool showDate = false;
  uint8_t red = 255;
  uint8_t green = 255;
  uint8_t blue = 255;
};

struct IDotMatrixTextSettings {
  uint8_t glyphCount = 0;
  uint8_t glyphWidth = 0;
  uint8_t glyphHeight = 0;
  uint8_t glyphBytes = 0;
  uint8_t motionEffect = 0;
  uint8_t speed = 5;
  uint8_t colorMode = 1;
  uint8_t red = 255;
  uint8_t green = 255;
  uint8_t blue = 255;
  bool backgroundEnabled = false;
  uint8_t backgroundRed = 0;
  uint8_t backgroundGreen = 0;
  uint8_t backgroundBlue = 0;
};

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
  virtual void onClock(const IDotMatrixClockSettings& settings) = 0;
  virtual bool onTextBegin(const IDotMatrixTextSettings& settings) = 0;
  virtual void onTextGlyph(
    uint8_t index,
    const uint8_t* bitmap,
    size_t bitmapLength
  ) = 0;
  virtual void onTextComplete() = 0;
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
  bool processTextPayload(const uint8_t* data, size_t length);

private:
  static bool hasValidLength(const uint8_t* data, size_t length);
  static void makeCommandAck(uint8_t command, uint8_t subcommand, IDotMatrixReply& reply);

  IDotMatrixProtocolEvents& events_;
  uint8_t screenType_ = 0x01;
};
