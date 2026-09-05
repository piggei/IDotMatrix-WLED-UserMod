#include "IDotMatrixMedia.h"

#include "IDotMatrixRenderer.h"
#include "wled.h"
#include <AnimatedGIF.h>
#include <cstring>
#include <new>
#include <cstdlib>


#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#include <esp32-hal-psram.h>
#endif

#if __has_include(<rom/miniz.h>)
#include <rom/miniz.h>
#elif __has_include(<miniz.h>)
#include <miniz.h>
#else
#error "ESP32 miniz is required for iDotMatrix PNG decoding"
#endif

namespace {
constexpr char GIF_PLAY[] = "/idot_play.gif";
constexpr char GIF_CACHE[] = "/idot_cache.bin";
constexpr size_t GIF_CACHE_HEADER_BYTES = 8u;
constexpr size_t GIF_CACHE_MAX_BYTES = 512u * 1024u;
constexpr uint8_t GIF_CACHE_MAGIC[4] = {'I', 'D', 'C', '1'};
constexpr const char* GIF_RX[] = {"/idot_rx0.tmp", "/idot_rx1.tmp"};
IDotMatrixMedia* activeMedia = nullptr;
File gifRxFile;
File gifCacheWriteFile;
File gifCacheReadFile;
File compactGifFile;

#if IDOT_GIF_BITS <= 10
// The validated 10-bit/16x16 decoder stays in static DRAM. Larger decoders
// must not permanently reduce the RAM available to WLED effects.
alignas(AnimatedGIF) uint8_t lowRamDecoderStorage[sizeof(AnimatedGIF)];
#endif

uint32_t be32(const uint8_t* value) {
  return (uint32_t(value[0]) << 24) | (uint32_t(value[1]) << 16) |
    (uint32_t(value[2]) << 8) | value[3];
}



void* allocateMediaBuffer(size_t bytes) {
  if (bytes == 0) return nullptr;
#if defined(ARDUINO_ARCH_ESP32)
  if (psramFound()) {
    void* external = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (external != nullptr) return external;
  }
  return heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
#else
  return std::malloc(bytes);
#endif
}

void freeMediaBuffer(void* buffer) {
#if defined(ARDUINO_ARCH_ESP32)
  heap_caps_free(buffer);
#else
  std::free(buffer);
#endif
}

uint8_t paeth(uint8_t a, uint8_t b, uint8_t c) {
  const int value = int(a) + int(b) - int(c);
  const int da = abs(value - int(a));
  const int db = abs(value - int(b));
  const int dc = abs(value - int(c));
  return da <= db && da <= dc ? a : db <= dc ? b : c;
}
}

IDotMatrixMedia::IDotMatrixMedia(IDotMatrixRenderer& renderer) : renderer_(renderer) {
  activeMedia = this;
}

IDotMatrixMedia::~IDotMatrixMedia() {
  cancelGifReceive();
  stopPlayback();
  if (activeMedia == this) activeMedia = nullptr;
}

size_t IDotMatrixMedia::gifDecoderBytes() const {
#if IDOT_GIF_BITS >= 12 && defined(ARDUINO_ARCH_ESP32)
  if (!psramFound()) {
    return IDotMatrixCompactGif::requiredWorkspaceBytes(renderer_.animationFrameBytes());
  }
#endif
  return sizeof(AnimatedGIF);
}

const char* IDotMatrixMedia::gifDecoderModeText() const {
#if IDOT_GIF_BITS >= 12 && defined(ARDUINO_ARCH_ESP32)
  return psramFound() ? "animatedgif12/psram" : "compact12/cache";
#elif IDOT_GIF_BITS >= 12
  return "compact12/cache";
#elif IDOT_GIF_BITS == 11
  return "animatedgif11";
#else
  return "animatedgif10";
#endif
}


bool IDotMatrixMedia::beginGif(size_t byteLength) {
  cancelGifReceive();
  lastError_ = Error::None;
  cacheLowHeapWaiting_ = false;
  cacheLowHeapSince_ = 0;
  cacheLowHeapWaitCount_ = 0;
  cacheLowHeapMin_ = 0;
  gifProbeFree_ = 0;
  gifProbeLargest_ = 0;
  if (byteLength < 6 || byteLength > 2u * 1024u * 1024u) {
    return false;
  }
  rxSlot_ = nextRxSlot_ & 1u;
  nextRxSlot_ ^= 1u;
  if (rxSlot_ == pendingSlot_) rxSlot_ ^= 1;
  const char* path = GIF_RX[uint8_t(rxSlot_)];
  WLED_FS.remove(path);
  gifRxFile = WLED_FS.open(path, "w");
  if (!gifRxFile) {
    rxSlot_ = -1;
    return false;
  }
  gifExpected_ = byteLength;
  gifWritten_ = 0;
  rxOpen_ = true;
  return true;
}

bool IDotMatrixMedia::writeGif(
  size_t offset, const uint8_t* data, size_t length
) {
  if (!rxOpen_ || !gifRxFile || data == nullptr || offset != gifWritten_ ||
      length > gifExpected_ - gifWritten_) return false;
  const size_t written = gifRxFile.write(data, length);
  gifWritten_ += written;
  return written == length;
}

bool IDotMatrixMedia::completeGif(bool crcValid) {
  if (rxOpen_) {
    gifRxFile.flush();
    gifRxFile.close();
    rxOpen_ = false;
  }
  const bool valid = crcValid && rxSlot_ >= 0 && gifWritten_ == gifExpected_;
  if (!valid) {
    if (rxSlot_ >= 0) WLED_FS.remove(GIF_RX[uint8_t(rxSlot_)]);
    rxSlot_ = -1;
    return false;
  }
  // A valid replacement is now durable in its RX slot.  On the no-PSRAM
  // frame-cache path, release the currently playing GIF/cache immediately so
  // the replacement starts from a clean filesystem/heap state.  Do this only
  // after CRC/length validation so a bad transfer cannot destroy the content
  // that is already being displayed.  Direct 10/11-bit and PSRAM playback
  // retain their established replacement behaviour.
  if (useFrameCache()) releasePlaybackResources();

  pendingGifBytes_ = gifWritten_;
  if (pendingSlot_ >= 0) WLED_FS.remove(GIF_RX[uint8_t(pendingSlot_)]);
  pendingSlot_ = rxSlot_;
  rxSlot_ = -1;
  promotePending_ = true;
  return true;
}

void IDotMatrixMedia::cancelGifReceive() {
  if (rxOpen_) {
    gifRxFile.close();
    rxOpen_ = false;
  }
  if (rxSlot_ >= 0) WLED_FS.remove(GIF_RX[uint8_t(rxSlot_)]);
  rxSlot_ = -1;
  gifExpected_ = 0;
  gifWritten_ = 0;
}

void IDotMatrixMedia::loop(uint32_t now) {
  // Promotion, decoder construction, cache generation and playback are split
  // across WLED loop iterations so Wi-Fi/BLE keep getting service time.
  if (promotePending_) {
    promotePending_ = false;
    if (promoteGif()) {
      openPending_ = true;
      openDelayLoops_ = 1;
    }
    return;
  }
  if (openPending_) {
    if (openDelayLoops_ > 0) {
      --openDelayLoops_;
      return;
    }
    openPending_ = false;
    if (useFrameCache()) openGifForCache();
    else openGif();
    return;
  }
  if (cacheBuilding_) {
    buildCacheFrame(now);
    return;
  }
  if (!gifPlaying_ || int32_t(now - nextFrameAt_) < 0) return;
  if (cachePlayback_) {
    playCachedFrame(now);
    return;
  }
  if (decoder_ == nullptr) return;
  if (restartPending_) {
    decoder_->reset();
    renderer_.clearAnimation();
    restartPending_ = false;
  }
  frameDrawn_ = false;
  int delayMs = 0;
  const int result = decoder_->playFrame(false, &delayMs);
  if (!frameDrawn_) {
    decoder_->reset();
    nextFrameAt_ = now + 10;
    return;
  }
  renderer_.publishAnimationFrame();
  if (delayMs < 10) delayMs = 10;
  nextFrameAt_ = now + uint32_t(delayMs);
  if (result == 0) restartPending_ = true;
}

bool IDotMatrixMedia::useFrameCache() const {
#if IDOT_GIF_BITS >= 12 && defined(ARDUINO_ARCH_ESP32)
  return !psramFound();
#elif IDOT_GIF_BITS >= 12
  return true;
#else
  return false;
#endif
}

bool IDotMatrixMedia::inspectGifFile(const char* path) {
#if !defined(ARDUINO_ARCH_ESP32)
  (void)path;
  // Host tests use a deliberately tiny filesystem stub. Hardware builds do
  // the real header/size inspection below.
  return true;
#else
  File file = WLED_FS.open(path, "r");
  if (!file) return false;
  const size_t fileBytes = file.size();
  uint8_t header[10] = {0};
  const size_t got = file.read(header, sizeof(header));
  file.close();
  if (got != sizeof(header) || fileBytes < 13) return false;
  if (memcmp(header, "GIF87a", 6) != 0 && memcmp(header, "GIF89a", 6) != 0) return false;
  const uint16_t width = uint16_t(header[6]) | (uint16_t(header[7]) << 8);
  const uint16_t height = uint16_t(header[8]) | (uint16_t(header[9]) << 8);
  if (width == 0 || height == 0 || width > IDOT_GIF_MAX_DIM || height > IDOT_GIF_MAX_DIM) return false;
  if (width != renderer_.logicalWidth() || height != renderer_.logicalHeight()) return false;
  return true;
#endif
}

bool IDotMatrixMedia::promoteGif() {
  if (pendingSlot_ < 0) return false;
  const char* rx = GIF_RX[uint8_t(pendingSlot_)];
#if defined(ARDUINO_ARCH_ESP32)
  File received = WLED_FS.open(rx, "r");
  const bool sizeMatches = received && received.size() == pendingGifBytes_;
  received.close();
#else
  const bool sizeMatches = true;
#endif
  if (!sizeMatches || !inspectGifFile(rx)) {
    lastError_ = Error::GifInvalid;
    WLED_FS.remove(rx);
    pendingSlot_ = -1;
    return false;
  }
  destroyDecoder(false);
  WLED_FS.remove(GIF_PLAY);
  bool promoted = WLED_FS.exists(rx) && WLED_FS.rename(rx, GIF_PLAY);
  // Some ESP32 filesystem builds have shown sporadic rename failures.  Falling
  // back to a streamed copy keeps the decoder path deterministic and never
  // buffers the complete GIF in RAM.
  if (!promoted && WLED_FS.exists(rx)) {
    File source = WLED_FS.open(rx, "r");
    File target = WLED_FS.open(GIF_PLAY, "w");
    if (source && target) {
      uint8_t buffer[256];
      promoted = true;
      const size_t sourceSize = source.size();
      while (source.position() < sourceSize) {
        const size_t remaining = sourceSize - source.position();
        const size_t count = source.read(buffer, remaining < sizeof(buffer) ? remaining : sizeof(buffer));
        if (count == 0 || target.write(buffer, count) != count) { promoted = false; break; }
      }
      target.flush();
    } else {
      promoted = false;
    }
    source.close();
    target.close();
    if (promoted) WLED_FS.remove(rx);
  }
  pendingSlot_ = -1;
  pendingGifBytes_ = 0;
  return promoted;
}

bool IDotMatrixMedia::ensureDecoderStorage() {
  if (decoderStorage_ != nullptr) return true;
#if IDOT_GIF_BITS <= 10
  decoderStorage_ = lowRamDecoderStorage;
  return true;
#elif IDOT_GIF_BITS == 11 && defined(ARDUINO_ARCH_ESP32)
  decoderStorage_ = heap_caps_malloc(sizeof(AnimatedGIF), MALLOC_CAP_8BIT);
  return decoderStorage_ != nullptr;
#elif IDOT_GIF_BITS == 11
  decoderStorage_ = ::operator new(sizeof(AnimatedGIF), std::nothrow);
  return decoderStorage_ != nullptr;
#elif defined(ARDUINO_ARCH_ESP32)
  // Full 12-bit direct decoding needs the complete 4096-entry LZW dictionary.
  // The no-PSRAM 64x64 path uses IDotMatrixCompactGif/frame-cache instead.
  // When direct mode is selected, prefer PSRAM and keep a guarded internal-DRAM
  // fallback rather than turning an allocation failure into WLED Error 8/reboot.
  if (psramFound()) {
    decoderStorage_ = heap_caps_malloc(
      sizeof(AnimatedGIF), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );
    if (decoderStorage_ != nullptr) return true;
  }
#ifndef IDOT_GIF_DRAM_RESERVE
#define IDOT_GIF_DRAM_RESERVE 10240u
#endif
  const size_t decoderBytes = sizeof(AnimatedGIF);
  const size_t freeBytes = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  const size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  gifProbeFree_ = freeBytes;
  gifProbeLargest_ = largest;
  // WLED has already staged the iDotMatrix effect before this probe, so the
  // remaining reserve protects only normal runtime allocations. It does not
  // need to be contiguous with the decoder itself.
  if (freeBytes < decoderBytes + IDOT_GIF_DRAM_RESERVE ||
      largest < decoderBytes) {
    return false;
  }
  decoderStorage_ = heap_caps_malloc(decoderBytes, MALLOC_CAP_8BIT);
  return decoderStorage_ != nullptr;
#else
  decoderStorage_ = ::operator new(sizeof(AnimatedGIF), std::nothrow);
  return decoderStorage_ != nullptr;
#endif
}

bool IDotMatrixMedia::openGifForCache() {
  resetGifCache();
  if (!WLED_FS.exists(GIF_PLAY) || !inspectGifFile(GIF_PLAY)) {
    lastError_ = Error::GifInvalid;
    return false;
  }

#if IDOT_GIF_BITS >= 12 && defined(ARDUINO_ARCH_ESP32)
  if (!psramFound()) {
    compactGifFile = WLED_FS.open(GIF_PLAY, "r");
    if (!compactGifFile) {
      lastError_ = Error::GifDecoderOpen;
      return false;
    }
    gifProbeFree_ = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    gifProbeLargest_ = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    const size_t required = IDotMatrixCompactGif::requiredWorkspaceBytes(renderer_.animationFrameBytes());
    if (gifProbeFree_ < required + gifDramReserve() || gifProbeLargest_ < required) {
      compactGifFile.close();
      lastError_ = Error::GifRamReserve;
      return false;
    }
    if (!renderer_.beginAnimation()) {
      compactGifFile.close();
      lastError_ = Error::GifCanvasOom;
      return false;
    }
    if (!compactDecoder_.open(
          &compactGifFile, compactRead, compactSeek,
          renderer_.logicalWidth(), renderer_.logicalHeight(),
          renderer_.width(), renderer_.height(), renderer_.animationFrameBytes()
        )) {
      compactGifFile.close();
      lastError_ = Error::GifDecoderOom;
      return false;
    }
    compactCache_ = true;
  } else
#endif
  {
    if (!ensureDecoderStorage()) {
#if IDOT_GIF_BITS >= 12 && defined(ARDUINO_ARCH_ESP32)
      lastError_ = psramFound() ? Error::GifDecoderOom : Error::GifRamReserve;
#else
      lastError_ = Error::GifDecoderOom;
#endif
      return false;
    }

    decoder_ = new (decoderStorage_) AnimatedGIF();
    decoder_->begin(LITTLE_ENDIAN_PIXELS);
    decoder_->setDrawType(GIF_DRAW_RAW);
    if (!decoder_->open(GIF_PLAY, openFile, closeFile, readFile, seekFile, drawGif)) {
      lastError_ = Error::GifDecoderOpen;
      destroyDecoder(true);
      return false;
    }
    if (!renderer_.beginAnimation()) {
      lastError_ = Error::GifCanvasOom;
      destroyDecoder(true);
      return false;
    }
    decoder_->reset();
  }

  cacheFrameBytes_ = renderer_.animationFrameBytes();
  if (cacheFrameBytes_ == 0 || cacheFrameBytes_ > 0xFFFFu) {
    lastError_ = Error::GifCacheIo;
    resetGifCache();
    destroyDecoder(true);
    return false;
  }
  WLED_FS.remove(GIF_CACHE);
  gifCacheWriteFile = WLED_FS.open(GIF_CACHE, "w");
  if (!gifCacheWriteFile) {
    lastError_ = Error::GifCacheIo;
    resetGifCache();
    destroyDecoder(true);
    return false;
  }
  uint8_t header[GIF_CACHE_HEADER_BYTES] = {
    GIF_CACHE_MAGIC[0], GIF_CACHE_MAGIC[1], GIF_CACHE_MAGIC[2], GIF_CACHE_MAGIC[3],
    renderer_.width(), renderer_.height(),
    uint8_t(cacheFrameBytes_ & 0xFFu), uint8_t((cacheFrameBytes_ >> 8) & 0xFFu)
  };
  if (gifCacheWriteFile.write(header, sizeof(header)) != sizeof(header)) {
    lastError_ = Error::GifCacheIo;
    resetGifCache();
    destroyDecoder(true);
    return false;
  }
  cacheBytes_ = sizeof(header);
  cachedFrames_ = 0;
  cacheFrameIndex_ = 0;
  renderer_.clearAnimation();
  renderer_.setVisible(false);
  lastError_ = Error::None;
  cacheBuilding_ = true;
  gifPlaying_ = false;
  cachePlayback_ = false;
  restartPending_ = false;
  return true;
}

bool IDotMatrixMedia::buildCacheFrame(uint32_t now) {
  (void)now;
  if (!cacheBuilding_ || !gifCacheWriteFile) return false;

#if IDOT_GIF_BITS >= 12 && defined(ARDUINO_ARCH_ESP32)
  if (!psramFound()) {
    const size_t freeBytes = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    const size_t runtimeReserve = gifCacheRuntimeReserve();
    const uint32_t lowHeapTimeoutMs = gifCacheLowHeapTimeoutMs();

    if (freeBytes < runtimeReserve) {
      ++cacheLowHeapWaitCount_;
      if (cacheLowHeapMin_ == 0 || freeBytes < cacheLowHeapMin_) {
        cacheLowHeapMin_ = freeBytes;
      }
      if (!cacheLowHeapWaiting_) {
        cacheLowHeapWaiting_ = true;
        cacheLowHeapSince_ = now;
      }

      // Wi-Fi, BLE, WebSocket and LittleFS all make short-lived allocations.
      // A single sample below the reserve is therefore not a terminal OOM.
      // Yield this media turn and let the normal WLED loop release transient
      // buffers; only a sustained low-heap condition aborts the cache build.
      if (uint32_t(now - cacheLowHeapSince_) < lowHeapTimeoutMs) return true;

      lastError_ = Error::GifRamReserve;
      resetGifCache();
      destroyDecoder(true);
      return false;
    }

    cacheLowHeapWaiting_ = false;
    cacheLowHeapSince_ = 0;
  }
#endif

  int delayMs = 0;
  bool reachedEnd = false;
  if (compactCache_) {
    uint16_t compactDelay = 10;
    const IDotMatrixCompactGif::Result result = compactDecoder_.decodeNextFrame(renderer_, compactDelay);
    if (result == IDotMatrixCompactGif::Result::Error) {
      lastError_ = Error::GifDecoderOpen;
      resetGifCache();
      destroyDecoder(true);
      return false;
    }
    if (result == IDotMatrixCompactGif::Result::End) {
      return finalizeGifCache(millis());
    }
    delayMs = int(compactDelay);
  } else {
    if (decoder_ == nullptr) return false;
    frameDrawn_ = false;
    const int result = decoder_->playFrame(false, &delayMs);
    if (!frameDrawn_) {
      lastError_ = Error::GifDecoderOpen;
      resetGifCache();
      destroyDecoder(true);
      return false;
    }
    reachedEnd = result == 0;
  }

  if (delayMs < 10) delayMs = 10;
  if (delayMs > 65535) delayMs = 65535;

  const size_t recordBytes = 2u + cacheFrameBytes_;
  if (cacheBytes_ + recordBytes > GIF_CACHE_MAX_BYTES) {
    lastError_ = Error::GifCacheFull;
    resetGifCache();
    destroyDecoder(true);
    return false;
  }
  const uint8_t delayBytes[2] = {
    uint8_t(uint16_t(delayMs) & 0xFFu),
    uint8_t((uint16_t(delayMs) >> 8) & 0xFFu)
  };
  if (gifCacheWriteFile.write(delayBytes, sizeof(delayBytes)) != sizeof(delayBytes) ||
      gifCacheWriteFile.write(
        reinterpret_cast<const uint8_t*>(renderer_.pixels()), cacheFrameBytes_
      ) != cacheFrameBytes_) {
    lastError_ = Error::GifCacheIo;
    resetGifCache();
    destroyDecoder(true);
    return false;
  }
  cacheBytes_ += recordBytes;
  ++cachedFrames_;

  if (reachedEnd) return finalizeGifCache(millis());
  return true;
}

bool IDotMatrixMedia::finalizeGifCache(uint32_t now) {
  cacheBuilding_ = false;
  gifCacheWriteFile.flush();
  gifCacheWriteFile.close();
  // The entire purpose of the cache is to remove the 12-bit decoder from RAM
  // before WLED's display effect becomes active.
  if (compactCache_) {
    compactDecoder_.close();
    compactCache_ = false;
    if (compactGifFile) compactGifFile.close();
    renderer_.endAnimation();
  } else {
    destroyDecoder(true);
  }
  if (cachedFrames_ == 0 || !WLED_FS.exists(GIF_CACHE)) {
    lastError_ = Error::GifCacheIo;
    resetGifCache();
    return false;
  }

  gifCacheReadFile = WLED_FS.open(GIF_CACHE, "r");
  if (!gifCacheReadFile || !gifCacheReadFile.seek(GIF_CACHE_HEADER_BYTES, SeekSet)) {
    lastError_ = Error::GifCacheIo;
    resetGifCache();
    return false;
  }
  renderer_.clearAnimation();
  renderer_.setVisible(false);
  cachePlayback_ = true;
  gifPlaying_ = true;
  cacheFrameIndex_ = 0;
  nextFrameAt_ = now;
  lastError_ = Error::None;
  return true;
}

bool IDotMatrixMedia::playCachedFrame(uint32_t now) {
  if (!cachePlayback_ || !gifPlaying_ || !gifCacheReadFile || cachedFrames_ == 0) return false;
  if (cacheFrameIndex_ >= cachedFrames_) {
    if (!gifCacheReadFile.seek(GIF_CACHE_HEADER_BYTES, SeekSet)) {
      lastError_ = Error::GifCacheIo;
      stopPlayback();
      return false;
    }
    cacheFrameIndex_ = 0;
  }

  uint8_t delayBytes[2] = {0, 0};
  if (gifCacheReadFile.read(delayBytes, sizeof(delayBytes)) != sizeof(delayBytes)) {
    lastError_ = Error::GifCacheIo;
    stopPlayback();
    return false;
  }
  const uint16_t delayMs = uint16_t(delayBytes[0]) | (uint16_t(delayBytes[1]) << 8);
  uint8_t* target = renderer_.animationFrameData();
  if (target == nullptr || gifCacheReadFile.read(target, cacheFrameBytes_) != cacheFrameBytes_) {
    lastError_ = Error::GifCacheIo;
    stopPlayback();
    return false;
  }
  renderer_.publishAnimationFrame();
  ++cacheFrameIndex_;
  nextFrameAt_ = now + (delayMs < 10 ? 10u : uint32_t(delayMs));
  return true;
}

void IDotMatrixMedia::resetGifCache() {
  compactDecoder_.close();
  compactCache_ = false;
  if (compactGifFile) compactGifFile.close();
  cacheBuilding_ = false;
  cachePlayback_ = false;
  cacheFrameBytes_ = 0;
  cacheBytes_ = 0;
  cachedFrames_ = 0;
  cacheFrameIndex_ = 0;
  if (gifCacheWriteFile) gifCacheWriteFile.close();
  if (gifCacheReadFile) gifCacheReadFile.close();
  WLED_FS.remove(GIF_CACHE);
}

bool IDotMatrixMedia::openGif() {
  if (!WLED_FS.exists(GIF_PLAY) || !inspectGifFile(GIF_PLAY)) {
    lastError_ = Error::GifInvalid;
    return false;
  }
  if (!ensureDecoderStorage()) {
#if IDOT_GIF_BITS >= 12 && defined(ARDUINO_ARCH_ESP32)
    if (psramFound()) lastError_ = Error::GifDecoderOom;
    else lastError_ = Error::GifRamReserve;
#else
    lastError_ = Error::GifDecoderOom;
#endif
    return false;
  }

  decoder_ = new (decoderStorage_) AnimatedGIF();
  decoder_->begin(LITTLE_ENDIAN_PIXELS);
  decoder_->setDrawType(GIF_DRAW_RAW);
  if (!decoder_->open(GIF_PLAY, openFile, closeFile, readFile, seekFile, drawGif)) {
    lastError_ = Error::GifDecoderOpen;
    destroyDecoder(false);
    return false;
  }
  if (!renderer_.beginAnimation()) {
    lastError_ = Error::GifCanvasOom;
    destroyDecoder(false);
    return false;
  }
  decoder_->reset();
  lastError_ = Error::None;
  gifPlaying_ = true;
  restartPending_ = false;
  nextFrameAt_ = millis();
  return true;
}

void IDotMatrixMedia::destroyDecoder(bool releaseStorage) {
  gifPlaying_ = false;
  restartPending_ = false;
  frameDrawn_ = false;
  if (decoder_ != nullptr) {
    decoder_->close();
    decoder_->~AnimatedGIF();
    decoder_ = nullptr;
  }
  if (releaseStorage && decoderStorage_ != nullptr) {
#if IDOT_GIF_BITS >= 11
  #if defined(ARDUINO_ARCH_ESP32)
    heap_caps_free(decoderStorage_);
  #else
    ::operator delete(decoderStorage_);
  #endif
#endif
    decoderStorage_ = nullptr;
  }
  renderer_.endAnimation();
}

void IDotMatrixMedia::releasePlaybackResources() {
  destroyDecoder(true);
  resetGifCache();
}

void IDotMatrixMedia::stopPlayback() {
  promotePending_ = false;
  openPending_ = false;
  openDelayLoops_ = 0;
  if (pendingSlot_ >= 0) {
    WLED_FS.remove(GIF_RX[uint8_t(pendingSlot_)]);
    pendingSlot_ = -1;
  }
  releasePlaybackResources();
}


const char* IDotMatrixMedia::lastErrorText() const {
  switch (lastError_) {
    case Error::GifInvalid: return "gif-invalid";
    case Error::GifDecoderOom: return "gif-decoder-oom";
    case Error::GifPsramRequired: return "gif-psram-required";
    case Error::GifRamReserve: return "gif-ram-reserve";
    case Error::GifDecoderOpen: return "gif-decoder-open";
    case Error::GifCanvasOom: return "gif-canvas-oom";
    case Error::GifCacheIo: return "gif-cache-io";
    case Error::GifCacheFull: return "gif-cache-full";
    case Error::None:
    default: return "none";
  }
}

void* IDotMatrixMedia::openFile(const char* name, int32_t* size) {
  File* file = new (std::nothrow) File(WLED_FS.open(name, "r"));
  if (file == nullptr || !(*file)) {
    delete file;
    return nullptr;
  }
  *size = int32_t(file->size());
  return file;
}

void IDotMatrixMedia::closeFile(void* handle) {
  File* file = static_cast<File*>(handle);
  if (file != nullptr) {
    file->close();
    delete file;
  }
}

int32_t IDotMatrixMedia::readFile(GIFFILE* file, uint8_t* buffer, int32_t length) {
  File* source = static_cast<File*>(file->fHandle);
  if (source == nullptr || !(*source) || length <= 0) return 0;
  int32_t remaining = file->iSize - file->iPos;
  if (remaining <= 0) return 0;
  if (length > remaining) length = remaining;
  const int32_t count = int32_t(source->read(buffer, size_t(length)));
  file->iPos = int32_t(source->position());
  return count;
}

int32_t IDotMatrixMedia::seekFile(GIFFILE* file, int32_t position) {
  File* source = static_cast<File*>(file->fHandle);
  if (source == nullptr || !(*source)) return -1;
  if (position < 0) position = 0;
  if (position > file->iSize) position = file->iSize;
  if (!source->seek(uint32_t(position), SeekSet)) return -1;
  file->iPos = int32_t(source->position());
  return file->iPos;
}

int32_t IDotMatrixMedia::compactRead(void* context, uint8_t* buffer, size_t length) {
  File* file = static_cast<File*>(context);
  if (file == nullptr || !(*file) || buffer == nullptr || length == 0) return 0;
  return int32_t(file->read(buffer, length));
}

bool IDotMatrixMedia::compactSeek(void* context, uint32_t position) {
  File* file = static_cast<File*>(context);
  return file != nullptr && (*file) && file->seek(position, SeekSet);
}

void IDotMatrixMedia::drawGif(GIFDRAW* draw) {
  if (activeMedia == nullptr || draw == nullptr) return;
  IDotMatrixMedia& self = *activeMedia;
  self.frameDrawn_ = true;
  const int y = draw->iY + draw->y;
  if (y < 0 || y >= self.renderer_.logicalHeight()) return;
  int width = draw->iWidth;
  if (draw->iX + width > self.renderer_.logicalWidth()) width = self.renderer_.logicalWidth() - draw->iX;
  if (width <= 0 || draw->pPalette == nullptr) return;
  uint8_t* indexes = draw->pPixels;
  if (draw->ucDisposalMethod == 2) {
    for (int x = 0; x < width; ++x) {
      if (indexes[x] == draw->ucTransparent) indexes[x] = draw->ucBackground;
    }
    draw->ucHasTransparency = 0;
  }
  for (int x = 0; x < width; ++x) {
    const int targetX = draw->iX + x;
    if (targetX < 0 || targetX >= self.renderer_.logicalWidth()) continue;
    const uint8_t index = indexes[x];
    if (draw->ucHasTransparency && index == draw->ucTransparent) continue;
    const uint16_t rgb = draw->pPalette[index];
    self.renderer_.setAnimationSourcePixel(
      uint8_t(targetX), uint8_t(y),
      uint8_t(((rgb >> 11) & 0x1F) * 255 / 31),
      uint8_t(((rgb >> 5) & 0x3F) * 255 / 63),
      uint8_t((rgb & 0x1F) * 255 / 31)
    );
  }
}

bool IDotMatrixMedia::decodePng(const uint8_t* png, size_t length) {
  static const uint8_t signature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  if (png == nullptr || length < 33 || memcmp(png, signature, 8) != 0) {
        return false;
  }
  uint32_t width = 0, height = 0, idatBytes = 0;
  uint8_t depth = 0, colorType = 0, compression = 0, filter = 0, interlace = 0;
  size_t position = 8;
  while (position + 12 <= length) {
    const uint32_t bytes = be32(png + position);
    if (uint64_t(position) + 12u + bytes > length) break;
    const uint8_t* type = png + position + 4;
    const uint8_t* chunk = png + position + 8;
    if (memcmp(type, "IHDR", 4) == 0 && bytes == 13) {
      width = be32(chunk); height = be32(chunk + 4); depth = chunk[8];
      colorType = chunk[9]; compression = chunk[10]; filter = chunk[11]; interlace = chunk[12];
    } else if (memcmp(type, "IDAT", 4) == 0) {
      idatBytes += bytes;
    } else if (memcmp(type, "IEND", 4) == 0) break;
    position += 12 + bytes;
  }
  if (width != renderer_.logicalWidth() || height != renderer_.logicalHeight() || depth != 8 ||
      compression != 0 || filter != 0 || interlace != 0 ||
      (colorType != 2 && colorType != 6) || idatBytes == 0) {
        return false;
  }
  const uint8_t bpp = colorType == 6 ? 4 : 3;
  const size_t rowBytes = size_t(width) * bpp;
  const size_t rawSize = (rowBytes + 1) * height;
  uint8_t* idat = static_cast<uint8_t*>(allocateMediaBuffer(idatBytes));
  uint8_t* raw = static_cast<uint8_t*>(allocateMediaBuffer(rawSize));
  tinfl_decompressor* inflater = static_cast<tinfl_decompressor*>(
    allocateMediaBuffer(sizeof(tinfl_decompressor))
  );
  if (idat == nullptr || raw == nullptr || inflater == nullptr) {
    freeMediaBuffer(idat); freeMediaBuffer(raw); freeMediaBuffer(inflater); return false;
  }
  position = 8;
  size_t output = 0;
  while (position + 12 <= length) {
    const uint32_t bytes = be32(png + position);
    if (uint64_t(position) + 12u + bytes > length) break;
    const uint8_t* type = png + position + 4;
    if (memcmp(type, "IDAT", 4) == 0) {
      memcpy(idat + output, png + position + 8, bytes); output += bytes;
    }
    if (memcmp(type, "IEND", 4) == 0) break;
    position += 12 + bytes;
  }
  tinfl_init(inflater);
  size_t inputBytes = idatBytes;
  size_t outputBytes = rawSize;
  const tinfl_status status = tinfl_decompress(
    inflater, idat, &inputBytes, raw, raw, &outputBytes,
    TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF
  );
  freeMediaBuffer(inflater);
  freeMediaBuffer(idat);
  if (output != idatBytes || status != TINFL_STATUS_DONE || outputBytes != rawSize) {
    freeMediaBuffer(raw); return false;
  }
  for (uint32_t y = 0; y < height; ++y) {
    uint8_t* row = raw + size_t(y) * (rowBytes + 1);
    uint8_t* current = row + 1;
    uint8_t* previous = y ? raw + size_t(y - 1) * (rowBytes + 1) + 1 : nullptr;
    if (row[0] > 4) { freeMediaBuffer(raw); return false; }
    for (size_t x = 0; x < rowBytes; ++x) {
      const uint8_t left = x >= bpp ? current[x - bpp] : 0;
      const uint8_t up = previous ? previous[x] : 0;
      const uint8_t upperLeft = previous && x >= bpp ? previous[x - bpp] : 0;
      if (row[0] == 1) current[x] = uint8_t(current[x] + left);
      else if (row[0] == 2) current[x] = uint8_t(current[x] + up);
      else if (row[0] == 3) current[x] = uint8_t(current[x] + (uint16_t(left) + up) / 2u);
      else if (row[0] == 4) current[x] = uint8_t(current[x] + paeth(left, up, upperLeft));
    }
  }
  const size_t rgbBytes = size_t(width) * height * 3u;
  bool ok = renderer_.beginRawImage(rgbBytes);
  uint8_t rgb[3];
  size_t rgbOffset = 0;
  for (uint32_t y = 0; ok && y < height; ++y) {
    const uint8_t* row = raw + size_t(y) * (rowBytes + 1) + 1;
    for (uint32_t x = 0; ok && x < width; ++x) {
      const uint8_t* pixel = row + size_t(x) * bpp;
      const uint8_t alpha = bpp == 4 ? pixel[3] : 255;
      rgb[0] = uint8_t((uint16_t(pixel[0]) * alpha + 127u) / 255u);
      rgb[1] = uint8_t((uint16_t(pixel[1]) * alpha + 127u) / 255u);
      rgb[2] = uint8_t((uint16_t(pixel[2]) * alpha + 127u) / 255u);
      ok = renderer_.writeRawImage(rgbOffset, rgb, sizeof(rgb));
      rgbOffset += sizeof(rgb);
    }
  }
  freeMediaBuffer(raw);
  ok = renderer_.completeRawImage(ok);
  return ok;
}
