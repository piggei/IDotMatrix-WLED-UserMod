#include "../IDotMatrixProtocol.h"

#include <cassert>
#include <cstring>

class TestEvents final : public IDotMatrixProtocolEvents {
public:
  void onScreenPower(bool on) override {
    screenEventReceived = true;
    screenOn = on;
  }

  void onBrightnessPercent(uint8_t percent) override {
    brightnessEventReceived = true;
    brightnessPercent = percent;
  }

  void onSolidColor(uint8_t red, uint8_t green, uint8_t blue) override {
    colorEventReceived = true;
    colorRed = red;
    colorGreen = green;
    colorBlue = blue;
  }

  void onGraffitiMode(bool enter) override {
    graffitiModeEventReceived = true;
    graffitiModeEntered = enter;
  }

  void onGraffitiPixels(
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    const uint8_t* coordinates,
    size_t coordinateBytes
  ) override {
    graffitiPixelsEventReceived = true;
    graffitiRed = red;
    graffitiGreen = green;
    graffitiBlue = blue;
    graffitiCoordinateBytes = coordinateBytes;
    assert(coordinateBytes <= sizeof(graffitiCoordinates));
    memcpy(graffitiCoordinates, coordinates, coordinateBytes);
  }

  void onClock(const IDotMatrixClockSettings& settings) override {
    clockEventReceived = true;
    clockSettings = settings;
  }

  bool onTextBegin(const IDotMatrixTextSettings& settings) override {
    textBeginReceived = true;
    textSettings = settings;
    return true;
  }

  void onTextGlyph(
    uint8_t index,
    const uint8_t* bitmap,
    size_t bitmapLength
  ) override {
    ++textGlyphsReceived;
    textLastGlyph = index;
    textLastBitmapLength = bitmapLength;
    assert(bitmapLength <= sizeof(textLastBitmap));
    memcpy(textLastBitmap, bitmap, bitmapLength);
  }

  void onTextComplete() override { textCompleteReceived = true; }

  bool screenEventReceived = false;
  bool screenOn = false;
  bool brightnessEventReceived = false;
  uint8_t brightnessPercent = 0;
  bool colorEventReceived = false;
  uint8_t colorRed = 0;
  uint8_t colorGreen = 0;
  uint8_t colorBlue = 0;
  bool graffitiModeEventReceived = false;
  bool graffitiModeEntered = false;
  bool graffitiPixelsEventReceived = false;
  uint8_t graffitiRed = 0;
  uint8_t graffitiGreen = 0;
  uint8_t graffitiBlue = 0;
  uint8_t graffitiCoordinates[16]{};
  size_t graffitiCoordinateBytes = 0;
  bool clockEventReceived = false;
  IDotMatrixClockSettings clockSettings{};
  bool textBeginReceived = false;
  bool textCompleteReceived = false;
  uint8_t textGlyphsReceived = 0;
  uint8_t textLastGlyph = 0;
  size_t textLastBitmapLength = 0;
  uint8_t textLastBitmap[64]{};
  IDotMatrixTextSettings textSettings{};
};

static void expectReply(
  const IDotMatrixReply& reply,
  const uint8_t* expected,
  size_t expectedLength
) {
  assert(reply.length == expectedLength);
  assert(memcmp(reply.data, expected, expectedLength) == 0);
}

int main() {
  TestEvents events;
  IDotMatrixProtocol protocol(events);
  IDotMatrixReply reply;

  const uint8_t deviceInfoRequest[] = {0x04, 0x00, 0x01, 0x80};
  const uint8_t deviceInfo16[] = {0x09, 0x00, 0x01, 0x80, 0x04, 0x0E, 0x01, 0x01, 0x00};
  assert(protocol.processFA02(deviceInfoRequest, sizeof(deviceInfoRequest), reply));
  expectReply(reply, deviceInfo16, sizeof(deviceInfo16));

  protocol.setScreenType(0x04);
  const uint8_t deviceInfo64[] = {0x09, 0x00, 0x01, 0x80, 0x04, 0x0E, 0x01, 0x04, 0x00};
  protocol.makeDeviceInfoReply(reply);
  expectReply(reply, deviceInfo64, sizeof(deviceInfo64));

  events.screenEventReceived = false;
  events.screenOn = false;
  protocol.onConnected();
  assert(events.screenEventReceived && events.screenOn);

  const uint8_t screenOff[] = {0x05, 0x00, 0x07, 0x01, 0x00};
  const uint8_t screenAck[] = {0x05, 0x00, 0x07, 0x01, 0x01};
  assert(protocol.processFA02(screenOff, sizeof(screenOff), reply));
  assert(events.screenEventReceived && !events.screenOn);
  expectReply(reply, screenAck, sizeof(screenAck));

  events.screenEventReceived = false;
  const uint8_t screenOn[] = {0x05, 0x00, 0x07, 0x01, 0x7F};
  assert(protocol.processFA02(screenOn, sizeof(screenOn), reply));
  assert(events.screenEventReceived && events.screenOn);
  expectReply(reply, screenAck, sizeof(screenAck));

  const uint8_t malformedLength[] = {0x06, 0x00, 0x07, 0x01, 0x01};
  events.screenEventReceived = false;
  assert(!protocol.processFA02(malformedLength, sizeof(malformedLength), reply));
  assert(!events.screenEventReceived && !reply.available());

  const uint8_t unknown[] = {0x04, 0x00, 0x7F, 0x7F};
  assert(!protocol.processFA02(unknown, sizeof(unknown), reply));
  assert(!reply.available());

  const uint8_t brightness50[] = {0x05, 0x00, 0x04, 0x80, 0x32};
  const uint8_t brightnessAck[] = {0x05, 0x00, 0x04, 0x80, 0x01};
  assert(protocol.processFA02(brightness50, sizeof(brightness50), reply));
  assert(events.brightnessEventReceived && events.brightnessPercent == 50);
  expectReply(reply, brightnessAck, sizeof(brightnessAck));

  events.brightnessEventReceived = false;
  const uint8_t brightnessOverRange[] = {0x05, 0x00, 0x04, 0x80, 0xFF};
  assert(protocol.processFA02(brightnessOverRange, sizeof(brightnessOverRange), reply));
  assert(events.brightnessEventReceived && events.brightnessPercent == 100);
  expectReply(reply, brightnessAck, sizeof(brightnessAck));

  const uint8_t solidColor[] = {0x07, 0x00, 0x02, 0x02, 0x12, 0x34, 0x56};
  const uint8_t solidColorAck[] = {0x05, 0x00, 0x02, 0x02, 0x01};
  assert(protocol.processFA02(solidColor, sizeof(solidColor), reply));
  assert(events.colorEventReceived);
  assert(events.colorRed == 0x12 && events.colorGreen == 0x34 && events.colorBlue == 0x56);
  expectReply(reply, solidColorAck, sizeof(solidColorAck));

  const uint8_t clockCommand[] = {
    0x08, 0x00, 0x06, 0x01, 0xC5, 0x12, 0x34, 0x56
  };
  const uint8_t clockAck[] = {0x05, 0x00, 0x06, 0x01, 0x01};
  assert(protocol.processFA02(clockCommand, sizeof(clockCommand), reply));
  assert(events.clockEventReceived);
  assert(events.clockSettings.style == 5);
  assert(events.clockSettings.use24Hour);
  assert(events.clockSettings.showDate);
  assert(events.clockSettings.red == 0x12);
  assert(events.clockSettings.green == 0x34);
  assert(events.clockSettings.blue == 0x56);
  expectReply(reply, clockAck, sizeof(clockAck));

  const uint8_t enterDiy[] = {0x05, 0x00, 0x04, 0x01, 0x01};
  const uint8_t diyAck[] = {0x05, 0x00, 0x04, 0x01, 0x01};
  assert(protocol.processFA02(enterDiy, sizeof(enterDiy), reply));
  assert(events.graffitiModeEventReceived && events.graffitiModeEntered);
  expectReply(reply, diyAck, sizeof(diyAck));

  events.graffitiModeEventReceived = false;
  const uint8_t leaveDiy[] = {0x05, 0x00, 0x04, 0x01, 0x00};
  assert(protocol.processFA02(leaveDiy, sizeof(leaveDiy), reply));
  assert(events.graffitiModeEventReceived && !events.graffitiModeEntered);
  expectReply(reply, diyAck, sizeof(diyAck));

  // Byte 4 is intentionally opaque. Complete coordinate pairs start at byte 8;
  // an unmatched trailing byte is retained in the event but ignored by the adapter.
  const uint8_t graffiti[] = {
    0x0D, 0x00, 0x05, 0x01, 0xA5, 0x12, 0x34, 0x56,
    0x01, 0x02, 0x0F, 0x0E, 0xEE
  };
  assert(protocol.processFA02(graffiti, sizeof(graffiti), reply));
  assert(!reply.available());
  assert(events.graffitiPixelsEventReceived);
  assert(events.graffitiRed == 0x12);
  assert(events.graffitiGreen == 0x34);
  assert(events.graffitiBlue == 0x56);
  assert(events.graffitiCoordinateBytes == 5);
  assert(memcmp(events.graffitiCoordinates, graffiti + 8, 5) == 0);

  uint8_t textPayload[54]{};
  textPayload[0] = 2;
  textPayload[4] = 1;
  textPayload[5] = 50;
  textPayload[6] = 1;
  textPayload[7] = 0x12;
  textPayload[8] = 0x34;
  textPayload[9] = 0x56;
  textPayload[10] = 1;
  textPayload[11] = 1;
  textPayload[12] = 2;
  textPayload[13] = 3;
  textPayload[14] = 0x02;
  textPayload[18] = 0x01;
  textPayload[34] = 0x02;
  textPayload[38] = 0x80;
  assert(protocol.processTextPayload(textPayload, sizeof(textPayload)));
  assert(events.textBeginReceived && events.textCompleteReceived);
  assert(events.textSettings.glyphCount == 2);
  assert(events.textSettings.glyphWidth == 8);
  assert(events.textSettings.glyphHeight == 16);
  assert(events.textSettings.glyphBytes == 16);
  assert(events.textSettings.motionEffect == 1);
  assert(events.textSettings.speed == 50);
  assert(events.textSettings.red == 0x12);
  assert(events.textSettings.backgroundEnabled);
  assert(events.textGlyphsReceived == 2 && events.textLastGlyph == 1);
  assert(events.textLastBitmapLength == 16 && events.textLastBitmap[0] == 0x80);

  textPayload[14] = 0x03;
  assert(!protocol.processTextPayload(textPayload, sizeof(textPayload)));

  uint8_t largeGlyphPayload[82]{};
  largeGlyphPayload[0] = 1;
  largeGlyphPayload[6] = 1;
  largeGlyphPayload[7] = 0xAA;
  largeGlyphPayload[14] = 0x05;
  largeGlyphPayload[18] = 0x01;
  largeGlyphPayload[81] = 0x80;
  assert(protocol.processTextPayload(largeGlyphPayload, sizeof(largeGlyphPayload)));
  assert(events.textSettings.glyphCount == 1);
  assert(events.textSettings.glyphWidth == 16);
  assert(events.textSettings.glyphHeight == 32);
  assert(events.textSettings.glyphBytes == 64);
  assert(events.textLastBitmapLength == 64);
  assert(events.textLastBitmap[0] == 0x01);
  assert(events.textLastBitmap[63] == 0x80);
}
