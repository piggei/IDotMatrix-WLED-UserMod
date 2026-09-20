#include "wled.h"
#include "IDotMatrixBLEServer.h"
#include "IDotMatrixProtocol.h"
#include "IDotMatrixRenderer.h"
#include "IDotMatrixMedia.h"
#include "IDotMatrixWLEDAdapter.h"
#include "IDotMatrixBuzzer.h"
#include "IDotMatrixAutomation.h"
#include "IDotMatrixCarousel.h"
#include "IDotMatrixPreset.h"
#include "IDotMatrixBuildProfile.h"
#include "IDotMatrixAudioSource.h"

#ifndef IDOT_DEFAULT_SCREEN_TYPE
#define IDOT_DEFAULT_SCREEN_TYPE 0x01
#endif

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_system.h>
#include <esp_heap_caps.h>
#endif

#if defined(IDOT_C3_WLED_IDF5) && !defined(CONFIG_IDF_TARGET_ESP32C3)
#error "IDOT_C3_WLED_IDF5 is only valid for ESP32-C3 builds"
#endif
#if defined(IDOT_C3_WLED_IDF5) && !defined(WLED_USE_SHARED_RMT)
#error "ESP32-C3 requires a WLED IDF5 build with WLED_USE_SHARED_RMT"
#endif
#if defined(IDOT_C3_WLED_IDF5) && !defined(IDOT_NIMBLE_V2_API)
#error "ESP32-C3 requires NimBLE-Arduino 2.x"
#endif
#if defined(IDOT_C3_WLED_IDF5) || defined(IDOT_S3_HUB75_WLED_IDF5)
#include <esp_idf_version.h>
#if ESP_IDF_VERSION_MAJOR < 5
#error "This iDotMatrix IDF5 profile requires ESP-IDF 5.x or newer"
#endif
#endif

#if defined(IDOT_S3_HUB75_WLED_IDF5) && !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "IDOT_S3_HUB75_WLED_IDF5 is only valid for ESP32-S3 builds"
#endif
#if defined(IDOT_S3_HUB75_WLED_IDF5) && !defined(WLED_ENABLE_HUB75MATRIX)
#error "ESP32-S3 iDotMatrix HUB75 profile requires WLED_ENABLE_HUB75MATRIX"
#endif
#if defined(IDOT_S3_HUB75_WLED_IDF5) && !defined(IDOT_NIMBLE_V2_API)
#error "ESP32-S3 iDotMatrix HUB75 profile requires NimBLE-Arduino 2.x"
#endif

static constexpr const char* IDOTMATRIX_RELEASE = "0.9.1";
static constexpr const char* IDOTMATRIX_BUILD = "0.9.1-rc.1";
static constexpr uint8_t IDOTMATRIX_APP_RELEASE_MAJOR = 0x00;
static constexpr uint8_t IDOTMATRIX_APP_RELEASE_MINOR = 0x09;

namespace {
const char USERMOD_NAME[] PROGMEM = "iDotMatrix";
const char CFG_ENABLED[] PROGMEM = "enabled";
const char CFG_SCREEN_TYPE[] PROGMEM = "screenType";
const char CFG_DEVICE_NAME[] PROGMEM = "deviceName";
const char CFG_RESCALE[] PROGMEM = "rescale";
const char CFG_BUZZER_PIN[] PROGMEM = "buzzer-pin";
const char CFG_BUZZER_ACTIVE_HIGH[] PROGMEM = "buzzerActiveHigh";
const char CFG_AUDIO_SOURCE[] PROGMEM = "audioSource";

#if defined(ARDUINO_ARCH_ESP32)
struct CrashSnapshot {
  uint32_t magic;
  uint32_t freeHeap;
  uint32_t minFreeHeap;
  uint32_t largestBlock;
  uint32_t uptimeMs;
  uint8_t content;
};
RTC_NOINIT_ATTR CrashSnapshot rtcSnapshot;
constexpr uint32_t SNAPSHOT_MAGIC = 0x49444D38u; // "IDM8"

const char* resetReasonText(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "poweron";
    case ESP_RST_EXT: return "external";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "int-wdt";
    case ESP_RST_TASK_WDT: return "task-wdt";
    case ESP_RST_WDT: return "wdt";
    case ESP_RST_DEEPSLEEP: return "deepsleep";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_SDIO: return "sdio";
    case ESP_RST_UNKNOWN:
    default: return "unknown";
  }
}
#endif
}

class IDotMatrixUsermod final : public Usermod {
private:
  bool enabled_ = true;
  uint8_t screenType_ = IDOT_DEFAULT_SCREEN_TYPE;
  String deviceName_;
  bool rescale_ = false;
  int8_t buzzerPin_ = -1;
  bool buzzerActiveHigh_ = true;
  bool buzzerHardwareReady_ = false;
  bool buzzerPinUnavailable_ = false;
  IDotMatrixAudioSourceMode audioSourceMode_ = IDotMatrixAudioSourceMode::Phone;
  bool audioReactivePresent_ = false;
  bool audioReactiveDataAvailable_ = false;
  uint32_t audioSourceNextPollAt_ = 0;
  bool setupComplete_ = false;
  IDotMatrixBuzzer buzzer_;
  IDotMatrixRenderer renderer_;
  IDotMatrixMedia media_{renderer_};
  IDotMatrixWLEDAdapter adapter_{renderer_, &media_};
  IDotMatrixProtocol protocol_{adapter_};
  IDotMatrixCarousel carousel_{protocol_, adapter_};
  IDotMatrixPreset preset_{protocol_, adapter_};
  IDotMatrixAutomation automation_{renderer_, adapter_, media_, buzzer_};
  IDotMatrixBLEServer ble_{protocol_};
  bool rmtBusActive_ = false;
  bool blockedByRmt_ = false;
  bool startPending_ = false;
  uint32_t startAt_ = 0;
  bool bleRestartRequired_ = false;
  bool runtimeRestartRequired_ = false;
#if defined(ARDUINO_ARCH_ESP32)
  esp_reset_reason_t bootResetReason_ = ESP_RST_UNKNOWN;
  CrashSnapshot previousSnapshot_{};
  bool previousSnapshotValid_ = false;
  uint32_t nextSnapshotAt_ = 0;
#endif

  static uint8_t dimensionForScreenType(uint8_t screenType) {
    if (screenType == 0x03) return 32;
    if (screenType == 0x04) return 64;
    return 16;
  }

  static String defaultDeviceName() {
#if defined(ARDUINO_ARCH_ESP32)
    const uint64_t mac = ESP.getEfuseMac();
    const uint32_t suffix = static_cast<uint32_t>(mac % 1000000ULL);
    char name[11];
    snprintf(name, sizeof(name), "IDM-%06u", static_cast<unsigned>(suffix));
    return String(name);
#else
    return String(F("IDM-000000"));
#endif
  }

  static String normalizedDeviceName(String value) {
    value.trim();
    String suffix = value;
    const bool hasIdmPrefix = value.length() >= 3 &&
      (value[0] == 'I' || value[0] == 'i') &&
      (value[1] == 'D' || value[1] == 'd') &&
      (value[2] == 'M' || value[2] == 'm');
    if (hasIdmPrefix) {
      suffix = value.substring(3);
      while (!suffix.isEmpty() &&
             (suffix[0] == '-' || suffix[0] == '.' || suffix[0] == '_' || suffix[0] == ' ')) {
        suffix.remove(0, 1);
      }
    }

    String clean;
    clean.reserve(11);
    for (size_t i = 0; i < suffix.length() && clean.length() < 11; ++i) {
      const char c = suffix[i];
      const bool valid = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
      if (valid) clean += c;
      else if (c == ' ' && !clean.isEmpty() && clean[clean.length() - 1] != '-') clean += '-';
    }
    if (clean.isEmpty()) {
      const String generated = defaultDeviceName();
      return generated;
    }
    return String(F("IDM-")) + clean;
  }

  bool hasDigitalRmtBus() const {
    for (const auto& bus : BusManager::busses) {
      if (bus && bus->isDigital() && bus->getDriverType() == 0) {
        return true;
      }
    }
    return false;
  }

  static constexpr bool isEsp32C3Build() {
#if defined(CONFIG_IDF_TARGET_ESP32C3)
    return true;
#else
    return false;
#endif
  }

  static constexpr bool c3SharedRmtBleEnabled() {
#if defined(IDOT_C3_WLED_IDF5)
    return true;
#else
    return false;
#endif
  }

  bool readAudioReactive(IDotMatrixAudioSettings& sample) {
    audioReactivePresent_ = UsermodManager::lookup(USERMOD_ID_AUDIOREACTIVE) != nullptr;

    um_data_t* data = nullptr;
    if (!UsermodManager::getUMData(&data, USERMOD_ID_AUDIOREACTIVE) ||
        data == nullptr || data->u_size < 3 || data->u_data == nullptr ||
        data->u_data[0] == nullptr || data->u_data[2] == nullptr) {
      audioReactiveDataAvailable_ = false;
      return false;
    }

    // Both WLED 16.0.1 AudioReactive and the pinned IDF5 branch export
    // volumeSmth at slot 0 and the 16-byte GEQ/FFT array at slot 2. Keep the
    // type checks defensive so a future incompatible provider cannot be read
    // through the old layout accidentally.
    if (data->u_type != nullptr &&
        (data->u_type[0] != UMT_FLOAT || data->u_type[2] != UMT_BYTE_ARR)) {
      audioReactiveDataAvailable_ = false;
      return false;
    }

    IDotMatrixAudioSource::mapAudioReactive(
      *static_cast<float*>(data->u_data[0]),
      static_cast<const uint8_t*>(data->u_data[2]),
      adapter_.audioUsesFFT(),
      adapter_.audioMode(),
      sample
    );
    audioReactiveDataAvailable_ = true;
    return true;
  }

  void serviceAudioSource(uint32_t now) {
    const bool strictLocal = audioSourceMode_ == IDotMatrixAudioSourceMode::AudioReactive;
    const bool autoLocal = audioSourceMode_ == IDotMatrixAudioSourceMode::Auto;

    if (!strictLocal && !autoLocal) {
      adapter_.setAudioDataOverride(false);
      // Presence is still useful diagnostic information in phone mode, but do
      // not repeatedly ask AudioReactive for live data when it is not selected.
      if (int32_t(now - audioSourceNextPollAt_) >= 0) {
        audioReactivePresent_ = UsermodManager::lookup(USERMOD_ID_AUDIOREACTIVE) != nullptr;
        audioReactiveDataAvailable_ = false;
        audioSourceNextPollAt_ = now + 1000u;
      }
      return;
    }

    if (int32_t(now - audioSourceNextPollAt_) < 0) return;
    audioSourceNextPollAt_ = now + 40u;

    IDotMatrixAudioSettings sample;
    const bool localAvailable = readAudioReactive(sample);
    if (localAvailable) {
      adapter_.setAudioDataOverride(true);
      adapter_.updateAudioSample(sample.level, sample.bands);
      return;
    }

    if (strictLocal) {
      // Explicit AudioReactive means exactly that source. If the usermod is
      // absent/disabled, keep the visualizer selected but feed silence rather
      // than silently reverting to the phone microphone.
      adapter_.setAudioDataOverride(true);
      adapter_.updateAudioSample(0, nullptr);
    } else {
      // Auto is the compatibility mode: prefer local AudioReactive data when
      // available, otherwise keep using the app's existing BLE audio stream.
      adapter_.setAudioDataOverride(false);
    }
  }

  const char* effectiveAudioSourceText() const {
    if (!adapter_.audioDataOverride()) return "phone";
    return audioReactiveDataAvailable_ ? "audioreactive" : "silent";
  }


  static void buzzerOutputThunk(void* context, bool on) {
    static_cast<IDotMatrixUsermod*>(context)->writeBuzzerOutput(on);
  }

  void writeBuzzerOutput(bool on) {
    if (!buzzerHardwareReady_ || buzzerPin_ < 0) return;
    digitalWrite(
      buzzerPin_,
      on ? (buzzerActiveHigh_ ? HIGH : LOW)
         : (buzzerActiveHigh_ ? LOW : HIGH)
    );
  }

  void teardownBuzzerHardware() {
    buzzer_.stop();
    if (buzzerHardwareReady_ && buzzerPin_ >= 0) {
      writeBuzzerOutput(false);
      pinMode(buzzerPin_, INPUT);
    }
    buzzerHardwareReady_ = false;
    buzzerPinUnavailable_ = false;
  }

  void setupBuzzerHardware() {
    buzzer_.stop();
    buzzerHardwareReady_ = false;
    buzzerPinUnavailable_ = false;
    if (!enabled_ || buzzerPin_ < 0) return;

    // WLED 0.16.x does not provide a collision-safe PinOwner for out-of-tree
    // usermods.  The config key deliberately ends in "pin", so WLED's Usermod
    // settings page advertises it as reserved.  At runtime we additionally
    // refuse pins already owned by the core or another registered component.
    // Do not borrow another usermod's PinOwner: that would make deallocation
    // ambiguous.  If upstream gains external PinOwner registration this check
    // can become a real allocatePin()/deallocatePin() pair without changing the
    // user-facing configuration.
    if (!PinManager::isPinOk(buzzerPin_, true) || PinManager::isPinAllocated(buzzerPin_)) {
      buzzerPinUnavailable_ = true;
      DEBUG_PRINTF_P(PSTR("[iDotMatrix] buzzer GPIO %d unavailable\n"), buzzerPin_);
      return;
    }

    pinMode(buzzerPin_, OUTPUT);
    buzzerHardwareReady_ = true;
    writeBuzzerOutput(false);
    DEBUG_PRINTF_P(
      PSTR("[iDotMatrix] active buzzer ready on GPIO %d (%s)\n"),
      buzzerPin_, buzzerActiveHigh_ ? "active-high" : "active-low"
    );
  }

  void registerBuzzerTestEndpoint() {
    server.on(F("/idotmatrix/buzzer-test"), HTTP_POST, [this](AsyncWebServerRequest* request) {
      if (strlen(settingsPIN) > 0 && !correctPIN) {
        request->send(403, FPSTR(CONTENT_TYPE_PLAIN), F("Unlock WLED settings before testing the buzzer."));
        return;
      }
      if (!enabled_) {
        request->send(409, FPSTR(CONTENT_TYPE_PLAIN), F("iDotMatrix usermod is disabled."));
        return;
      }
      if (buzzerPin_ < 0) {
        request->send(409, FPSTR(CONTENT_TYPE_PLAIN), F("Configure the buzzer GPIO and save first."));
        return;
      }
      if (buzzerPinUnavailable_) {
        request->send(409, FPSTR(CONTENT_TYPE_PLAIN), F("The configured buzzer GPIO is unavailable."));
        return;
      }
      if (!buzzerHardwareReady_) {
        request->send(409, FPSTR(CONTENT_TYPE_PLAIN), F("Buzzer hardware is not ready."));
        return;
      }
      if (buzzer_.isPlaying()) {
        request->send(409, FPSTR(CONTENT_TYPE_PLAIN), F("Buzzer is already active."));
        return;
      }

      // One finite three-beep trill is enough to validate wiring and polarity.
      // Alarms use the repeating pattern; schedules use a finite activation alert.
      buzzer_.startTest(millis());
      request->send(200, FPSTR(CONTENT_TYPE_PLAIN), F("ok"));
    });
  }

public:
  void setup() override {
#if defined(ARDUINO_ARCH_ESP32)
    bootResetReason_ = esp_reset_reason();
    if (rtcSnapshot.magic == SNAPSHOT_MAGIC) {
      previousSnapshot_ = rtcSnapshot;
      previousSnapshotValid_ = true;
    }
    rtcSnapshot = CrashSnapshot{};
    rtcSnapshot.magic = SNAPSHOT_MAGIC;
#endif
    buzzer_.attach(&IDotMatrixUsermod::buzzerOutputThunk, this);
    protocol_.setAutomationEvents(&automation_);
    protocol_.setCarouselEvents(&carousel_);
    protocol_.setPresetEvents(&preset_);
    protocol_.setDeviceReleaseVersion(IDOTMATRIX_APP_RELEASE_MAJOR, IDOTMATRIX_APP_RELEASE_MINOR);
    automation_.attachProtocol(&protocol_);
    registerBuzzerTestEndpoint();
    setupBuzzerHardware();
    if (deviceName_.isEmpty()) deviceName_ = defaultDeviceName();
    if (!enabled_) { setupComplete_ = true; return; }

    // WLED's classic-ESP32 RMT-HI LED driver can conflict with the Bluetooth
    // controller, so the stable policy remains "RMT => block BLE" there. The
    // supported ESP32-C3 profile is the narrow exception: it is compiled only
    // against WLED's IDF5/shared-RMT backend, which was hardware-validated with
    // BLE active and is guarded at compile time above.
    rmtBusActive_ = hasDigitalRmtBus();
    blockedByRmt_ = IDotMatrixBuildProfile::shouldBlockBleForRmt(
      rmtBusActive_, isEsp32C3Build(), c3SharedRmtBleEnabled()
    );
    if (blockedByRmt_) {
      DEBUG_PRINTLN(F("[iDotMatrix] BLE blocked: select I2S for every digital LED output"));
      setupComplete_ = true;
      return;
    }
    if (rmtBusActive_ && c3SharedRmtBleEnabled()) {
      DEBUG_PRINTLN(F("[iDotMatrix] ESP32-C3 shared-RMT + BLE coexistence enabled"));
    }

    // ESP-IDF requires Wi-Fi modem sleep while Wi-Fi and Bluetooth coexist.
    // WLED defaults noWifiSleep to true on ESP32, which makes the Wi-Fi task
    // intentionally abort after Bluetooth has been enabled.
    noWifiSleep = false;

    uint8_t storageWidth = 0;
    uint8_t storageHeight = 0;
#ifndef WLED_DISABLE_2D
    if (rescale_ && strip.isMatrix) {
      const uint8_t logical = dimensionForScreenType(screenType_);
      storageWidth = uint8_t(Segment::maxWidth < logical ? Segment::maxWidth : logical);
      storageHeight = uint8_t(Segment::maxHeight < logical ? Segment::maxHeight : logical);
      if (storageWidth == 0 || storageHeight == 0) {
        storageWidth = 0;
        storageHeight = 0;
      }
    }
#endif
    if (!renderer_.begin(screenType_, storageWidth, storageHeight)) {
      DEBUG_PRINTLN(F("[iDotMatrix] framebuffer allocation failed"));
    }
    adapter_.setRescaleEnabled(rescale_);
    adapter_.setAudioDataOverride(audioSourceMode_ == IDotMatrixAudioSourceMode::AudioReactive);

    if (adapter_.registerDisplayEffect()) {
      DEBUG_PRINTF_P(
        PSTR("[iDotMatrix] display effect registered as id %u\n"),
        adapter_.displayEffectId()
      );
      // WLED queues the boot preset in beginStrip() and applies it only after
      // UsermodManager::setup() has returned. The effect is therefore already
      // registered before the preset is deserialized; no Usermod-side replay
      // is required or desirable.
    } else {
      DEBUG_PRINTLN(F("[iDotMatrix] display effect registration failed"));
    }

    automation_.begin();
    carousel_.begin();
    preset_.begin();

    // Let WLED complete its first Wi-Fi initialization pass before starting
    // the lower-memory NimBLE host.
    startPending_ = true;
    startAt_ = millis() + 5000;
    setupComplete_ = true;
  }

  void onStateChange(uint8_t mode) override {
    (void)mode;
    if (!enabled_ || blockedByRmt_) return;

    // Selection is consumed in loop() by scanning every real WLED segment.
    // Trigger a render pass as usual, but Carousel/Clock startup does not wait
    // for the custom-effect callback and is completely independent of BLE.
    strip.trigger();
  }

  void loop() override {
    if (!enabled_) return;

    // The optional buzzer is independent of BLE/display rendering. Keep its
    // non-blocking pattern engine alive even when BLE is blocked by an RMT bus.
    buzzer_.loop(millis());
    if (blockedByRmt_) return;


    if (startPending_ && int32_t(millis() - startAt_) >= 0) {
      startPending_ = false;
      if (ble_.begin(deviceName_.c_str(), screenType_)) {
        bleRestartRequired_ = false;
        DEBUG_PRINTLN(F("[iDotMatrix] NimBLE server and advertising initialized"));
      } else {
        DEBUG_PRINTLN(F("[iDotMatrix] NimBLE server initialization failed"));
      }
    }

    // Poll the WLED segment table before BLE/media service. This is the
    // authoritative "effect selected" event for standalone startup and also
    // runs while the phone app is disconnected. At boot it naturally becomes
    // true as soon as WLED has applied the configured boot preset.
    adapter_.pollDisplayEffectSelection();

    serviceAudioSource(millis());
    protocol_.loop(millis());
    ble_.loop();

    // Selecting iDotMatrix means: stored Carousel first, otherwise
    // Clock. No BLE connection or app command is required.
    if (adapter_.takeDisplayEffectActivationRequest()) {
      // A real WLED invocation is authoritative even if the transition has not
      // yet committed Segment::mode. First make the public WLED state match the
      // effect WLED is actually servicing, then apply the standalone policy.
      adapter_.claimDisplayEffectFromCallback();
      if (!carousel_.playing() && !carousel_.autoStartPending() && !preset_.playing() &&
          !adapter_.hasLogicalContent()) {
        if (carousel_.hasAssets()) carousel_.enter();
        else adapter_.restoreClockFallback();
      }
      strip.trigger();
    }

    carousel_.loop(millis());
    preset_.loop(millis());
    automation_.loop(millis());
    adapter_.loop(millis());

    // WLED effects can be selected directly from the Web UI/API while an
    // iDotMatrix GIF or other media mode is active. Detect that ownership
    // change before servicing media so dynamic decoder RAM is released at once.
    adapter_.syncWLEDControl();
    media_.loop(millis());
    const bool mediaFailed = media_.lastError() != IDotMatrixMedia::Error::None;
    const bool carouselMediaFailed =
      carousel_.playing() && carousel_.currentSlot() >= 0 && mediaFailed &&
      (adapter_.isGifPending() || adapter_.isGifActive());
    const bool presetMediaFailed =
      preset_.playing() && preset_.currentProtocolSlot() >= 0 && mediaFailed &&
      (adapter_.isGifPending() || adapter_.isGifActive());
    adapter_.syncGifPlayback(media_.gifActive(), mediaFailed);
    if (carouselMediaFailed) carousel_.onPlaybackFailure(millis());
    if (presetMediaFailed) preset_.suspend();

    // Conversely, selecting a native WLED effect must stop autonomous Carousel
    // ownership.  During GIF frame-cache staging WLED Static is temporary and
    // gifPending keeps the Carousel alive until iDotMatrix is restored.
    if (carousel_.playing() && !adapter_.isDisplayEffectActive() &&
        !adapter_.isGifPending()) {
      carousel_.suspend();
    }
    if (preset_.playing() && !adapter_.isDisplayEffectActive() &&
        !adapter_.isGifPending()) {
      preset_.suspend();
    }

#if defined(ARDUINO_ARCH_ESP32)
    const uint32_t now = millis();
    if (int32_t(now - nextSnapshotAt_) >= 0) {
      nextSnapshotAt_ = now + 250;
      rtcSnapshot.magic = SNAPSHOT_MAGIC;
      rtcSnapshot.freeHeap = ESP.getFreeHeap();
      rtcSnapshot.minFreeHeap = ESP.getMinFreeHeap();
      rtcSnapshot.largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
      rtcSnapshot.uptimeMs = now;
      rtcSnapshot.content = adapter_.isClockActive() ? 1 :
        adapter_.isTextActive() ? 2 : adapter_.isGifActive() ? 3 :
        adapter_.isRawImageActive() ? 4 : adapter_.isLightEffectActive() ? 6 :
        adapter_.isSolidActive() ? 7 : adapter_.isCountdownActive() ? 8 :
        adapter_.isStopwatchActive() ? 9 : adapter_.isScoreboardActive() ? 10 :
        renderer_.isVisible() ? 5 : 0;
    }
#endif
  }

  void addToJsonInfo(JsonObject& root) override {
    JsonObject user = root["u"];
    if (user.isNull()) user = root.createNestedObject("u");

    JsonArray info = user.createNestedArray(FPSTR(USERMOD_NAME));
    if (!enabled_) {
      info.add(F("Disabled"));
      if (runtimeRestartRequired_) info.add(F("Restart required after Enabled change"));
      return;
    }

    if (blockedByRmt_) {
      info.add(F("BLE blocked: change digital LED driver from RMT to I2S"));
      return;
    }

    info.add(startPending_ ? F("BLE startup pending") : ble_.isInitialized()
      ? (ble_.isConnected() ? F("BLE connected") : ble_.isAdvertising()
        ? F("BLE advertising") : F("BLE ready"))
      : F("BLE init failed"));
    if (rmtBusActive_ && c3SharedRmtBleEnabled()) {
      info.add(F("RMT+BLE=ESP32-C3 shared-RMT"));
    }
#if defined(IDOT_C3_WLED_IDF5)
    info.add(F("framework=WLED IDF5/shared-RMT"));
    info.add(F("wledBase=d55037f"));
#if defined(IDOT_NIMBLE_V2_API)
    info.add(F("nimble=2.x API"));
#else
    info.add(F("nimble=1.x API"));
#endif
#endif
#if defined(IDOT_S3_HUB75_WLED_IDF5)
    info.add(F("framework=WLED IDF5/HUB75"));
    info.add(F("target=MatrixPortal-S3"));
    info.add(F("wledBase=06ae26d"));
#if defined(IDOT_NIMBLE_V2_API)
    info.add(F("nimble=2.x API"));
#else
    info.add(F("nimble=1.x API"));
#endif
#if defined(ARDUINO_ARCH_ESP32)
    info.add(String(F("psram=")) + ESP.getPsramSize() +
      F(" free=") + ESP.getFreePsram());
#endif
#endif
    info.add(String(F("profile=")) + String(renderer_.logicalWidth()) + 'x' + String(renderer_.logicalHeight()));
    info.add(String(F("canvas=")) + String(renderer_.width()) + 'x' + String(renderer_.height()));
    {
      String scaleLine = String(F("output=")) + adapter_.targetWidth() + 'x' + adapter_.targetHeight();
      if (adapter_.autoUpscaleActive()) {
        scaleLine += F(" scale=auto-up");
        if (renderer_.width() > 0 && renderer_.height() > 0 &&
            adapter_.targetWidth() % renderer_.width() == 0 &&
            adapter_.targetHeight() % renderer_.height() == 0) {
          scaleLine += ':';
          scaleLine += adapter_.targetWidth() / renderer_.width();
          scaleLine += 'x';
          scaleLine += adapter_.targetHeight() / renderer_.height();
        }
      } else if (adapter_.autoDownscaleActive()) {
        scaleLine += F(" scale=auto-down");
        if (adapter_.targetWidth() > 0 && adapter_.targetHeight() > 0 &&
            renderer_.width() % adapter_.targetWidth() == 0 &&
            renderer_.height() % adapter_.targetHeight() == 0) {
          scaleLine += ':';
          scaleLine += renderer_.width() / adapter_.targetWidth();
          scaleLine += 'x';
          scaleLine += renderer_.height() / adapter_.targetHeight();
        }
      } else {
        scaleLine += adapter_.dimensionsMatch() ? F(" scale=1x1") : F(" scale=auto-mixed");
      }
      info.add(scaleLine);
    }
    info.add(String(F("name=")) + deviceName_);
    info.add(String(F("release=")) + IDOTMATRIX_RELEASE);
    info.add(String(F("build=")) + IDOTMATRIX_BUILD);
    {
      char fxLine[224];
      snprintf(fxLine, sizeof(fxLine), "displayFx=id:%u count:%u seg:%u mode:%u current:%u active:%u observed:%u cb:%lu livecb:%lu oldcb:%lu cbseg:%u cbmode:%u cblive:%u lease:%u logical:%u",
        unsigned(adapter_.displayEffectId()), unsigned(strip.getModeCount()),
        unsigned(adapter_.displayEffectSegmentId()), unsigned(adapter_.selectedEffectId()),
        unsigned(effectCurrent), adapter_.isDisplayEffectActive() ? 1u : 0u,
        adapter_.displayEffectObserved() ? 1u : 0u,
        static_cast<unsigned long>(adapter_.displayEffectCallbackCount()),
        static_cast<unsigned long>(adapter_.displayEffectLiveCallbackCount()),
        static_cast<unsigned long>(adapter_.displayEffectOldCallbackCount()),
        unsigned(adapter_.displayEffectCallbackSegmentId()),
        unsigned(adapter_.displayEffectCallbackContextMode()),
        adapter_.displayEffectLastCallbackWasLive() ? 1u : 0u,
        adapter_.displayEffectCallbackLeaseActive(millis()) ? 1u : 0u,
        adapter_.hasLogicalContent() ? 1u : 0u);
      info.add(fxLine);
    }
    info.add(String(F("audioSource=")) + IDotMatrixAudioSource::modeText(audioSourceMode_) +
      F(" active=") + effectiveAudioSourceText());
    info.add(String(F("audioReactive=")) +
      (audioReactivePresent_ ? (audioReactiveDataAvailable_ ? F("data") : F("present")) : F("absent")));
    info.add(String(F("gifDecoder=")) + media_.gifDecoderModeText());
    info.add(String(F("gifDecoderBytes=")) + media_.gifDecoderBytes());
    if (media_.gifProbeFree() > 0) {
      info.add(String(F("gifProbe=")) + media_.gifProbeFree() +
        F(" largest=") + media_.gifProbeLargest() +
        F(" reserve=") + media_.gifDramReserve());
    }
    if (adapter_.isLightEffectActive()) {
      info.add(String(F("lightEffect=")) + adapter_.lightEffectId() +
        F(" speed=") + adapter_.lightEffectSpeed() +
        F(" colors=") + adapter_.lightEffectColorCount());
    }
    if (adapter_.isCountdownActive()) {
      const uint32_t remaining = adapter_.countdownRemainingSeconds(millis());
      info.add(String(F("countdown=")) +
        (adapter_.isCountdownRunning() ? F("run") :
          adapter_.isCountdownPaused() ? F("pause") : F("stop")) +
        F(" remain=") + remaining + F("s"));
    }
    if (adapter_.isStopwatchActive()) {
      info.add(String(F("stopwatch=")) +
        (adapter_.isStopwatchRunning() ? F("run") : F("stop")) +
        F(" elapsed=") + adapter_.stopwatchElapsedSeconds(millis()) + F("s"));
    }
    if (adapter_.isScoreboardActive()) {
      info.add(String(F("score=")) + adapter_.scoreA() + ':' + adapter_.scoreB());
    }
    if (buzzerPin_ < 0) {
      info.add(F("buzzer=disabled"));
    } else if (buzzerPinUnavailable_) {
      info.add(String(F("buzzer=gpio ")) + String(buzzerPin_) + F(" unavailable"));
    } else if (buzzerHardwareReady_) {
      info.add(String(F("buzzer=active gpio=")) + String(buzzerPin_) +
        F(" polarity=") + (buzzerActiveHigh_ ? F("high ") : F("low ")) +
        (buzzer_.isPlaying() ? F("playing") : F("idle")));
    } else {
      info.add(String(F("buzzer=gpio ")) + String(buzzerPin_) + F(" not-ready"));
    }
    info.add(String(F("alarms=")) + automation_.configuredAlarmCount() +
      (automation_.alarmActive()
        ? String(F(" active=")) + automation_.activeAlarmSlot()
        : String()));
    {
      char alarmDiag[192];
      automation_.alarmDiagnosticSummary(alarmDiag, sizeof(alarmDiag), millis());
      info.add(alarmDiag);
      for (uint8_t slot = 0; slot < IDotMatrixAlarmSettings::SLOT_COUNT; ++slot) {
        char slotDiag[160];
        if (automation_.alarmDiagnosticSlot(slot, slotDiag, sizeof(slotDiag))) info.add(slotDiag);
      }
    }
    info.add(String(F("schedule=")) +
      (automation_.scheduleEnabled() ? F("on") : F("off")) +
      F(" activities=") + automation_.configuredScheduleCount() +
      (automation_.activeScheduleIndex() >= 0
        ? String(F(" active=")) + automation_.activeScheduleIndex()
        : String()) +
      (automation_.scheduleSoundEnabled() ? F(" sound=1") : F(" sound=0")));
    char carouselLine[96];
    snprintf(
      carouselLine, sizeof(carouselLine),
      "carousel=%s stored=%u configured=%u slot=%d dwell=%us",
      carousel_.playing() ? "playing" : (carousel_.resumeOnBoot() ? "stored" : "off"),
      unsigned(carousel_.storedCount()),
      unsigned(carousel_.configuredCount()),
      int(carousel_.currentSlot()),
      unsigned(carousel_.currentDwellSeconds())
    );
    info.add(carouselLine);
    if (carousel_.failedMask() != 0) {
      char failedLine[72];
      snprintf(failedLine, sizeof(failedLine), "carouselFailed=0x%03X last=%d",
        unsigned(carousel_.failedMask()), int(carousel_.lastFailedSlot()));
      info.add(failedLine);
    }
    if (!carousel_.lastManifestSaveOk()) info.add(F("carouselManifest=save-failed"));
    {
      char presetLine[128];
      snprintf(
        presetLine, sizeof(presetLine),
        "preset=%s active=%u pending=%u slot=%d hold=%lums upload=%u",
        preset_.playing() ? "playing" : "off",
        unsigned(preset_.activeCount()),
        unsigned(preset_.pendingCount()),
        int(preset_.currentProtocolSlot()),
        static_cast<unsigned long>(preset_.currentHoldMs()),
        preset_.uploadIndicatorActive() ? 1u : 0u
      );
      info.add(presetLine);
    }
    if (automation_.scheduleUploadOpen()) info.add(F("scheduleUpload=staging"));
    if (strcmp(automation_.lastErrorText(), "none") != 0) {
      info.add(String(F("automationError=")) + automation_.lastErrorText());
    }
    if (adapter_.isGifPending()) info.add(F("gifPending=1"));
    if (media_.gifCaching()) {
      info.add(String(F("gifCache=building frames=")) + media_.gifCachedFrames());
    } else if (media_.gifCachedFrames() > 0) {
      info.add(String(F("gifCachedFrames=")) + media_.gifCachedFrames());
    }
    if (media_.gifCacheWaitCount() > 0) {
      info.add(String(F("gifCacheWaits=")) + media_.gifCacheWaitCount() +
        F(" low=") + media_.gifCacheLowHeapMin() +
        F(" guard=") + media_.gifCacheRuntimeReserve());
    }
    if (media_.gifCacheBuildCount() > 0 || media_.gifCacheReuseCount() > 0) {
      info.add(String(F("gifCacheStats=build:")) + media_.gifCacheBuildCount() +
        F(" reuse:") + media_.gifCacheReuseCount());
    }
    if (ble_.rxDropped() || ble_.rxOversize() || ble_.rxMalformed() ||
        ble_.reassemblyTimeouts() || ble_.bulkTimeouts()) {
      char rxLine[128];
      snprintf(rxLine, sizeof(rxLine),
        "bleRx=drop:%lu oversize:%lu malformed:%lu faTimeout:%lu bulkTimeout:%lu",
        static_cast<unsigned long>(ble_.rxDropped()),
        static_cast<unsigned long>(ble_.rxOversize()),
        static_cast<unsigned long>(ble_.rxMalformed()),
        static_cast<unsigned long>(ble_.reassemblyTimeouts()),
        static_cast<unsigned long>(ble_.bulkTimeouts()));
      info.add(rxLine);
    }
    if (adapter_.protocolResetCount() > 0) {
      const bool carouselResetOk = carousel_.lastResetOk();
      const bool automationResetOk = automation_.lastResetOk();
      info.add(String(F("protocolReset=count:")) + adapter_.protocolResetCount() +
        F(" status:") + ((carouselResetOk && automationResetOk) ? F("ok") : F("partial")) +
        F(" carousel:") + (carouselResetOk ? F("ok") : F("fail")) +
        F(" automation:") + (automationResetOk ? F("ok") : F("fail")));
    }
    if (bleRestartRequired_) info.add(F("Restart required after name/profile change"));
    if (runtimeRestartRequired_) info.add(F("Restart required after Enabled change"));
    if (media_.lastError() != IDotMatrixMedia::Error::None) {
      info.add(String(F("mediaError=")) + media_.lastErrorText());
    }
#if defined(ARDUINO_ARCH_ESP32)
    info.add(String(F("bootReset=")) + resetReasonText(bootResetReason_) +
      F(" code=") + int(bootResetReason_));
    info.add(String(F("heap=")) + ESP.getFreeHeap() +
      F(" min=") + ESP.getMinFreeHeap() +
      F(" largest=") + heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    if (previousSnapshotValid_ && bootResetReason_ != ESP_RST_POWERON) {
      info.add(String(F("preResetHeap=")) + previousSnapshot_.freeHeap +
        F(" min=") + previousSnapshot_.minFreeHeap +
        F(" largest=") + previousSnapshot_.largestBlock +
        F(" ms=") + previousSnapshot_.uptimeMs +
        F(" content=") + previousSnapshot_.content);
    }
#endif
    if (adapter_.isAudioActive()) {
      info.add(String(adapter_.audioUsesFFT() ? F("content=audio FFT mode=") :
        F("content=audio LEVEL mode=")) + String(adapter_.audioMode() + 1));
    } else {
      info.add(adapter_.isClockActive() ? F("content=clock") :
        adapter_.isCountdownActive() ? F("content=countdown") :
        adapter_.isStopwatchActive() ? F("content=stopwatch") :
        adapter_.isScoreboardActive() ? F("content=scoreboard") :
        adapter_.isTextActive() ? F("content=text") :
        adapter_.isGifActive() ? F("content=gif") :
        adapter_.isRawImageActive() ? F("content=image") :
        adapter_.isLightEffectActive() ? F("content=light") :
        adapter_.isSolidActive() ? F("content=solid") :
        renderer_.isVisible() ? F("content=graffiti") : F("content=WLED"));
    }
  }

  void addToConfig(JsonObject& root) override {
    JsonObject config = root.createNestedObject(FPSTR(USERMOD_NAME));
    config[FPSTR(CFG_ENABLED)] = enabled_;
    config[FPSTR(CFG_SCREEN_TYPE)] = screenType_;
    // The settings page renders the fixed IDM- prefix outside the input. Store
    // only the editable suffix in the form/config; readFromConfig() accepts
    // both this format and legacy full names for backward compatibility.
    config[FPSTR(CFG_DEVICE_NAME)] = deviceName_.startsWith("IDM-")
      ? deviceName_.substring(4)
      : deviceName_;
#if IDOT_SCREEN_MAX_DIM > 16
    config[FPSTR(CFG_RESCALE)] = rescale_;
#endif
    config[FPSTR(CFG_BUZZER_PIN)] = buzzerPin_;
    config[FPSTR(CFG_BUZZER_ACTIVE_HIGH)] = buzzerActiveHigh_;
    config[FPSTR(CFG_AUDIO_SOURCE)] = static_cast<uint8_t>(audioSourceMode_);
  }

  bool readFromConfig(JsonObject& root) override {
    JsonObject config = root[FPSTR(USERMOD_NAME)];
    if (config.isNull()) return false;

    const int8_t previousBuzzerPin = buzzerPin_;
    const bool previousBuzzerActiveHigh = buzzerActiveHigh_;
    const bool previousEnabled = enabled_;
    const IDotMatrixAudioSourceMode previousAudioSource = audioSourceMode_;
    uint8_t audioSourceRaw = static_cast<uint8_t>(audioSourceMode_);

    bool complete = true;
    complete &= getJsonValue(config[FPSTR(CFG_ENABLED)], enabled_, true);
    complete &= getJsonValue(config[FPSTR(CFG_SCREEN_TYPE)], screenType_, uint8_t(IDOT_DEFAULT_SCREEN_TYPE));
    complete &= getJsonValue(config[FPSTR(CFG_DEVICE_NAME)], deviceName_, defaultDeviceName());
#if IDOT_SCREEN_MAX_DIM > 16
    complete &= getJsonValue(config[FPSTR(CFG_RESCALE)], rescale_, false);
#else
    // A 16x16-only build has no alternative logical profile. Ignore and
    // remove any stale rescale setting inherited from a larger firmware.
    rescale_ = false;
#endif
    complete &= getJsonValue(config[FPSTR(CFG_BUZZER_PIN)], buzzerPin_, int8_t(-1));
    complete &= getJsonValue(config[FPSTR(CFG_BUZZER_ACTIVE_HIGH)], buzzerActiveHigh_, true);
    complete &= getJsonValue(config[FPSTR(CFG_AUDIO_SOURCE)], audioSourceRaw, uint8_t(0));

    audioSourceMode_ = IDotMatrixAudioSource::normalizeMode(audioSourceRaw);
    screenType_ = IDotMatrixBuildProfile::normalizeScreenType(screenType_);
    deviceName_ = normalizedDeviceName(deviceName_);
    adapter_.setRescaleEnabled(rescale_);
    if (previousAudioSource != audioSourceMode_) {
      audioSourceNextPollAt_ = 0;
      audioReactiveDataAvailable_ = false;
      adapter_.setAudioDataOverride(audioSourceMode_ == IDotMatrixAudioSourceMode::AudioReactive);
    }
    if (setupComplete_ && previousEnabled != enabled_) runtimeRestartRequired_ = true;
    if (setupComplete_ &&
        (previousBuzzerPin != buzzerPin_ ||
         previousBuzzerActiveHigh != buzzerActiveHigh_ ||
         previousEnabled != enabled_)) {
      const int8_t requestedPin = buzzerPin_;
      const bool requestedActiveHigh = buzzerActiveHigh_;
      buzzerPin_ = previousBuzzerPin;
      buzzerActiveHigh_ = previousBuzzerActiveHigh;
      teardownBuzzerHardware();
      buzzerPin_ = requestedPin;
      buzzerActiveHigh_ = requestedActiveHigh;
      setupBuzzerHardware();
    }
    // Derive this status from the configuration currently active in the BLE
    // stack. Comparing with the value held before readFromConfig() made the
    // flag sticky and could report a restart even after a successful boot.
    bleRestartRequired_ = ble_.isInitialized() &&
      (deviceName_ != ble_.deviceName() || screenType_ != ble_.screenType());
    return complete;
  }

  void appendConfigData() override {
    oappend(F("dd=addDropdown('iDotMatrix','screenType');"));
    oappend(F("addOption(dd,'16 x 16',1);"));
#if IDOT_SCREEN_MAX_DIM >= 32
    oappend(F("addOption(dd,'32 x 32',3);"));
#endif
#if IDOT_SCREEN_MAX_DIM >= 64
    oappend(F("addOption(dd,'64 x 64',4);"));
#endif
    oappend(F("dd=addDropdown('iDotMatrix','audioSource');"));
    oappend(F("addOption(dd,'Phone / BLE',0);"));
    oappend(F("addOption(dd,'WLED AudioReactive',1);"));
    oappend(F("addOption(dd,'Auto (AudioReactive, then Phone)',2);"));
    oappend(F("(()=>{const s=(k,o,n)=>{let e=document.querySelector('[name=\"iDotMatrix:'+k+'\"]');if(!e)return;if(e.id){let l=document.querySelector('label[for=\"'+e.id+'\"]');if(l){l.textContent=n;return;}}for(let p=e.parentElement,d=0;p&&d<5;p=p.parentElement,d++){for(let x of p.childNodes)if(x.nodeType===3&&x.nodeValue.trim()===o){x.nodeValue=x.nodeValue.replace(o,n);return;}for(let l of p.querySelectorAll('label,span,td'))if(l.children.length===0&&l.textContent.trim()===o){l.textContent=n;return;}}};s('enabled','Enabled','Enabled:');s('screenType','ScreenType','ScreenType:');s('deviceName','DeviceName','DeviceName:');s('rescale','Rescale','Scale the logical profile to the selected WLED 2D segment:');s('buzzer-pin','Buzzer Pin','Buzzer Pin:');s('buzzerActiveHigh','BuzzerActiveHigh','BuzzerActiveHigh:');s('audioSource','AudioSource','Audio Source:');})();"));
#if IDOT_SCREEN_MAX_DIM > 16
    oappend(F("addInfo('iDotMatrix:screenType',1,'<div style=\"color:#fa0;font-style:italic;margin-top:8px\">Change requires reboot and app reconnection.</div>');"));
#endif
    oappend(F("(()=>{let e=document.querySelector('[name=\"iDotMatrix:deviceName\"]');if(!e){let r=[...document.querySelectorAll('tr')].find(x=>x.cells&&x.cells[0]&&x.cells[0].textContent.trim()==='DeviceName');e=r&&r.querySelector('input');}if(e&&!document.getElementById('idotmatrix-prefix'))e.insertAdjacentHTML('beforebegin','<span id=\"idotmatrix-prefix\">IDM-</span>');})();"));
    oappend(F("addInfo('iDotMatrix:enabled',1,'<div style=\"color:#fa0;font-style:italic;margin-top:8px\">Changing Enabled requires reboot.</div>');"));
    oappend(F("addInfo('iDotMatrix:deviceName',1,'<div style=\"color:#fa0;font-style:italic;margin-top:8px\">Change requires reboot and app reconnection.</div>');"));
    oappend(F("addInfo('iDotMatrix:audioSource',1,'<div style=\"color:#fa0;font-style:italic;margin-top:8px\">AudioReactive uses WLED Usermod data when that Usermod is compiled and enabled. Auto falls back to Phone / BLE; explicit AudioReactive becomes silent if local audio data is unavailable.</div>');"));
    oappend(F("addInfo('iDotMatrix:buzzerActiveHigh',1,'<button type=\"button\" onclick=\"fetch(&quot;/idotmatrix/buzzer-test&quot;,{method:&quot;POST&quot;}).then(async r=>{if(!r.ok)alert(await r.text())}).catch(()=>alert(&quot;Buzzer test failed&quot;))\">Test buzzer</button><div style=\"color:#fa0;font-style:italic;margin-top:8px\">Save before testing.</div>');"));
  }
};

static IDotMatrixUsermod idotMatrixUsermod;
REGISTER_USERMOD(idotMatrixUsermod);
