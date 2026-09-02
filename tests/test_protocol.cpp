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

  bool screenEventReceived = false;
  bool screenOn = false;
  bool brightnessEventReceived = false;
  uint8_t brightnessPercent = 0;
  bool colorEventReceived = false;
  uint8_t colorRed = 0;
  uint8_t colorGreen = 0;
  uint8_t colorBlue = 0;
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
}
