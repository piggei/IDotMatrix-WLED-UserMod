#include "wled.h"
#include "IDotMatrixBLEServer.h"
#include "IDotMatrixProtocol.h"
#include "IDotMatrixRenderer.h"
#include "IDotMatrixMedia.h"
#include "IDotMatrixWLEDAdapter.h"

namespace {
const char USERMOD_NAME[] PROGMEM = "iDotMatrix";
const char CFG_ENABLED[] PROGMEM = "enabled";
const char CFG_SCREEN_TYPE[] PROGMEM = "screenType";
const char CFG_DEVICE_NAME[] PROGMEM = "deviceName";
const char CFG_RESCALE[] PROGMEM = "rescale";
}

class IDotMatrixUsermod final : public Usermod {
private:
  bool enabled_ = true;
  uint8_t screenType_ = 0x01;
  String deviceName_ = "IDM-858931";
  bool rescale_ = false;
  IDotMatrixRenderer renderer_;
  IDotMatrixMedia media_{renderer_};
  IDotMatrixWLEDAdapter adapter_{renderer_, &media_};
  IDotMatrixProtocol protocol_{adapter_};
  IDotMatrixBLEServer ble_{protocol_};
  bool blockedByRmt_ = false;
  bool startPending_ = false;
  uint32_t startAt_ = 0;
  bool bleRestartRequired_ = false;

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
    if (clean.isEmpty()) clean = F("858931");
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

public:
  void setup() override {
    if (!enabled_) return;

    // WLED's ESP32 RMT-HI LED driver shares a high-level interrupt with the
    // Bluetooth controller. With this framework they are assigned to
    // different CPU cores, which causes an immediate boot loop. I2S is the
    // supported LED backend for this BLE build.
    blockedByRmt_ = hasDigitalRmtBus();
    if (blockedByRmt_) {
      DEBUG_PRINTLN(F("[iDotMatrix] BLE blocked: select I2S for every digital LED output"));
      return;
    }

    // ESP-IDF requires Wi-Fi modem sleep while Wi-Fi and Bluetooth coexist.
    // WLED defaults noWifiSleep to true on ESP32, which makes the Wi-Fi task
    // intentionally abort after Bluetooth has been enabled.
    noWifiSleep = false;

    if (!renderer_.begin(screenType_)) {
      DEBUG_PRINTLN(F("[iDotMatrix] logical framebuffer allocation failed"));
    }
    adapter_.setRescaleEnabled(rescale_);

    if (adapter_.registerDisplayEffect()) {
      DEBUG_PRINTF_P(
        PSTR("[iDotMatrix] display effect registered as id %u\n"),
        adapter_.displayEffectId()
      );
    } else {
      DEBUG_PRINTLN(F("[iDotMatrix] display effect registration failed"));
    }

    // Let WLED complete its first Wi-Fi initialization pass before starting
    // the lower-memory NimBLE host.
    startPending_ = true;
    startAt_ = millis() + 5000;
  }

  void loop() override {
    if (!enabled_ || blockedByRmt_) return;

    if (startPending_ && int32_t(millis() - startAt_) >= 0) {
      startPending_ = false;
      if (ble_.begin(deviceName_.c_str(), screenType_)) {
        bleRestartRequired_ = false;
        DEBUG_PRINTLN(F("[iDotMatrix] NimBLE server and advertising initialized"));
      } else {
        DEBUG_PRINTLN(F("[iDotMatrix] NimBLE server initialization failed"));
      }
    }

    ble_.loop();


    media_.loop(millis());
  }

  void addToJsonInfo(JsonObject& root) override {
    JsonObject user = root["u"];
    if (user.isNull()) user = root.createNestedObject("u");

    JsonArray info = user.createNestedArray(FPSTR(USERMOD_NAME));
    if (!enabled_) {
      info.add(F("Disabled"));
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
    info.add(String(F("profile=")) + String(renderer_.width()) + 'x' + String(renderer_.height()));
    info.add(String(F("name=")) + deviceName_);
    if (bleRestartRequired_) info.add(F("Restart required after name/profile change"));
    info.add(adapter_.isClockActive() ? F("content=clock") :
      adapter_.isTextActive() ? F("content=text") :
      adapter_.isGifActive() ? F("content=gif") :
      adapter_.isRawImageActive() ? F("content=image") :
      renderer_.isVisible() ? F("content=graffiti") : F("content=WLED"));
  }

  void addToConfig(JsonObject& root) override {
    JsonObject config = root.createNestedObject(FPSTR(USERMOD_NAME));
    config[FPSTR(CFG_ENABLED)] = enabled_;
    config[FPSTR(CFG_SCREEN_TYPE)] = screenType_;
    config[FPSTR(CFG_DEVICE_NAME)] = deviceName_;
    config[FPSTR(CFG_RESCALE)] = rescale_;
  }

  bool readFromConfig(JsonObject& root) override {
    JsonObject config = root[FPSTR(USERMOD_NAME)];
    if (config.isNull()) return false;

    bool complete = true;
    complete &= getJsonValue(config[FPSTR(CFG_ENABLED)], enabled_, true);
    complete &= getJsonValue(config[FPSTR(CFG_SCREEN_TYPE)], screenType_, uint8_t(0x01));
    complete &= getJsonValue(config[FPSTR(CFG_DEVICE_NAME)], deviceName_, String("IDM-858931"));
    complete &= getJsonValue(config[FPSTR(CFG_RESCALE)], rescale_, false);

    if (screenType_ != 0x01 && screenType_ != 0x03 && screenType_ != 0x04) {
      screenType_ = 0x01;
    }
    deviceName_ = normalizedDeviceName(deviceName_);
    adapter_.setRescaleEnabled(rescale_);
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
    oappend(F("addOption(dd,'32 x 32',3);"));
    oappend(F("addOption(dd,'64 x 64',4);"));
    oappend(F("addInfo('iDotMatrix:screenType',1,'BLE logical profile; changing it requires reboot.');"));
    oappend(F("addInfo('iDotMatrix:deviceName',1,'IDM- is added automatically; changing it requires reboot.');"));
    oappend(F("addInfo('iDotMatrix:rescale',1,'Scale the logical profile to the selected WLED 2D segment.');"));
  }
};

static IDotMatrixUsermod idotMatrixUsermod;
REGISTER_USERMOD(idotMatrixUsermod);
