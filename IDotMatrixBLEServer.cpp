#include "IDotMatrixBLEServer.h"
#include "IDotMatrixBLEFraming.h"

#include <cstdlib>
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

#if defined(IDOT_NIMBLE_V2_API)
  if (!NimBLEDevice::init(deviceName_)) return false;
#else
  NimBLEDevice::init(deviceName_);
#endif
  // Match the standalone emulator as closely as NimBLE allows.  The peer still
  // chooses the final negotiated MTU, but advertising the maximum local MTU
  // prevents an unnecessarily small server-side ceiling.
  NimBLEDevice::setMTU(517);
  server_ = NimBLEDevice::createServer();
  if (server_ == nullptr) return false;
#if defined(IDOT_NIMBLE_V2_API)
  // NimBLE 2.x owns callbacks by default. These callback objects are members,
  // so explicitly disable ownership/deletion.
  server_->setCallbacks(&serverCallbacks_, false);
#else
  server_->setCallbacks(&serverCallbacks_);
#endif

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
#if defined(IDOT_NIMBLE_V2_API)
  // NimBLE 2.x starts the complete GATT database from the server. Individual
  // NimBLEService::start() calls are deprecated/no-ops there.
  if (!server_->start()) return false;
#else
  faService->start();
  aeService->start();
#endif

  initialized_ = true;
  startAdvertising();
  return true;
}

void IDotMatrixBLEServer::loop() {
  if (!initialized_) return;

  // Connection callbacks only publish flags. All protocol lifetime changes are
  // performed here in the WLED task, including FA02 assembler destruction.
  bool haveConnectionEvent = false;
  bool newConnectedState = connected_;
  portENTER_CRITICAL(&queueMux_);
  if (connectionEventPending_) {
    haveConnectionEvent = true;
    newConnectedState = pendingConnectedState_;
    connectionEventPending_ = false;
    if (!newConnectedState) rxHead_ = rxTail_ = rxCount_ = 0;
  }
  portEXIT_CRITICAL(&queueMux_);
  if (haveConnectionEvent) {
    connected_ = newConnectedState;
    if (connected_) {
      protocol_.onConnected();
      deviceInfoPushesRemaining_ = 2;
      deviceInfoPushAt_ = millis() + 1200;
    } else {
      faAssembler_.reset();
      audioStreamActive_ = false;
      protocol_.resetAudioStream();
      abortTransfers();
      restartAdvertising_ = true;
      restartAdvertisingAt_ = millis() + 300;
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
  while (dequeue(packet)) processPacket(packet);

  const uint32_t now = millis();
  if (faAssembler_.expected() > 0 && !faAssembler_.complete() &&
      uint32_t(now - reassemblyLastWriteAt_) >= 5000u) {
    ++reassemblyTimeouts_;
    faAssembler_.reset();
    abortTransfers();
  }

  if (bulkTransfer_.isActive() && bulkLastProgressAt_ != 0 &&
      uint32_t(now - bulkLastProgressAt_) >= 5000u) {
    ++bulkTimeouts_;
    abortTransfers();
  }

  // Some device events (currently countdown completion) are reported by the
  // real display asynchronously on FA03 rather than as a direct command ACK.
  IDotMatrixReply asyncReply;
  if (protocol_.pollAsyncReply(asyncReply) && asyncReply.available()) {
    sendFA03(asyncReply.data, asyncReply.length);
  }
}

#if defined(IDOT_NIMBLE_V2_API)
void IDotMatrixBLEServer::ServerCallbacks::onConnect(
  NimBLEServer* server, NimBLEConnInfo& connInfo
) {
  (void)server;
  (void)connInfo;
  owner_.onConnect();
}

void IDotMatrixBLEServer::ServerCallbacks::onDisconnect(
  NimBLEServer* server, NimBLEConnInfo& connInfo, int reason
) {
  (void)server;
  (void)connInfo;
  (void)reason;
  owner_.onDisconnect();
}

void IDotMatrixBLEServer::ServerCallbacks::onMTUChange(
  uint16_t mtu, NimBLEConnInfo& connInfo
) {
  (void)connInfo;
  owner_.onMTUChange(mtu);
}

void IDotMatrixBLEServer::WriteCallbacks::onWrite(
  NimBLECharacteristic* characteristic, NimBLEConnInfo& connInfo
) {
  (void)connInfo;
  owner_.enqueueFromCallback(characteristic);
}
#else
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
#endif

void IDotMatrixBLEServer::onConnect() {
  portENTER_CRITICAL(&queueMux_);
  pendingConnectedState_ = true;
  connectionEventPending_ = true;
  portEXIT_CRITICAL(&queueMux_);
}

void IDotMatrixBLEServer::onDisconnect() {
  portENTER_CRITICAL(&queueMux_);
  pendingConnectedState_ = false;
  connectionEventPending_ = true;
  portEXIT_CRITICAL(&queueMux_);
}

void IDotMatrixBLEServer::onMTUChange(uint16_t mtu) {
  (void)mtu;
}

void IDotMatrixBLEServer::enqueueFromCallback(NimBLECharacteristic* characteristic) {
  if (characteristic == nullptr) return;

  const std::string value = characteristic->getValue();
  if (value.empty()) return;
  if (value.length() > RX_PACKET_MAX) {
    portENTER_CRITICAL(&queueMux_);
    ++rxOversize_;
    portEXIT_CRITICAL(&queueMux_);
    return;
  }

  const RxChannel channel = characteristic == ae01_ ? RxChannel::AE01 : RxChannel::FA02;
  portENTER_CRITICAL(&queueMux_);
  if (rxCount_ >= RX_QUEUE_SIZE) {
    ++rxDropped_;
    portEXIT_CRITICAL(&queueMux_);
    return;
  }

  RxPacket& packet = rxQueue_[rxHead_];
  packet.channel = channel;
  packet.length = static_cast<uint16_t>(value.length());
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
    processFA02Write(packet.data, packet.length);
    return;
  }

  IDotMatrixReply reply;
  processAE01(packet.data, packet.length, reply);
  if (reply.available()) sendFA03(reply.data, reply.length);
}

void IDotMatrixBLEServer::processFA02Write(const uint8_t* data, size_t length) {
  if (data == nullptr || length == 0) return;

  const bool audioStart = idotStartsAudioFrame(data, length);
  if (audioStreamActive_ && idotStartsKnownNonAudioFrame(data, length) && !audioStart) {
    audioStreamActive_ = false;
    protocol_.resetAudioStream();
  }
  if (audioStart || audioStreamActive_) {
    audioStreamActive_ = true;
    IDotMatrixReply reply;
    protocol_.processAudioStream(data, length, reply);
    if (reply.available()) sendFA03(reply.data, reply.length);
    return;
  }

  // The WLED loop is the sole owner of faAssembler_. A single ATT write may
  // finish one logical packet and already contain bytes of the next, so consume
  // it in bounded slices rather than silently discarding trailing bytes.
  size_t offset = 0;
  while (offset < length) {
    size_t needed = 0;
    if (faAssembler_.expected() == 0) {
      if (length - offset < 2) {
        ++rxMalformed_;
        faAssembler_.reset();
        return;
      }
      const uint16_t declaredLength = uint16_t(data[offset]) |
        (uint16_t(data[offset + 1]) << 8);
      if (declaredLength == 0 || declaredLength > BULK_PACKET_MAX ||
          !faAssembler_.ensureCapacity(declaredLength)) {
        ++rxMalformed_;
        faAssembler_.reset();
        return;
      }
      needed = declaredLength;
      reassemblyLastWriteAt_ = millis();
    } else {
      needed = size_t(faAssembler_.expected()) - faAssembler_.received();
    }
    const size_t available = length - offset;
    const size_t take = needed < available ? needed : available;
    const IDotMatrixFA02Assembler::Result result = faAssembler_.append(data + offset, take);
    offset += take;

    if (result == IDotMatrixFA02Assembler::Result::Invalid ||
        result == IDotMatrixFA02Assembler::Result::Busy) {
      ++rxMalformed_;
      faAssembler_.reset();
      return;
    }
    if (result == IDotMatrixFA02Assembler::Result::Accumulating) {
      reassemblyLastWriteAt_ = millis();
      return;
    }

    IDotMatrixReply reply;
    processFA02Complete(faAssembler_.data(), faAssembler_.expected(), reply);
    faAssembler_.reset();
    if (reply.available()) sendFA03(reply.data, reply.length);
  }
}

void IDotMatrixBLEServer::abortTransfers() {
  bulkTransfer_.reset();
  bulkLastProgressAt_ = 0;
  if (carouselTransferReady_) protocol_.cancelCarouselAsset();
  if (presetTransferReady_) protocol_.cancelPresetAsset();
  carouselTransferReady_ = false;
  presetTransferReady_ = false;
  protocol_.completeRawImage(false);
  protocol_.completeGif(false);
  rawTransferReady_ = false;
  gifTransferReady_ = false;
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
    if (bulkResult.began || bulkResult.chunkLength > 0) bulkLastProgressAt_ = millis();
    if (bulkResult.completed || bulkResult.aborted) bulkLastProgressAt_ = 0;
    const bool carouselAsset =
      (bulkResult.type == 0x01 || bulkResult.type == 0x03) && bulkResult.imageIndex < 12;
    const bool presetAsset =
      (bulkResult.type == 0x01 || bulkResult.type == 0x03) &&
      bulkResult.imageIndex >= 14 && bulkResult.imageIndex <= 19;

    if (bulkResult.aborted) {
      if (carouselTransferReady_) protocol_.cancelCarouselAsset();
      if (presetTransferReady_) protocol_.cancelPresetAsset();
      protocol_.completeRawImage(false);
      protocol_.completeGif(false);
      rawTransferReady_ = false;
      gifTransferReady_ = false;
      carouselTransferReady_ = false;
      presetTransferReady_ = false;
    }

    if (presetAsset) {
      if (bulkResult.began) {
        presetTransferReady_ = protocol_.beginPresetAsset(
          bulkResult.type,
          bulkResult.imageIndex,
          bulkResult.timeSign,
          bulkResult.totalLength
        );
      }
      if (bulkResult.chunkLength > 0 && presetTransferReady_) {
        presetTransferReady_ = protocol_.writePresetAsset(
          bulkResult.chunkOffset, bulkResult.chunkData, bulkResult.chunkLength
        );
      }
      if (bulkResult.completed) {
        protocol_.completePresetAsset(bulkResult.crcValid && presetTransferReady_);
        presetTransferReady_ = false;
      }
    } else if (carouselAsset) {
      if (bulkResult.began) {
        carouselTransferReady_ = protocol_.beginCarouselAsset(
          bulkResult.type,
          bulkResult.imageIndex,
          bulkResult.timeSign,
          bulkResult.totalLength
        );
      }
      if (bulkResult.chunkLength > 0 && carouselTransferReady_) {
        carouselTransferReady_ = protocol_.writeCarouselAsset(
          bulkResult.chunkOffset, bulkResult.chunkData, bulkResult.chunkLength
        );
      }
      if (bulkResult.completed) {
        protocol_.completeCarouselAsset(bulkResult.crcValid && carouselTransferReady_);
        carouselTransferReady_ = false;
      }
    } else if (bulkResult.type == 0x01) {
      if (bulkResult.began) {
        protocol_.suspendCarousel();
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
        protocol_.suspendCarousel();
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
      protocol_.suspendCarousel();
      protocol_.processTextPayload(
        bulkTransfer_.textPayload(),
        bulkTransfer_.textPayloadLength()
      );
      // processTextPayload() synchronously copies the glyph bitmaps into the
      // renderer, so the potentially large Bulk scratch buffer can be released
      // immediately instead of remaining resident until the next TEXT transfer.
      bulkTransfer_.reset();
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
#if defined(IDOT_NIMBLE_V2_API)
  // NimBLE 2.x no longer enables scan responses implicitly. AE00 discovery
  // therefore needs to be enabled explicitly.
  advertising->enableScanResponse(true);
  advertising_ = advertising->start();
#else
  advertising->start();
  advertising_ = true;
#endif
}
