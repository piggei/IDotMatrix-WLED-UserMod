#include "IDotMatrixBLEServer.h"

#include <cstring>
#include <string>

namespace {
constexpr char FA_SERVICE_UUID[] = "000000fa-0000-1000-8000-00805f9b34fb";
constexpr char FA02_UUID[]       = "0000fa02-0000-1000-8000-00805f9b34fb";
constexpr char FA03_UUID[]       = "0000fa03-0000-1000-8000-00805f9b34fb";
constexpr char AE_SERVICE_UUID[] = "0000ae00-0000-1000-8000-00805f9b34fb";
constexpr char AE01_UUID[]       = "0000ae01-0000-1000-8000-00805f9b34fb";
constexpr char AE02_UUID[]       = "0000ae02-0000-1000-8000-00805f9b34fb";
}

bool IDotMatrixBLEServer::begin(const char* deviceName, uint8_t screenType) {
  if (initialized_) return true;

  if (deviceName != nullptr && deviceName[0] != '\0') {
    strlcpy(deviceName_, deviceName, sizeof(deviceName_));
  }

  if (screenType != 0x01 && screenType != 0x03 && screenType != 0x04) {
    screenType = 0x01;
  }
  screenType_ = screenType;

  NimBLEDevice::init(deviceName_);
  server_ = NimBLEDevice::createServer();
  if (server_ == nullptr) return false;
  server_->setCallbacks(&serverCallbacks_);

  NimBLEService* faService = server_->createService(FA_SERVICE_UUID);
  NimBLEService* aeService = server_->createService(AE_SERVICE_UUID);
  if (faService == nullptr || aeService == nullptr) return false;

  fa02_ = faService->createCharacteristic(
    FA02_UUID,
    NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  fa03_ = faService->createCharacteristic(
    FA03_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  ae01_ = aeService->createCharacteristic(
    AE01_UUID,
    NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  ae02_ = aeService->createCharacteristic(
    AE02_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );

  if (fa02_ == nullptr || fa03_ == nullptr || ae01_ == nullptr || ae02_ == nullptr) {
    return false;
  }

  fa02_->setCallbacks(&writeCallbacks_);
  ae01_->setCallbacks(&writeCallbacks_);
  faService->start();
  aeService->start();

  initialized_ = true;
  startAdvertising();
  return true;
}

void IDotMatrixBLEServer::loop() {
  if (!initialized_) return;

  if (restartAdvertising_ && int32_t(millis() - restartAdvertisingAt_) >= 0) {
    restartAdvertising_ = false;
    startAdvertising();
  }

  RxPacket packet;
  while (dequeue(packet)) {
    processPacket(packet);
  }
}

void IDotMatrixBLEServer::ServerCallbacks::onConnect(NimBLEServer* server) {
  (void)server;
  owner_.onConnect();
}

void IDotMatrixBLEServer::ServerCallbacks::onDisconnect(NimBLEServer* server) {
  (void)server;
  owner_.onDisconnect();
}

void IDotMatrixBLEServer::WriteCallbacks::onWrite(NimBLECharacteristic* characteristic) {
  owner_.enqueueFromCallback(characteristic);
}

void IDotMatrixBLEServer::onConnect() {
  connected_ = true;
  advertising_ = false;
}

void IDotMatrixBLEServer::onDisconnect() {
  connected_ = false;
  restartAdvertising_ = true;
  restartAdvertisingAt_ = millis() + 300;
}

void IDotMatrixBLEServer::enqueueFromCallback(NimBLECharacteristic* characteristic) {
  if (characteristic == nullptr) return;

  const std::string value = characteristic->getValue();
  if (value.empty()) return;

  const size_t copyLength = value.length() > RX_PACKET_MAX ? RX_PACKET_MAX : value.length();
  const RxChannel channel = characteristic == ae01_ ? RxChannel::AE01 : RxChannel::FA02;

  portENTER_CRITICAL(&queueMux_);
  ++receivedPackets_;
  if (rxCount_ >= RX_QUEUE_SIZE) {
    ++droppedPackets_;
    portEXIT_CRITICAL(&queueMux_);
    return;
  }

  RxPacket& packet = rxQueue_[rxHead_];
  packet.channel = channel;
  packet.length = static_cast<uint8_t>(copyLength);
  memcpy(packet.data, value.data(), copyLength);
  rxHead_ = (rxHead_ + 1) % RX_QUEUE_SIZE;
  ++rxCount_;
  portEXIT_CRITICAL(&queueMux_);
}

bool IDotMatrixBLEServer::dequeue(RxPacket& packet) {
  portENTER_CRITICAL(&queueMux_);
  if (rxCount_ == 0) {
    portEXIT_CRITICAL(&queueMux_);
    return false;
  }

  packet = rxQueue_[rxTail_];
  rxTail_ = (rxTail_ + 1) % RX_QUEUE_SIZE;
  --rxCount_;
  portEXIT_CRITICAL(&queueMux_);
  return true;
}

void IDotMatrixBLEServer::processPacket(const RxPacket& packet) {
  if (packet.channel == RxChannel::FA02) {
    processFA02(packet.data, packet.length);
  } else {
    processAE01(packet.data, packet.length);
  }
}

void IDotMatrixBLEServer::processFA02(const uint8_t* data, size_t length) {
  if (length < 4) return;

  const uint8_t command = data[2];
  const uint8_t subcommand = data[3];

  // Device information request.
  if (length == 4 && command == 0x01 && subcommand == 0x80) {
    sendDeviceInfo();
    return;
  }

  // App time synchronization. Time storage/rendering will be added later.
  if (length == 11 && command == 0x01 && subcommand == 0x80) {
    sendCommandStatus(command, subcommand, 0x01);
  }
}

void IDotMatrixBLEServer::processAE01(const uint8_t* data, size_t length) {
  // Reserved for the later streaming/media phase.
  (void)data;
  (void)length;
}

void IDotMatrixBLEServer::sendFA03(const uint8_t* data, size_t length) {
  if (!connected_ || fa03_ == nullptr || data == nullptr || length == 0) return;
  fa03_->setValue(data, length);
  fa03_->notify();
}

void IDotMatrixBLEServer::sendCommandStatus(
  uint8_t command,
  uint8_t subcommand,
  uint8_t status
) {
  const uint8_t response[] = {0x05, 0x00, command, subcommand, status};
  sendFA03(response, sizeof(response));
}

void IDotMatrixBLEServer::sendDeviceInfo() {
  const uint8_t response[] = {
    0x09, 0x00, 0x01, 0x80, 0x04, 0x0E, 0x01, screenType_, 0x00
  };
  sendFA03(response, sizeof(response));
}

void IDotMatrixBLEServer::startAdvertising() {
  if (!initialized_ || connected_) return;

  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  if (advertising == nullptr) return;

  if (advertising_) {
    advertising->stop();
    advertising_ = false;
  }

  NimBLEAdvertisementData advertisementData;
  advertisementData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
  advertisementData.setName(deviceName_);
  // Use the 16-bit representation in the packet. The full GATT UUID is the
  // same Bluetooth-base UUID, while the short form keeps advertising <31 B.
  advertisementData.setCompleteServices(NimBLEUUID(uint16_t(0x00FA)));

  const char manufacturerData[] = {
    static_cast<char>(0x54),
    static_cast<char>(0x52),
    static_cast<char>(0x00),
    static_cast<char>(0x70),
    static_cast<char>(screenType_)
  };
  advertisementData.setManufacturerData(
    std::string(manufacturerData, sizeof(manufacturerData))
  );
  advertising->setAdvertisementData(advertisementData);

  NimBLEAdvertisementData scanResponseData;
  scanResponseData.setCompleteServices(NimBLEUUID(uint16_t(0xAE00)));
  advertising->setScanResponseData(scanResponseData);
  advertising->start();
  advertising_ = true;
}
