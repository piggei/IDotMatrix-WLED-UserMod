#pragma once

#include <Arduino.h>
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
  uint32_t receivedPackets() const { return receivedPackets_; }
  uint32_t droppedPackets() const { return droppedPackets_; }
  uint32_t deviceInfoPushAttempts() const { return deviceInfoPushAttempts_; }
  uint32_t bulkChunks() const { return bulkTransfer_.acceptedChunks(); }
  uint32_t bulkCompleted() const { return bulkTransfer_.completedTransfers(); }
  uint32_t bulkCRCErrors() const { return bulkTransfer_.crcErrors(); }
  uint32_t bulkRejected() const { return bulkTransfer_.rejectedTransfers(); }
  size_t textPayloadLength() const { return bulkTransfer_.textPayloadLength(); }
  uint32_t textParsed() const { return textParsed_; }
  uint32_t textParseErrors() const { return textParseErrors_; }
  uint32_t fragmentedWrites() const { return fragmentedWrites_; }
  uint32_t reassemblyErrors() const { return reassemblyErrors_; }
  uint16_t reassemblyExpected() const { return faAssembler_.expected(); }
  uint16_t reassemblyReceived() const { return faAssembler_.received(); }
  uint32_t unknownPackets() const { return unknownPackets_; }
  uint16_t lastUnknownLength() const { return lastUnknownLength_; }
  const uint8_t* lastUnknownData() const { return lastUnknownData_; }
  uint8_t lastUnknownStored() const { return lastUnknownStored_; }

private:
  static constexpr size_t RX_PACKET_MAX = 64;
  static constexpr uint8_t RX_QUEUE_SIZE = 4;
  // The reference reassembles fragmented FA02 writes before dispatching bulk.
  // Observed chunks are up to 4096 payload bytes; retain the 16-byte header.
  static constexpr size_t BULK_PACKET_MAX = IDotMatrixFA02Assembler::MAX_PACKET_SIZE;
  static constexpr uint8_t UNKNOWN_BYTES = 12;

  enum class RxChannel : uint8_t {
    FA02,
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
    void onConnect(NimBLEServer* server) override;
    void onDisconnect(NimBLEServer* server) override;

  private:
    IDotMatrixBLEServer& owner_;
  };

  class WriteCallbacks final : public NimBLECharacteristicCallbacks {
  public:
    explicit WriteCallbacks(IDotMatrixBLEServer& owner) : owner_(owner) {}
    void onWrite(NimBLECharacteristic* characteristic) override;

  private:
    IDotMatrixBLEServer& owner_;
  };

  void onConnect();
  void onDisconnect();
  void enqueueFromCallback(NimBLECharacteristic* characteristic);
  bool dequeue(RxPacket& packet);
  void processPacket(const RxPacket& packet);
  void processFA02Complete(const uint8_t* data, size_t length, IDotMatrixReply& reply);
  void processAE01(const uint8_t* data, size_t length, IDotMatrixReply& reply);
  void recordUnknown(const uint8_t* data, size_t length);
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
  uint32_t deviceInfoPushAttempts_ = 0;
  volatile bool restartAdvertising_ = false;
  uint32_t restartAdvertisingAt_ = 0;
  uint8_t screenType_ = 0x01;
  char deviceName_[32] = "IDM-858931";

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
  volatile uint32_t receivedPackets_ = 0;
  volatile uint32_t droppedPackets_ = 0;
  IDotMatrixFA02Assembler faAssembler_;
  volatile uint32_t fragmentedWrites_ = 0;
  volatile uint32_t reassemblyErrors_ = 0;
  volatile uint32_t unknownPackets_ = 0;
  uint32_t textParsed_ = 0;
  uint32_t textParseErrors_ = 0;
  uint16_t lastUnknownLength_ = 0;
  uint8_t lastUnknownStored_ = 0;
  uint8_t lastUnknownData_[UNKNOWN_BYTES]{};
};
