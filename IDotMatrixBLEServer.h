#pragma once

#include <Arduino.h>

// NimBLE-Arduino 2.x ships a version header; the 1.4.3 stable dependency does
// not.  Detect it at compile time so the WLED 16.0.1 classic-ESP32 profiles
// keep their proven NimBLE 1.x API while the supported ESP32-C3/IDF5 profile uses 2.x.
#if defined(__has_include)
#  if __has_include(<NimBLECppVersion.h>)
#    include <NimBLECppVersion.h>
#    if defined(NIMBLE_CPP_VERSION_MAJOR) && NIMBLE_CPP_VERSION_MAJOR >= 2
#      define IDOT_NIMBLE_V2_API 1
#    endif
#  endif
#endif

#include <NimBLEDevice.h>

#include "IDotMatrixBulkTransfer.h"
#include "IDotMatrixFA02Assembler.h"
#include "IDotMatrixProtocol.h"

class IDotMatrixBLEServer {
public:
  explicit IDotMatrixBLEServer(IDotMatrixProtocol& protocol) : protocol_(protocol) {}

  bool begin(const char* deviceName, uint8_t screenType);
  void loop();

  bool isInitialized() const { return initialized_; }
  bool isAdvertising() const { return advertising_; }
  bool isConnected() const { return connected_; }
  uint8_t screenType() const { return screenType_; }
  const char* deviceName() const { return deviceName_; }

private:
  static constexpr size_t RX_PACKET_MAX = 64;
  static constexpr uint8_t RX_QUEUE_SIZE = 4;
  // The reference reassembles fragmented FA02 writes before dispatch. Normal
  // media bulk stays near 4 KiB; alarm/program packets may grow to 8 KiB and
  // the assembler allocates that extra capacity only while such a packet exists.
  static constexpr size_t BULK_PACKET_MAX = IDotMatrixFA02Assembler::MAX_PACKET_SIZE;

  enum class RxChannel : uint8_t {
    FA02,
    AudioFA02,
    AE01
  };

  struct RxPacket {
    RxChannel channel;
    uint8_t length;
    uint8_t data[RX_PACKET_MAX];
  };

  class ServerCallbacks final : public NimBLEServerCallbacks {
  public:
    explicit ServerCallbacks(IDotMatrixBLEServer& owner) : owner_(owner) {}
#if defined(IDOT_NIMBLE_V2_API)
    void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override;
    void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) override;
    void onMTUChange(uint16_t mtu, NimBLEConnInfo& connInfo) override;
#else
    void onConnect(NimBLEServer* server) override;
    void onDisconnect(NimBLEServer* server) override;
    void onMTUChange(uint16_t mtu, ble_gap_conn_desc* desc) override;
#endif

  private:
    IDotMatrixBLEServer& owner_;
  };

  class WriteCallbacks final : public NimBLECharacteristicCallbacks {
  public:
    explicit WriteCallbacks(IDotMatrixBLEServer& owner) : owner_(owner) {}
#if defined(IDOT_NIMBLE_V2_API)
    void onWrite(NimBLECharacteristic* characteristic, NimBLEConnInfo& connInfo) override;
#else
    void onWrite(NimBLECharacteristic* characteristic) override;
#endif

  private:
    IDotMatrixBLEServer& owner_;
  };

  void onConnect();
  void onDisconnect();
  void onMTUChange(uint16_t mtu);
  void enqueueFromCallback(NimBLECharacteristic* characteristic);
  bool dequeue(RxPacket& packet);
  void processPacket(const RxPacket& packet);
  void processFA02Complete(const uint8_t* data, size_t length, IDotMatrixReply& reply);
  void processAE01(const uint8_t* data, size_t length, IDotMatrixReply& reply);
  void sendFA03(const uint8_t* data, size_t length);
  void startAdvertising();

  IDotMatrixProtocol& protocol_;
  IDotMatrixBulkTransfer bulkTransfer_;
  bool initialized_ = false;
  bool advertising_ = false;
  volatile bool connected_ = false;
  volatile bool connectionEventPending_ = false;
  uint8_t deviceInfoPushesRemaining_ = 0;
  uint32_t deviceInfoPushAt_ = 0;
  volatile bool restartAdvertising_ = false;
  uint32_t restartAdvertisingAt_ = 0;
  uint8_t screenType_ = 0x01;
  char deviceName_[32] = "IDM-000000";

  NimBLEServer* server_ = nullptr;
  NimBLECharacteristic* fa02_ = nullptr;
  NimBLECharacteristic* fa03_ = nullptr;
  NimBLECharacteristic* ae01_ = nullptr;
  NimBLECharacteristic* ae02_ = nullptr;

  ServerCallbacks serverCallbacks_{*this};
  WriteCallbacks writeCallbacks_{*this};

  portMUX_TYPE queueMux_ = portMUX_INITIALIZER_UNLOCKED;
  RxPacket rxQueue_[RX_QUEUE_SIZE]{};
  volatile uint8_t rxHead_ = 0;
  volatile uint8_t rxTail_ = 0;
  volatile uint8_t rxCount_ = 0;
  volatile bool audioStreamActive_ = false;
  IDotMatrixFA02Assembler faAssembler_;
  volatile uint32_t reassemblyLastWriteAt_ = 0;
  bool rawTransferReady_ = false;
  bool gifTransferReady_ = false;
};
