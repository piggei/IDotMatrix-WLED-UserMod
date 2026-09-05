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
  protocol_.setScreenType(screenType_);

  NimBLEDevice::init(deviceName_);
  // Match the standalone emulator as closely as NimBLE allows.  The peer still
  // chooses the final negotiated MTU, but advertising the maximum local MTU
  // prevents an unnecessarily small server-side ceiling.
  NimBLEDevice::setMTU(517);
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

  // Apply WLED state changes from its main loop, never from the NimBLE task.
  if (connectionEventPending_) {
    connectionEventPending_ = false;
    if (connected_) {
      protocol_.onConnected();
      deviceInfoPushesRemaining_ = 2;
      deviceInfoPushAt_ = millis() + 1200;
    } else {
      bulkTransfer_.reset();
      protocol_.completeRawImage(false);
      protocol_.completeGif(false);
      rawTransferReady_ = false;
      gifTransferReady_ = false;
      portENTER_CRITICAL(&queueMux_);
      faAssembler_.reset();
      portEXIT_CRITICAL(&queueMux_);
    }
  }

  if (deviceInfoPushesRemaining_ > 0 && int32_t(millis() - deviceInfoPushAt_) >= 0) {
    if (connected_) {
      IDotMatrixReply reply;
      protocol_.makeDeviceInfoReply(reply);
      sendFA03(reply.data, reply.length);
      --deviceInfoPushesRemaining_;
      if (deviceInfoPushesRemaining_ > 0) deviceInfoPushAt_ = millis() + 1300;
    } else {
      deviceInfoPushesRemaining_ = 0;
    }
  }

  if (restartAdvertising_ && int32_t(millis() - restartAdvertisingAt_) >= 0) {
    restartAdvertising_ = false;
    startAdvertising();
  }

  RxPacket packet;
  while (dequeue(packet)) {
    processPacket(packet);
  }

  // Never let an abandoned fragmented media packet poison the characteristic
  // indefinitely.  A reconnect used to be the only way to clear this state.
  if (faAssembler_.expected() > 0 && !faAssembler_.complete() &&
      uint32_t(millis() - reassemblyLastWriteAt_) >= 5000u) {
    portENTER_CRITICAL(&queueMux_);
    faAssembler_.reset();
    portEXIT_CRITICAL(&queueMux_);
    bulkTransfer_.reset();
    protocol_.completeRawImage(false);
    protocol_.completeGif(false);
    rawTransferReady_ = false;
    gifTransferReady_ = false;
  }

  // The standalone reference assembles FA02 fragments until the length in the
  // first two bytes is complete, then dispatches the logical packet. The app
  // waits for an ACK before starting its next bulk packet, so one hand-off slot
  // is sufficient and avoids a multi-slot 4 KiB queue.
  if (faAssembler_.complete()) {
    IDotMatrixReply reply;
    processFA02Complete(faAssembler_.data(), faAssembler_.expected(), reply);
    portENTER_CRITICAL(&queueMux_);
    faAssembler_.reset();
    portEXIT_CRITICAL(&queueMux_);
    if (reply.available()) sendFA03(reply.data, reply.length);
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

void IDotMatrixBLEServer::ServerCallbacks::onMTUChange(
  uint16_t mtu, ble_gap_conn_desc* desc
) {
  (void)desc;
  owner_.onMTUChange(mtu);
}

void IDotMatrixBLEServer::WriteCallbacks::onWrite(NimBLECharacteristic* characteristic) {
  owner_.enqueueFromCallback(characteristic);
}

void IDotMatrixBLEServer::onConnect() {
  connected_ = true;
  advertising_ = false;
  connectionEventPending_ = true;
}

void IDotMatrixBLEServer::onDisconnect() {
  connected_ = false;
  connectionEventPending_ = true;
  restartAdvertising_ = true;
  restartAdvertisingAt_ = millis() + 300;
}

void IDotMatrixBLEServer::onMTUChange(uint16_t mtu) {
  (void)mtu;
}

void IDotMatrixBLEServer::enqueueFromCallback(NimBLECharacteristic* characteristic) {
  if (characteristic == nullptr) return;

  const std::string value = characteristic->getValue();
  if (value.empty()) return;

  const RxChannel channel = characteristic == ae01_ ? RxChannel::AE01 : RxChannel::FA02;

  portENTER_CRITICAL(&queueMux_);
  if (channel == RxChannel::FA02) {
    if (faAssembler_.complete()) {
      portEXIT_CRITICAL(&queueMux_);
      return;
    }

    if (faAssembler_.expected() == 0) {
      if (value.length() < 2) {
        portEXIT_CRITICAL(&queueMux_);
        return;
      }
      const uint16_t declaredLength = uint16_t(uint8_t(value[0])) |
        (uint16_t(uint8_t(value[1])) << 8);
      if (declaredLength == 0 || declaredLength > BULK_PACKET_MAX) {
        portEXIT_CRITICAL(&queueMux_);
        return;
      }

      // Preserve the original four-entry queue for complete short commands so
      // rapid power/brightness/colour writes do not contend for the bulk slot.
      if (declaredLength <= RX_PACKET_MAX && value.length() >= declaredLength) {
        if (rxCount_ >= RX_QUEUE_SIZE) {
          portEXIT_CRITICAL(&queueMux_);
          return;
        }
        RxPacket& packet = rxQueue_[rxHead_];
        packet.channel = RxChannel::FA02;
        packet.length = static_cast<uint8_t>(declaredLength);
        memcpy(packet.data, value.data(), declaredLength);
        rxHead_ = (rxHead_ + 1) % RX_QUEUE_SIZE;
        ++rxCount_;
        portEXIT_CRITICAL(&queueMux_);
        return;
      }

    }

    const IDotMatrixFA02Assembler::Result result = faAssembler_.append(
      reinterpret_cast<const uint8_t*>(value.data()),
      value.length()
    );
    if (result == IDotMatrixFA02Assembler::Result::Invalid ||
        result == IDotMatrixFA02Assembler::Result::Busy) {
      // Malformed or overlapping fragments are discarded; the app can retry.
    } else if (result == IDotMatrixFA02Assembler::Result::Accumulating) {
      reassemblyLastWriteAt_ = millis();
      // Do not emit a protocol ACK for an ATT fragment.  The verified standalone
      // emulator only ACKs after the complete logical FA02 packet is assembled.
    }
    portEXIT_CRITICAL(&queueMux_);
    return;
  }

  // AE01 is retained because it belongs to the emulated GATT database, but the
  // BUILD 80 reference only logs it. No implemented content path uses it.
  if (rxCount_ >= RX_QUEUE_SIZE) {
    portEXIT_CRITICAL(&queueMux_);
    return;
  }

  RxPacket& packet = rxQueue_[rxHead_];
  packet.channel = channel;
  packet.length = static_cast<uint8_t>(value.length() > RX_PACKET_MAX
    ? RX_PACKET_MAX
    : value.length());
  memcpy(packet.data, value.data(), packet.length);
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
    IDotMatrixReply reply;
    processFA02Complete(packet.data, packet.length, reply);
    if (reply.available()) sendFA03(reply.data, reply.length);
  } else {
    IDotMatrixReply reply;
    processAE01(packet.data, packet.length, reply);
    if (reply.available()) sendFA03(reply.data, reply.length);
  }
}

void IDotMatrixBLEServer::processFA02Complete(
  const uint8_t* data,
  size_t length,
  IDotMatrixReply& reply
) {
  reply.length = 0;
  // Newly observed app format: a complete PNG is wrapped in a compact
  // 9-byte type-0 envelope rather than the common 16-byte CRC bulk header.
  if (protocol_.processInlinePng(data, length, reply)) {
    return;
  }
  IDotMatrixBulkResult bulkResult;
  if (bulkTransfer_.processPacket(data, length, bulkResult)) {
    if (bulkResult.aborted) {
      protocol_.completeRawImage(false);
      protocol_.completeGif(false);
      rawTransferReady_ = false;
      gifTransferReady_ = false;
    }

    if (bulkResult.type == 0x01) {
      if (bulkResult.began) {
        gifTransferReady_ = protocol_.beginGif(bulkResult.totalLength);
      }
      if (bulkResult.chunkLength > 0 && gifTransferReady_) {
        gifTransferReady_ = protocol_.writeGif(
          bulkResult.chunkOffset, bulkResult.chunkData, bulkResult.chunkLength
        );
      }
      if (bulkResult.completed) {
        protocol_.completeGif(bulkResult.crcValid && gifTransferReady_);
        gifTransferReady_ = false;
      }
    } else if (bulkResult.type == 0x02) {
      if (bulkResult.began) {
        // The transfer's total length is validated by the renderer when the
        // first chunk arrives. For sequential RAW data, its final byte count
        // must match the logical RGB framebuffer exactly.
        rawTransferReady_ = protocol_.beginRawImage(bulkResult.totalLength);
      }
      if (bulkResult.chunkLength > 0 && rawTransferReady_) {
        rawTransferReady_ = protocol_.writeRawImage(
          bulkResult.chunkOffset,
          bulkResult.chunkData,
          bulkResult.chunkLength
        );
      }
      if (bulkResult.completed) {
        protocol_.completeRawImage(bulkResult.crcValid && rawTransferReady_);
        rawTransferReady_ = false;
      }
    } else if (bulkResult.type == 0x03 && bulkResult.completed && bulkResult.crcValid) {
      protocol_.processTextPayload(
        bulkTransfer_.textPayload(),
        bulkTransfer_.textPayloadLength()
      );
    }
    if (bulkResult.replyAvailable) {
      const uint8_t response[] = {
        0x05, 0x00, bulkResult.type, 0x00, bulkResult.status
      };
      memcpy(reply.data, response, sizeof(response));
      reply.length = sizeof(response);
    }
    return;
  }

  if (!protocol_.processFA02(data, length, reply)) {
    // BUILD 80 acknowledges unknown short commands. Preserve that tolerant
    // behavior so an app revision can continue, while deliberately not
    // acknowledging unsupported GIF bulk packets as if they were handled.
    if (length >= 4 && length <= RX_PACKET_MAX) {
      const uint8_t response[] = {0x05, 0x00, data[2], data[3], 0x01};
      memcpy(reply.data, response, sizeof(response));
      reply.length = sizeof(response);
    }
  }
}

void IDotMatrixBLEServer::processAE01(
  const uint8_t* data,
  size_t length,
  IDotMatrixReply& reply
) {
  reply.length = 0;
  (void)data;
  (void)length;
}

void IDotMatrixBLEServer::sendFA03(const uint8_t* data, size_t length) {
  if (!connected_ || fa03_ == nullptr || data == nullptr || length == 0) return;
  fa03_->setValue(data, length);
  fa03_->notify();
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
