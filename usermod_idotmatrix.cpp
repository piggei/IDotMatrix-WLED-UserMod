#include "wled.h"
#include "IDotMatrixBLEServer.h"

namespace {
const char USERMOD_NAME[] PROGMEM = "iDotMatrix";
const char CFG_ENABLED[] PROGMEM = "enabled";
const char CFG_SCREEN_TYPE[] PROGMEM = "screenType";
const char CFG_DEVICE_NAME[] PROGMEM = "deviceName";
}

class IDotMatrixUsermod final : public Usermod {
private:
  bool enabled_ = true;
  uint8_t screenType_ = 0x01;
  String deviceName_ = "IDM-858931";
  IDotMatrixBLEServer ble_;
  bool blockedByRmt_ = false;
  bool startPending_ = false;
  uint32_t startAt_ = 0;

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
        DEBUG_PRINTLN(F("[iDotMatrix] NimBLE server and advertising initialized"));
      } else {
        DEBUG_PRINTLN(F("[iDotMatrix] NimBLE server initialization failed"));
      }
    }

    ble_.loop();
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

    info.add(startPending_ ? F("BLE advertising pending") : ble_.isInitialized()
      ? (ble_.isConnected() ? F("BLE connected") : ble_.isAdvertising()
        ? F("BLE advertising") : F("BLE ready, not advertising"))
      : F("BLE init failed"));
    info.add(String(F("profile=0x")) + String(ble_.screenType(), HEX));
    info.add(String(F("rx=")) + String(ble_.receivedPackets()));
    info.add(String(F("dropped=")) + String(ble_.droppedPackets()));
  }

  void addToConfig(JsonObject& root) override {
    JsonObject config = root.createNestedObject(FPSTR(USERMOD_NAME));
    config[FPSTR(CFG_ENABLED)] = enabled_;
    config[FPSTR(CFG_SCREEN_TYPE)] = screenType_;
    config[FPSTR(CFG_DEVICE_NAME)] = deviceName_;
  }

  bool readFromConfig(JsonObject& root) override {
    JsonObject config = root[FPSTR(USERMOD_NAME)];
    if (config.isNull()) return false;

    bool complete = true;
    complete &= getJsonValue(config[FPSTR(CFG_ENABLED)], enabled_, true);
    complete &= getJsonValue(config[FPSTR(CFG_SCREEN_TYPE)], screenType_, uint8_t(0x01));
    complete &= getJsonValue(config[FPSTR(CFG_DEVICE_NAME)], deviceName_, String("IDM-858931"));

    if (screenType_ != 0x01 && screenType_ != 0x03 && screenType_ != 0x04) {
      screenType_ = 0x01;
    }
    if (deviceName_.isEmpty()) deviceName_ = F("IDM-858931");
    return complete;
  }
};

static IDotMatrixUsermod idotMatrixUsermod;
REGISTER_USERMOD(idotMatrixUsermod);
