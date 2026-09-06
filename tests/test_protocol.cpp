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

  void onLightEffect(const IDotMatrixLightEffectSettings& settings) override {
    lightEffectEventReceived = true;
    lightEffectSettings = settings;
  }

  void onAudio(const IDotMatrixAudioSettings& settings) override {
    audioEventReceived = true;
    audioSettings = settings;
    ++audioEventCount;
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

  void onCountdown(const IDotMatrixCountdownSettings& settings) override {
    countdownEventReceived = true;
    countdownSettings = settings;
  }

  void onStopwatch(uint8_t mode) override {
    stopwatchEventReceived = true;
    stopwatchMode = mode;
  }

  void onScoreboard(uint16_t scoreA, uint16_t scoreB) override {
    scoreboardEventReceived = true;
    scoreboardA = scoreA;
    scoreboardB = scoreB;
  }

  bool takeCountdownFinished() override {
    const bool ready = countdownFinishedPending;
    countdownFinishedPending = false;
    return ready;
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

  bool onRawImageBegin(size_t byteLength) override {
    rawBeginReceived = true;
    rawExpectedBytes = byteLength;
    return rawAccept;
  }

  bool onRawImageData(
    size_t offset,
    const uint8_t* data,
    size_t length
  ) override {
    rawDataReceived = true;
    rawOffset = offset;
    rawLength = length;
    if (data != nullptr && length > 0) rawFirstByte = data[0];
    return rawAccept;
  }

  bool onRawImageComplete(bool crcValid) override {
    rawCompleteReceived = true;
    rawCRCValid = crcValid;
    return rawAccept && crcValid;
  }

  bool onPngImage(const uint8_t* data, size_t length) override {
    pngReceived = data != nullptr;
    pngLength = length;
    return true;
  }

  bool onGifBegin(size_t byteLength) override {
    gifLength = byteLength;
    return true;
  }

  bool onGifData(size_t offset, const uint8_t* data, size_t length) override {
    gifOffset = offset;
    gifDataLength = length;
    return data != nullptr;
  }

  bool onGifComplete(bool crcValid) override {
    gifCRCValid = crcValid;
    return crcValid;
  }

  bool screenEventReceived = false;
  bool screenOn = false;
  bool brightnessEventReceived = false;
  uint8_t brightnessPercent = 0;
  bool colorEventReceived = false;
  uint8_t colorRed = 0;
  uint8_t colorGreen = 0;
  uint8_t colorBlue = 0;
  bool lightEffectEventReceived = false;
  IDotMatrixLightEffectSettings lightEffectSettings{};
  bool audioEventReceived = false;
  uint32_t audioEventCount = 0;
  IDotMatrixAudioSettings audioSettings{};
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
  bool countdownEventReceived = false;
  IDotMatrixCountdownSettings countdownSettings{};
  bool stopwatchEventReceived = false;
  uint8_t stopwatchMode = 0;
  bool scoreboardEventReceived = false;
  uint16_t scoreboardA = 0;
  uint16_t scoreboardB = 0;
  bool countdownFinishedPending = false;
  bool textBeginReceived = false;
  bool textCompleteReceived = false;
  uint8_t textGlyphsReceived = 0;
  uint8_t textLastGlyph = 0;
  size_t textLastBitmapLength = 0;
  uint8_t textLastBitmap[64]{};
  IDotMatrixTextSettings textSettings{};
  bool rawAccept = true;
  bool rawBeginReceived = false;
  bool rawDataReceived = false;
  bool rawCompleteReceived = false;
  bool rawCRCValid = false;
  size_t rawExpectedBytes = 0;
  size_t rawOffset = 0;
  size_t rawLength = 0;
  uint8_t rawFirstByte = 0;
  bool pngReceived = false;
  size_t pngLength = 0;
  size_t gifLength = 0;
  size_t gifOffset = 0;
  size_t gifDataLength = 0;
  bool gifCRCValid = false;
};


class TestAutomation final : public IDotMatrixAutomationEvents {
public:
  void onTimeSync(const IDotMatrixTimeSyncSettings& settings) override {
    timeReceived = true;
    time = settings;
  }

  bool onAlarm(
    const IDotMatrixAlarmSettings& settings,
    const uint8_t* media,
    size_t mediaLength
  ) override {
    alarmReceived = true;
    alarm = settings;
    alarmMediaLength = mediaLength;
    alarmFirstByte = mediaLength && media ? media[0] : 0;
    return alarmAccept;
  }

  void onScheduleGlobal(uint8_t flags) override {
    scheduleGlobalReceived = true;
    scheduleFlags = flags;
  }

  bool onScheduleActivity(
    const IDotMatrixScheduleActivitySettings& settings,
    const uint8_t* media,
    size_t mediaLength
  ) override {
    scheduleActivityReceived = true;
    schedule = settings;
    scheduleMediaLength = mediaLength;
    scheduleFirstByte = mediaLength && media ? media[0] : 0;
    return scheduleAccept;
  }

  bool timeReceived = false;
  IDotMatrixTimeSyncSettings time{};
  bool alarmReceived = false;
  bool alarmAccept = true;
  IDotMatrixAlarmSettings alarm{};
  size_t alarmMediaLength = 0;
  uint8_t alarmFirstByte = 0;
  bool scheduleGlobalReceived = false;
  uint8_t scheduleFlags = 0;
  bool scheduleActivityReceived = false;
  bool scheduleAccept = true;
  IDotMatrixScheduleActivitySettings schedule{};
  size_t scheduleMediaLength = 0;
  uint8_t scheduleFirstByte = 0;
};

static uint32_t testCRC32(const uint8_t* data, size_t length) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
    }
  }
  return crc ^ 0xFFFFFFFFu;
}

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
  TestAutomation automation;
  IDotMatrixProtocol protocol(events);
  protocol.setAutomationEvents(&automation);
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

  const uint8_t timeSync[] = {0x0B,0x00,0x01,0x80,0x1A,0x09,0x05,0x06,0x17,0x2A,0x0B};
  const uint8_t timeAck[] = {0x05,0x00,0x01,0x80,0x01};
  assert(protocol.processFA02(timeSync, sizeof(timeSync), reply));
  assert(automation.timeReceived);
  assert(automation.time.year == 2026 && automation.time.month == 9 && automation.time.day == 5);
  assert(automation.time.hour == 23 && automation.time.minute == 42 && automation.time.second == 11);
  expectReply(reply, timeAck, sizeof(timeAck));

  const uint8_t alarmShort[] = {0x0C,0x00,0x00,0x80,0x02,0x83,0x07,0x1E,0x0A,0x00,0x01,0x01};
  const uint8_t alarmAck[] = {0x05,0x00,0x00,0x80,0x01};
  assert(protocol.processFA02(alarmShort, sizeof(alarmShort), reply));
  assert(automation.alarmReceived && automation.alarm.slot == 2);
  assert(!automation.alarm.fullHeader && automation.alarm.flags == 0x83);
  assert(automation.alarm.contentType == 1 && automation.alarm.buzzer == 1);
  expectReply(reply, alarmAck, sizeof(alarmAck));

  uint8_t alarmFull[28] = {0};
  alarmFull[0] = sizeof(alarmFull); alarmFull[1] = 0; alarmFull[2] = 0x00; alarmFull[3] = 0x80;
  alarmFull[4] = 1; alarmFull[5] = 0x03; alarmFull[6] = 6; alarmFull[7] = 45; alarmFull[8] = 20;
  alarmFull[10] = 2; alarmFull[11] = 1;
  const uint8_t alarmMedia[] = {1,2,3,4};
  const uint32_t alarmCrc = testCRC32(alarmMedia, sizeof(alarmMedia));
  alarmFull[13] = sizeof(alarmMedia);
  alarmFull[17] = uint8_t(alarmCrc); alarmFull[18] = uint8_t(alarmCrc >> 8);
  alarmFull[19] = uint8_t(alarmCrc >> 16); alarmFull[20] = uint8_t(alarmCrc >> 24);
  alarmFull[23] = 0x14;
  memcpy(alarmFull + 24, alarmMedia, sizeof(alarmMedia));
  automation.alarmReceived = false;
  assert(protocol.processFA02(alarmFull, sizeof(alarmFull), reply));
  assert(automation.alarmReceived && automation.alarm.fullHeader);
  assert(automation.alarm.mediaSize == 4 && automation.alarmMediaLength == 4 && automation.alarmFirstByte == 1);
  expectReply(reply, alarmAck, sizeof(alarmAck));

  const uint8_t scheduleGlobal[] = {0x05,0x00,0x07,0x80,0x03};
  const uint8_t scheduleGlobalAck[] = {0x05,0x00,0x07,0x80,0x01};
  assert(protocol.processFA02(scheduleGlobal, sizeof(scheduleGlobal), reply));
  assert(automation.scheduleGlobalReceived && automation.scheduleFlags == 0x03);
  expectReply(reply, scheduleGlobalAck, sizeof(scheduleGlobalAck));

  uint8_t schedulePacket[27] = {0};
  schedulePacket[0] = sizeof(schedulePacket); schedulePacket[1] = 0;
  schedulePacket[2] = 0x05; schedulePacket[3] = 0x80; schedulePacket[4] = 3; schedulePacket[5] = 0xFF;
  schedulePacket[6] = 8; schedulePacket[7] = 30; schedulePacket[8] = 9; schedulePacket[9] = 15;
  schedulePacket[10] = 2; schedulePacket[11] = 0;
  const uint8_t scheduleMedia[] = {9,8,7,6};
  schedulePacket[12] = sizeof(scheduleMedia);
  const uint32_t scheduleCrc = testCRC32(scheduleMedia, sizeof(scheduleMedia));
  schedulePacket[16] = uint8_t(scheduleCrc); schedulePacket[17] = uint8_t(scheduleCrc >> 8);
  schedulePacket[18] = uint8_t(scheduleCrc >> 16); schedulePacket[19] = uint8_t(scheduleCrc >> 24);
  schedulePacket[22] = 0x1E;
  memcpy(schedulePacket + 23, scheduleMedia, sizeof(scheduleMedia));
  const uint8_t scheduleAck[] = {0x05,0x00,0x05,0x80,0x03};
  assert(protocol.processFA02(schedulePacket, sizeof(schedulePacket), reply));
  assert(automation.scheduleActivityReceived && automation.schedule.index == 3);
  assert(automation.schedule.contentType == 2 && automation.scheduleMediaLength == 4 && automation.scheduleFirstByte == 9);
  expectReply(reply, scheduleAck, sizeof(scheduleAck));

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

  const uint8_t lightEffect[] = {
    0x0D, 0x00, 0x03, 0x02, 0x06, 0x05, 0x02,
    0x7F, 0x00, 0x00,
    0x00, 0x40, 0x20
  };
  const uint8_t lightEffectAck[] = {0x05, 0x00, 0x03, 0x02, 0x01};
  assert(protocol.processFA02(lightEffect, sizeof(lightEffect), reply));
  assert(events.lightEffectEventReceived);
  assert(events.lightEffectSettings.effect == 6);
  assert(events.lightEffectSettings.speed == 5);
  assert(events.lightEffectSettings.colorCount == 2);
  assert(events.lightEffectSettings.colors[0].red == 255);
  assert(events.lightEffectSettings.colors[0].green == 0);
  assert(events.lightEffectSettings.colors[0].blue == 0);
  assert(events.lightEffectSettings.colors[1].red == 0);
  assert(events.lightEffectSettings.colors[1].green == 128);
  assert(events.lightEffectSettings.colors[1].blue == 64);
  expectReply(reply, lightEffectAck, sizeof(lightEffectAck));

  const uint8_t audioLevel[] = {0x06,0x00,0x00,0x02,0xFF,0x05};
  const uint8_t audioLevelAck[] = {0x05,0x00,0x00,0x02,0x01};
  assert(protocol.processAudioStream(audioLevel, sizeof(audioLevel), reply));
  assert(events.audioEventReceived && !events.audioSettings.fft);
  assert(events.audioSettings.mode == 4 && events.audioSettings.level == 12);
  expectReply(reply, audioLevelAck, sizeof(audioLevelAck));

  // FFT writes are a byte stream: one 33-byte BLE write commonly contains a
  // full 21-byte frame and 12 bytes of the next. The following write completes
  // that frame and starts another one.
  uint8_t fftStream[42]{};
  for (uint8_t frame = 0; frame < 2; ++frame) {
    const size_t base = size_t(frame) * 21u;
    fftStream[base] = 0x21; fftStream[base + 2] = 0x01;
    fftStream[base + 3] = 0x02; fftStream[base + 4] = frame;
    for (uint8_t i = 0; i < 16; ++i) fftStream[base + 5 + i] = uint8_t(i + frame);
  }
  const uint32_t beforeFFT = events.audioEventCount;
  assert(protocol.processAudioStream(fftStream, 33, reply));
  assert(events.audioEventCount == beforeFFT + 1 && events.audioSettings.fft);
  assert(events.audioSettings.mode == 0 && events.audioSettings.bands[7] == 7);
  assert(protocol.processAudioStream(fftStream + 33, 9, reply));
  assert(events.audioEventCount == beforeFFT + 2);
  assert(events.audioSettings.mode == 1 && events.audioSettings.bands[7] == 8);
  const uint8_t audioFftAck[] = {0x05,0x00,0x01,0x02,0x01};
  expectReply(reply, audioFftAck, sizeof(audioFftAck));

  protocol.resetAudioStream();
  const uint32_t beforeHeaderSplit = events.audioEventCount;
  assert(protocol.processAudioStream(fftStream, 24, reply));
  assert(events.audioEventCount == beforeHeaderSplit + 1);
  assert(protocol.processAudioStream(fftStream + 24, 18, reply));
  assert(events.audioEventCount == beforeHeaderSplit + 2);

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

  const uint8_t countdownStart[] = {0x07, 0x00, 0x08, 0x80, 0x01, 0x02, 0x1E};
  const uint8_t countdownAck[] = {0x05, 0x00, 0x08, 0x80, 0x01};
  assert(protocol.processFA02(countdownStart, sizeof(countdownStart), reply));
  assert(events.countdownEventReceived);
  assert(events.countdownSettings.mode == 1);
  assert(events.countdownSettings.minutes == 2);
  assert(events.countdownSettings.seconds == 30);
  expectReply(reply, countdownAck, sizeof(countdownAck));

  const uint8_t stopwatchPause[] = {0x05, 0x00, 0x09, 0x80, 0x02};
  const uint8_t stopwatchAck[] = {0x05, 0x00, 0x09, 0x80, 0x01};
  assert(protocol.processFA02(stopwatchPause, sizeof(stopwatchPause), reply));
  assert(events.stopwatchEventReceived && events.stopwatchMode == 2);
  expectReply(reply, stopwatchAck, sizeof(stopwatchAck));

  const uint8_t scoreboard[] = {0x08, 0x00, 0x0A, 0x80, 0x34, 0x12, 0x78, 0x56};
  const uint8_t scoreboardAck[] = {0x05, 0x00, 0x0A, 0x80, 0x01};
  assert(protocol.processFA02(scoreboard, sizeof(scoreboard), reply));
  assert(events.scoreboardEventReceived);
  assert(events.scoreboardA == 0x1234 && events.scoreboardB == 0x5678);
  expectReply(reply, scoreboardAck, sizeof(scoreboardAck));

  assert(!protocol.pollAsyncReply(reply));
  assert(!reply.available());
  events.countdownFinishedPending = true;
  const uint8_t countdownFinished[] = {0x05, 0x00, 0x08, 0x80, 0x03};
  assert(protocol.pollAsyncReply(reply));
  expectReply(reply, countdownFinished, sizeof(countdownFinished));
  assert(!protocol.pollAsyncReply(reply));

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

  const uint8_t rawBytes[] = {1, 2, 3};
  assert(protocol.beginRawImage(sizeof(rawBytes)));
  assert(protocol.writeRawImage(0, rawBytes, sizeof(rawBytes)));
  assert(protocol.completeRawImage(true));
  assert(events.rawBeginReceived && events.rawDataReceived &&
    events.rawCompleteReceived && events.rawCRCValid);
  assert(events.rawExpectedBytes == sizeof(rawBytes));
  assert(events.rawOffset == 0 && events.rawLength == sizeof(rawBytes));
  assert(events.rawFirstByte == 1);

  uint8_t pngPacket[17] = {
    17, 0, 0, 0, 0, 8, 0, 0, 0,
    0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A
  };
  assert(protocol.processInlinePng(pngPacket, sizeof(pngPacket), reply));
  const uint8_t pngAck[] = {5, 0, 0, 0, 3};
  expectReply(reply, pngAck, sizeof(pngAck));
  assert(events.pngReceived && events.pngLength == 8);

  assert(protocol.beginGif(123));
  assert(protocol.writeGif(4, rawBytes, sizeof(rawBytes)));
  assert(protocol.completeGif(true));
  assert(events.gifLength == 123 && events.gifOffset == 4);
  assert(events.gifDataLength == sizeof(rawBytes) && events.gifCRCValid);
}
