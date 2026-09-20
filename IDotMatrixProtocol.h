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

struct IDotMatrixRGB {
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;
};

struct IDotMatrixLightEffectSettings {
  static constexpr uint8_t MAX_COLORS = 16;

  uint8_t effect = 0;
  uint8_t speed = 90;
  uint8_t colorCount = 0;
  IDotMatrixRGB colors[MAX_COLORS]{};
};

struct IDotMatrixAudioSettings {
  bool fft = false;
  uint8_t mode = 0;
  uint8_t level = 0;
  uint8_t bands[8]{};
};

struct IDotMatrixCountdownSettings {
  uint8_t mode = 0;
  uint8_t minutes = 0;
  uint8_t seconds = 0;
};

struct IDotMatrixTimeSyncSettings {
  uint16_t year = 2000;
  uint8_t month = 1;
  uint8_t day = 1;
  uint8_t weekday = 0;
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t second = 0;
};

struct IDotMatrixAlarmSettings {
  static constexpr uint8_t SLOT_COUNT = 10;
  static constexpr size_t FULL_HEADER_SIZE = 24;

  uint8_t slot = 0;
  uint8_t flags = 0;
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t durationSeconds = 10;
  uint8_t reserved1 = 0;
  uint8_t contentType = 0;
  uint8_t buzzer = 0;
  uint8_t reserved2 = 0;
  uint32_t mediaSize = 0;
  uint32_t mediaCRC = 0;
  uint16_t reserved3 = 0;
  uint8_t mediaId = 0;
  size_t packetLength = 0;
  bool fullHeader = false;
};

struct IDotMatrixScheduleActivitySettings {
  static constexpr uint8_t MAX_ACTIVITIES = 32;
  static constexpr size_t HEADER_SIZE = 23;

  uint8_t index = 0;
  uint8_t flags = 0;
  uint8_t startHour = 0;
  uint8_t startMinute = 0;
  uint8_t endHour = 0;
  uint8_t endMinute = 0;
  uint8_t contentType = 0;
  uint8_t chunkMarker = 0;
  uint32_t mediaSize = 0;
  uint32_t mediaCRC = 0;
  uint16_t reserved = 0;
  uint8_t mediaId = 0;
};

struct IDotMatrixTextSettings {
  uint8_t glyphCount = 0;
  uint8_t glyphWidth = 0;
  uint8_t glyphHeight = 0;
  uint16_t glyphBytes = 0;
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
  virtual void onDeviceReset() = 0;
  virtual void onScreenPower(bool on) = 0;
  virtual void onBrightnessPercent(uint8_t percent) = 0;
  virtual void onSolidColor(uint8_t red, uint8_t green, uint8_t blue) = 0;
  virtual void onLightEffect(const IDotMatrixLightEffectSettings& settings) = 0;
  virtual void onAudio(const IDotMatrixAudioSettings& settings) = 0;
  virtual void onGraffitiMode(bool enter) = 0;
  virtual void onGraffitiPixels(
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    const uint8_t* coordinates,
    size_t coordinateBytes
  ) = 0;
  virtual bool onGraffitiRasterBegin(size_t byteLength) = 0;
  virtual bool onGraffitiRasterData(
    size_t offset,
    const uint8_t* data,
    size_t length
  ) = 0;
  virtual bool onGraffitiRasterComplete(bool valid) = 0;
  virtual void onClock(const IDotMatrixClockSettings& settings) = 0;
  virtual void onCountdown(const IDotMatrixCountdownSettings& settings) = 0;
  virtual void onStopwatch(uint8_t mode) = 0;
  virtual void onScoreboard(uint16_t scoreA, uint16_t scoreB) = 0;
  virtual bool takeCountdownFinished() = 0;
  virtual bool onTextBegin(const IDotMatrixTextSettings& settings) = 0;
  virtual void onTextGlyph(
    uint8_t index,
    const uint8_t* bitmap,
    size_t bitmapLength
  ) = 0;
  virtual void onTextComplete() = 0;
  virtual bool onRawImageBegin(size_t byteLength) = 0;
  virtual bool onRawImageData(
    size_t offset,
    const uint8_t* data,
    size_t length
  ) = 0;
  virtual bool onRawImageComplete(bool crcValid) = 0;
  virtual bool onPngImage(const uint8_t* data, size_t length) = 0;
  virtual bool onGifBegin(size_t byteLength) = 0;
  virtual bool onGifData(size_t offset, const uint8_t* data, size_t length) = 0;
  virtual bool onGifComplete(bool crcValid) = 0;
};

// Alarm/program handling is deliberately kept separate from the display event
// sink.  The display adapter remains concerned only with framebuffer/WLED
// ownership, while the usermod can attach a persistent automation subsystem
// that owns time, filesystem metadata and the optional buzzer.
class IDotMatrixAutomationEvents {
public:
  virtual ~IDotMatrixAutomationEvents() = default;
  virtual void onTimeSync(const IDotMatrixTimeSyncSettings& settings) = 0;
  virtual bool onAlarm(
    const IDotMatrixAlarmSettings& settings,
    const uint8_t* media,
    size_t mediaLength
  ) = 0;
  virtual void onScheduleGlobal(uint8_t flags) = 0;
  virtual void onAutomationReset() = 0;
  virtual bool onScheduleActivity(
    const IDotMatrixScheduleActivitySettings& settings,
    const uint8_t* media,
    size_t mediaLength
  ) = 0;
};


class IDotMatrixCarouselEvents {
public:
  virtual ~IDotMatrixCarouselEvents() = default;
  virtual void onCarouselReset() = 0;
  virtual void onCarouselConfigure(const uint8_t* slots, uint8_t count) = 0;
  virtual void onCarouselEnter() = 0;
  virtual void onCarouselSuspend() = 0;
  virtual bool onCarouselAssetBegin(uint8_t type, uint8_t slot, uint16_t dwellSeconds, size_t totalLength) = 0;
  virtual bool onCarouselAssetData(size_t offset, const uint8_t* data, size_t length) = 0;
  virtual bool onCarouselAssetComplete(bool crcValid) = 0;
  virtual void onCarouselAssetCancel() = 0;
};

class IDotMatrixPresetEvents {
public:
  virtual ~IDotMatrixPresetEvents() = default;
  virtual void onPresetReset() = 0;
  virtual void onPresetActivate(const uint8_t* slots, uint8_t count) = 0;
  virtual void onPresetSuspend() = 0;
  virtual bool onPresetAssetBegin(uint8_t type, uint8_t protocolSlot, uint16_t timeSign, size_t totalLength) = 0;
  virtual bool onPresetAssetData(size_t offset, const uint8_t* data, size_t length) = 0;
  virtual bool onPresetAssetComplete(bool crcValid) = 0;
  virtual void onPresetAssetCancel() = 0;
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
  ~IDotMatrixProtocol();

  void setScreenType(uint8_t screenType);
  void setDeviceReleaseVersion(uint8_t major, uint8_t minor);
  void setAutomationEvents(IDotMatrixAutomationEvents* events) { automationEvents_ = events; }
  void setCarouselEvents(IDotMatrixCarouselEvents* events) { carouselEvents_ = events; }
  void setPresetEvents(IDotMatrixPresetEvents* events) { presetEvents_ = events; }
  void onConnected();
  void loop(uint32_t now);
  void makeDeviceInfoReply(IDotMatrixReply& reply) const;
  bool processFA02(const uint8_t* data, size_t length, IDotMatrixReply& reply);
  bool processAudioStream(const uint8_t* data, size_t length, IDotMatrixReply& reply);
  void resetAudioStream();
  bool pollAsyncReply(IDotMatrixReply& reply);
  bool processTextPayload(const uint8_t* data, size_t length, bool takeDisplayOwnership = true);
  bool beginRawImage(size_t byteLength);
  bool writeRawImage(size_t offset, const uint8_t* data, size_t length);
  bool completeRawImage(bool crcValid);
  bool processInlinePng(const uint8_t* data, size_t length, IDotMatrixReply& reply);
  bool processGraffitiRaster(const uint8_t* data, size_t length, IDotMatrixReply& reply);
  void cancelGraffitiRaster();
  bool beginGif(size_t byteLength);
  bool writeGif(size_t offset, const uint8_t* data, size_t length);
  bool completeGif(bool crcValid);
  bool beginCarouselAsset(uint8_t type, uint8_t slot, uint16_t dwellSeconds, size_t totalLength);
  bool writeCarouselAsset(size_t offset, const uint8_t* data, size_t length);
  bool completeCarouselAsset(bool crcValid);
  void cancelCarouselAsset();
  void suspendCarousel();
  bool beginPresetAsset(uint8_t type, uint8_t protocolSlot, uint16_t timeSign, size_t totalLength);
  bool writePresetAsset(size_t offset, const uint8_t* data, size_t length);
  bool completePresetAsset(bool crcValid);
  void cancelPresetAsset();
  void suspendPreset();

private:
  static bool hasValidLength(const uint8_t* data, size_t length);
  static uint16_t readLE16(const uint8_t* data);
  static uint32_t readLE32(const uint8_t* data);
  static uint32_t crc32(const uint8_t* data, size_t length);
  static void makeCommandAck(uint8_t command, uint8_t subcommand, IDotMatrixReply& reply);


  static constexpr uint32_t AUTOMATION_TRANSFER_TIMEOUT_MS = 5000u;
  static constexpr size_t AUTOMATION_TRANSFER_MAX_BYTES = 512u * 1024u;
  static constexpr uint32_t GRAFFITI_RASTER_TIMEOUT_MS = 5000u;

  struct MultipartTransfer {
    uint8_t* buffer = nullptr;
    size_t expected = 0;
    size_t received = 0;
    uint32_t crc = 0;
    uint32_t lastRxMs = 0;
    bool active = false;
  };

  struct GraffitiRasterTransfer {
    size_t expected = 0;
    size_t received = 0;
    uint32_t lastRxMs = 0;
    bool active = false;
    bool sinkReady = false;
  };

  static void* allocateMultipart(size_t size);
  static void freeMultipart(void* memory);
  static void resetMultipart(MultipartTransfer& transfer);
  bool appendMultipart(MultipartTransfer& transfer, const uint8_t* data, size_t length, size_t expected, uint32_t crc);
  void expireMultipartTransfers(uint32_t now);
  size_t expectedGraffitiRasterBytes() const;
  static bool sameAlarmTransfer(const IDotMatrixAlarmSettings& a, const IDotMatrixAlarmSettings& b);
  static bool sameProgramTransfer(const IDotMatrixScheduleActivitySettings& a, const IDotMatrixScheduleActivitySettings& b);

  IDotMatrixProtocolEvents& events_;
  IDotMatrixAutomationEvents* automationEvents_ = nullptr;
  IDotMatrixCarouselEvents* carouselEvents_ = nullptr;
  IDotMatrixPresetEvents* presetEvents_ = nullptr;
  uint8_t screenType_ = 0x01;
  uint8_t releaseMajor_ = 0x00;
  uint8_t releaseMinor_ = 0x00;
  uint8_t audioFrame_[21]{};
  uint8_t audioFrameLength_ = 0;
  uint8_t audioFrameExpected_ = 0;

  MultipartTransfer alarmTransfer_{};
  MultipartTransfer programTransfer_{};
  GraffitiRasterTransfer graffitiRaster_{};
  IDotMatrixAlarmSettings alarmTransferSettings_{};
  IDotMatrixScheduleActivitySettings programTransferSettings_{};
  uint32_t nowMs_ = 0;
};
