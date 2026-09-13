#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

class NimBLEServer;
class NimBLECharacteristic;
class NimBLEConnInfo {};
struct ble_gap_conn_desc {};

namespace NIMBLE_PROPERTY {
  enum : uint16_t { READ=1, WRITE=2, WRITE_NR=4, NOTIFY=8 };
}
constexpr uint8_t BLE_HS_ADV_F_DISC_GEN = 0x02;
constexpr uint8_t BLE_HS_ADV_F_BREDR_UNSUP = 0x04;

class NimBLEUUID {
public:
  explicit NimBLEUUID(uint16_t) {}
};
class NimBLEServerCallbacks {
public:
  virtual ~NimBLEServerCallbacks() = default;
#if defined(IDOT_NIMBLE_V2_API)
  virtual void onConnect(NimBLEServer*, NimBLEConnInfo&) {}
  virtual void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) {}
  virtual void onMTUChange(uint16_t, NimBLEConnInfo&) {}
#else
  virtual void onConnect(NimBLEServer*) {}
  virtual void onDisconnect(NimBLEServer*) {}
  virtual void onMTUChange(uint16_t, ble_gap_conn_desc*) {}
#endif
};
class NimBLECharacteristicCallbacks {
public:
  virtual ~NimBLECharacteristicCallbacks() = default;
#if defined(IDOT_NIMBLE_V2_API)
  virtual void onWrite(NimBLECharacteristic*, NimBLEConnInfo&) {}
#else
  virtual void onWrite(NimBLECharacteristic*) {}
#endif
};
class NimBLECharacteristic {
public:
  void setCallbacks(NimBLECharacteristicCallbacks*) {}
  std::string getValue() const { return {}; }
  void setValue(const uint8_t*, size_t) {}
  void notify() {}
};
class NimBLEService {
public:
  NimBLECharacteristic* createCharacteristic(const char*, uint16_t) { return &ch_; }
  void start() {}
private:
  NimBLECharacteristic ch_;
};
class NimBLEAdvertisementData {
public:
  void setFlags(uint8_t) {}
  void setName(const char*) {}
  void setCompleteServices(const NimBLEUUID&) {}
  void setManufacturerData(const std::string&) {}
};
class NimBLEAdvertising {
public:
  void stop() {}
  void setAdvertisementData(const NimBLEAdvertisementData&) {}
  void setScanResponseData(const NimBLEAdvertisementData&) {}
  void enableScanResponse(bool) {}
#if defined(IDOT_NIMBLE_V2_API)
  bool start() { return true; }
#else
  void start() {}
#endif
};
class NimBLEServer {
public:
#if defined(IDOT_NIMBLE_V2_API)
  void setCallbacks(NimBLEServerCallbacks*, bool) {}
  bool start() { return true; }
#else
  void setCallbacks(NimBLEServerCallbacks*) {}
#endif
  NimBLEService* createService(const char*) { return &service_; }
private:
  NimBLEService service_;
};
class NimBLEDevice {
public:
#if defined(IDOT_NIMBLE_V2_API)
  static bool init(const char*) { return true; }
#else
  static void init(const char*) {}
#endif
  static void setMTU(uint16_t) {}
  static NimBLEServer* createServer() { static NimBLEServer s; return &s; }
  static NimBLEAdvertising* getAdvertising() { static NimBLEAdvertising a; return &a; }
};
