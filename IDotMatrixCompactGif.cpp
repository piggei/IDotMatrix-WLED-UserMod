#include "IDotMatrixCompactGif.h"
#include "IDotMatrixRenderer.h"

#include <cstring>
#include <cstdlib>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

namespace {
void* allocWorkspace(size_t bytes) {
#if defined(ARDUINO_ARCH_ESP32)
  return heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
#else
  return std::malloc(bytes);
#endif
}

void freeWorkspace(void* ptr) {
#if defined(ARDUINO_ARCH_ESP32)
  heap_caps_free(ptr);
#else
  std::free(ptr);
#endif
}

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return uint16_t((uint16_t(r >> 3) << 11) | (uint16_t(g >> 2) << 5) | uint16_t(b >> 3));
}
}

IDotMatrixCompactGif::~IDotMatrixCompactGif() {
  close();
}

bool IDotMatrixCompactGif::readExact(uint8_t* dst, size_t length) {
  if (readFn_ == nullptr || dst == nullptr) return false;
  size_t done = 0;
  while (done < length) {
    const int32_t got = readFn_(context_, dst + done, length - done);
    if (got <= 0) return false;
    done += size_t(got);
    position_ += uint32_t(got);
  }
  return true;
}

bool IDotMatrixCompactGif::readByte(uint8_t& value) {
  return readExact(&value, 1);
}

bool IDotMatrixCompactGif::skipBytes(size_t length) {
  uint8_t scratch[32];
  while (length > 0) {
    const size_t chunk = length < sizeof(scratch) ? length : sizeof(scratch);
    if (!readExact(scratch, chunk)) return false;
    length -= chunk;
  }
  return true;
}

bool IDotMatrixCompactGif::readPalette(uint16_t* target, uint16_t entries) {
  if (target == nullptr || entries == 0 || entries > 256) return false;
  uint8_t rgb[3];
  for (uint16_t i = 0; i < entries; ++i) {
    if (!readExact(rgb, sizeof(rgb))) return false;
    target[i] = rgb565(rgb[0], rgb[1], rgb[2]);
  }
  return true;
}

bool IDotMatrixCompactGif::open(
  void* context,
  ReadFn readFn,
  SeekFn seekFn,
  uint8_t logicalWidth,
  uint8_t logicalHeight,
  uint8_t storageWidth,
  uint8_t storageHeight,
  size_t frameBytes
) {
  close();
  if (context == nullptr || readFn == nullptr || seekFn == nullptr ||
      logicalWidth == 0 || logicalHeight == 0 || storageWidth == 0 || storageHeight == 0 ||
      frameBytes == 0) return false;

  context_ = context;
  readFn_ = readFn;
  seekFn_ = seekFn;
  logicalWidth_ = logicalWidth;
  logicalHeight_ = logicalHeight;
  storageWidth_ = storageWidth;
  storageHeight_ = storageHeight;
  frameBytes_ = frameBytes;
  workspaceBytes_ = requiredWorkspaceBytes(frameBytes_);
  workspace_ = static_cast<uint8_t*>(allocWorkspace(workspaceBytes_));
  if (workspace_ == nullptr) {
    close();
    return false;
  }
  uint8_t* p = workspace_;
  prefixPacked_ = p; p += 6144;
  suffix_ = p; p += 4096;
  stack_ = p; p += 4096;
  globalPalette_ = reinterpret_cast<uint16_t*>(p); p += 512;
  localPalette_ = reinterpret_cast<uint16_t*>(p); p += 512;
  previousCanvas_ = p;

  if (!seekFn_(context_, 0)) { close(); return false; }
  position_ = 0;
  uint8_t header[13];
  if (!readExact(header, sizeof(header))) { close(); return false; }
  if (std::memcmp(header, "GIF87a", 6) != 0 && std::memcmp(header, "GIF89a", 6) != 0) {
    close();
    return false;
  }
  const uint16_t width = uint16_t(header[6]) | (uint16_t(header[7]) << 8);
  const uint16_t height = uint16_t(header[8]) | (uint16_t(header[9]) << 8);
  if (width != logicalWidth_ || height != logicalHeight_) { close(); return false; }

  const uint8_t packed = header[10];
  const bool hasGlobalPalette = (packed & 0x80u) != 0;
  globalPaletteEntries_ = hasGlobalPalette ? uint16_t(1u << ((packed & 0x07u) + 1u)) : 0;
  const uint8_t backgroundIndex = header[11];
  if (hasGlobalPalette) {
    if (!readPalette(globalPalette_, globalPaletteEntries_)) { close(); return false; }
    if (backgroundIndex < globalPaletteEntries_) backgroundColor_ = globalPalette_[backgroundIndex];
  } else {
    backgroundColor_ = 0;
  }

  pendingGce_ = Gce{};
  previousDisposal_ = 0;
  previousCanvasValid_ = false;
  opened_ = true;
  return true;
}

void IDotMatrixCompactGif::close() {
  if (workspace_ != nullptr) freeWorkspace(workspace_);
  workspace_ = nullptr;
  workspaceBytes_ = 0;
  prefixPacked_ = nullptr;
  suffix_ = nullptr;
  stack_ = nullptr;
  globalPalette_ = nullptr;
  localPalette_ = nullptr;
  previousCanvas_ = nullptr;
  context_ = nullptr;
  readFn_ = nullptr;
  seekFn_ = nullptr;
  opened_ = false;
  position_ = 0;
  subRemaining_ = 0;
  subEnded_ = false;
  bitBuffer_ = 0;
  bitCount_ = 0;
}

bool IDotMatrixCompactGif::skipSubBlocks() {
  uint8_t size = 0;
  while (readByte(size)) {
    if (size == 0) return true;
    if (!skipBytes(size)) return false;
  }
  return false;
}

void IDotMatrixCompactGif::applyPreviousDisposal(IDotMatrixRenderer& renderer) {
  if (previousDisposal_ == 3 && previousCanvasValid_ && previousCanvas_ != nullptr && frameBytes_ > 0) {
    uint8_t* target = renderer.animationFrameData();
    if (target != nullptr) std::memcpy(target, previousCanvas_, frameBytes_);
  } else if (previousDisposal_ == 2) {
    const uint8_t r = uint8_t(((backgroundColor_ >> 11) & 0x1Fu) * 255u / 31u);
    const uint8_t g = uint8_t(((backgroundColor_ >> 5) & 0x3Fu) * 255u / 63u);
    const uint8_t b = uint8_t((backgroundColor_ & 0x1Fu) * 255u / 31u);
    for (uint8_t ty = 0; ty < storageHeight_; ++ty) {
      const uint16_t sy = uint16_t(ty) * logicalHeight_ / storageHeight_;
      if (sy < previousTop_ || sy >= uint16_t(previousTop_ + previousHeight_)) continue;
      for (uint8_t tx = 0; tx < storageWidth_; ++tx) {
        const uint16_t sx = uint16_t(tx) * logicalWidth_ / storageWidth_;
        if (sx >= previousLeft_ && sx < uint16_t(previousLeft_ + previousWidth_)) {
          renderer.setAnimationPixel(tx, ty, r, g, b);
        }
      }
    }
  }
  previousCanvasValid_ = false;
}

void IDotMatrixCompactGif::savePreviousCanvas(const IDotMatrixRenderer& renderer) {
  if (previousCanvas_ == nullptr || frameBytes_ == 0 || renderer.pixels() == nullptr) return;
  std::memcpy(previousCanvas_, reinterpret_cast<const uint8_t*>(renderer.pixels()), frameBytes_);
  previousCanvasValid_ = true;
}

uint16_t IDotMatrixCompactGif::prefixGet(uint16_t index) const {
  const size_t base = (size_t(index) >> 1) * 3u;
  if ((index & 1u) == 0) {
    return uint16_t(prefixPacked_[base]) | (uint16_t(prefixPacked_[base + 1] & 0x0Fu) << 8);
  }
  return uint16_t(prefixPacked_[base + 1] >> 4) | (uint16_t(prefixPacked_[base + 2]) << 4);
}

void IDotMatrixCompactGif::prefixSet(uint16_t index, uint16_t value) {
  value &= 0x0FFFu;
  const size_t base = (size_t(index) >> 1) * 3u;
  if ((index & 1u) == 0) {
    prefixPacked_[base] = uint8_t(value & 0xFFu);
    prefixPacked_[base + 1] = uint8_t((prefixPacked_[base + 1] & 0xF0u) | ((value >> 8) & 0x0Fu));
  } else {
    prefixPacked_[base + 1] = uint8_t((prefixPacked_[base + 1] & 0x0Fu) | ((value & 0x0Fu) << 4));
    prefixPacked_[base + 2] = uint8_t((value >> 4) & 0xFFu);
  }
}

void IDotMatrixCompactGif::resetDictionary(uint8_t minCodeSize) {
  minCodeSize_ = minCodeSize;
  clearCode_ = uint16_t(1u << minCodeSize_);
  endCode_ = uint16_t(clearCode_ + 1u);
  nextCode_ = uint16_t(endCode_ + 1u);
  codeSize_ = uint8_t(minCodeSize_ + 1u);
  for (uint16_t i = 0; i < clearCode_; ++i) suffix_[i] = uint8_t(i);
}

bool IDotMatrixCompactGif::readSubByte(uint8_t& value) {
  if (subEnded_) return false;
  if (subRemaining_ == 0) {
    uint8_t size = 0;
    if (!readByte(size)) return false;
    if (size == 0) {
      subEnded_ = true;
      return false;
    }
    subRemaining_ = size;
  }
  if (!readByte(value)) return false;
  --subRemaining_;
  return true;
}

bool IDotMatrixCompactGif::readCode(uint8_t width, uint16_t& code) {
  while (bitCount_ < width) {
    uint8_t value = 0;
    if (!readSubByte(value)) return false;
    bitBuffer_ |= uint32_t(value) << bitCount_;
    bitCount_ = uint8_t(bitCount_ + 8u);
  }
  code = uint16_t(bitBuffer_ & ((1u << width) - 1u));
  bitBuffer_ >>= width;
  bitCount_ = uint8_t(bitCount_ - width);
  return true;
}

void IDotMatrixCompactGif::advanceRow(uint16_t height, bool interlaced, uint16_t& y, uint8_t& pass) const {
  if (!interlaced) {
    ++y;
    return;
  }
  static const uint8_t starts[4] = {0, 4, 2, 1};
  static const uint8_t steps[4] = {8, 8, 4, 2};
  y = uint16_t(y + steps[pass]);
  while (y >= height && pass < 3) {
    ++pass;
    y = starts[pass];
  }
}

bool IDotMatrixCompactGif::emitIndex(
  IDotMatrixRenderer& renderer,
  uint8_t paletteIndex,
  const uint16_t* palette,
  const Gce& gce,
  uint16_t left,
  uint16_t top,
  uint16_t width,
  uint16_t height,
  bool interlaced,
  uint16_t& x,
  uint16_t& y,
  uint8_t& pass,
  size_t& emitted
) {
  if (y >= height || emitted >= size_t(width) * height) return false;
  if (!(gce.transparent && paletteIndex == gce.transparentIndex)) {
    const uint16_t sx = uint16_t(left + x);
    const uint16_t sy = uint16_t(top + y);
    if (sx < logicalWidth_ && sy < logicalHeight_) {
      const uint16_t color = palette[paletteIndex];
      renderer.setAnimationSourcePixel(
        uint8_t(sx), uint8_t(sy),
        uint8_t(((color >> 11) & 0x1Fu) * 255u / 31u),
        uint8_t(((color >> 5) & 0x3Fu) * 255u / 63u),
        uint8_t((color & 0x1Fu) * 255u / 31u)
      );
    }
  }
  ++emitted;
  ++x;
  if (x >= width) {
    x = 0;
    advanceRow(height, interlaced, y, pass);
  }
  return true;
}

bool IDotMatrixCompactGif::decodeImage(
  IDotMatrixRenderer& renderer,
  uint16_t left,
  uint16_t top,
  uint16_t width,
  uint16_t height,
  bool interlaced,
  const uint16_t* palette,
  const Gce& gce
) {
  uint8_t minCode = 0;
  if (!readByte(minCode) || minCode < 2 || minCode > 8) return false;
  resetDictionary(minCode);
  subRemaining_ = 0;
  subEnded_ = false;
  bitBuffer_ = 0;
  bitCount_ = 0;

  int32_t oldCode = -1;
  uint8_t first = 0;
  uint16_t x = 0;
  uint16_t y = 0;
  uint8_t pass = 0;
  size_t emitted = 0;
  const size_t expected = size_t(width) * height;

  while (true) {
    uint16_t code = 0;
    if (!readCode(codeSize_, code)) return false;
    if (code == clearCode_) {
      resetDictionary(minCode);
      oldCode = -1;
      continue;
    }
    if (code == endCode_) break;
    if (code > nextCode_) return false;

    uint16_t inCode = code;
    size_t stackCount = 0;
    if (code == nextCode_) {
      if (oldCode < 0 || stackCount >= 4096) return false;
      stack_[stackCount++] = first;
      code = uint16_t(oldCode);
    }
    while (code >= clearCode_) {
      if (code >= nextCode_ || stackCount >= 4096) return false;
      stack_[stackCount++] = suffix_[code];
      code = prefixGet(code);
    }
    first = uint8_t(code);
    if (stackCount >= 4096) return false;
    stack_[stackCount++] = first;

    while (stackCount > 0) {
      if (!emitIndex(
            renderer, stack_[--stackCount], palette, gce,
            left, top, width, height, interlaced, x, y, pass, emitted
          )) return false;
    }

    if (oldCode >= 0 && nextCode_ < 4096) {
      prefixSet(nextCode_, uint16_t(oldCode));
      suffix_[nextCode_] = first;
      ++nextCode_;
      if (nextCode_ == (1u << codeSize_) && codeSize_ < 12) ++codeSize_;
    }
    oldCode = inCode;
  }

  // Consume any bytes left in the image-data sub-block chain after EOI.
  bitBuffer_ = 0;
  bitCount_ = 0;
  while (!subEnded_) {
    uint8_t ignored = 0;
    if (!readSubByte(ignored)) break;
  }
  return emitted == expected;
}

IDotMatrixCompactGif::Result IDotMatrixCompactGif::decodeNextFrame(
  IDotMatrixRenderer& renderer, uint16_t& delayMs
) {
  delayMs = 10;
  if (!opened_) return Result::Error;

  applyPreviousDisposal(renderer);

  while (true) {
    uint8_t marker = 0;
    if (!readByte(marker)) return Result::Error;
    if (marker == 0x3B) return Result::End;

    if (marker == 0x21) {
      uint8_t label = 0;
      if (!readByte(label)) return Result::Error;
      if (label == 0xF9) {
        uint8_t size = 0;
        uint8_t gce[4] = {0, 0, 0, 0};
        uint8_t terminator = 0;
        if (!readByte(size) || size != 4 || !readExact(gce, sizeof(gce)) ||
            !readByte(terminator) || terminator != 0) return Result::Error;
        pendingGce_.disposal = uint8_t((gce[0] >> 2) & 0x07u);
        pendingGce_.transparent = (gce[0] & 0x01u) != 0;
        pendingGce_.delayCs = uint16_t(gce[1]) | (uint16_t(gce[2]) << 8);
        if (pendingGce_.delayCs == 0) pendingGce_.delayCs = 1;
        pendingGce_.transparentIndex = gce[3];
      } else {
        if (!skipSubBlocks()) return Result::Error;
      }
      continue;
    }

    if (marker != 0x2C) return Result::Error;
    uint8_t descriptor[9];
    if (!readExact(descriptor, sizeof(descriptor))) return Result::Error;
    const uint16_t left = uint16_t(descriptor[0]) | (uint16_t(descriptor[1]) << 8);
    const uint16_t top = uint16_t(descriptor[2]) | (uint16_t(descriptor[3]) << 8);
    const uint16_t width = uint16_t(descriptor[4]) | (uint16_t(descriptor[5]) << 8);
    const uint16_t height = uint16_t(descriptor[6]) | (uint16_t(descriptor[7]) << 8);
    const uint8_t packed = descriptor[8];
    if (width == 0 || height == 0 || left + width > logicalWidth_ || top + height > logicalHeight_) {
      return Result::Error;
    }
    const bool localPalette = (packed & 0x80u) != 0;
    const bool interlaced = (packed & 0x40u) != 0;
    const uint16_t* palette = globalPalette_;
    if (localPalette) {
      const uint16_t entries = uint16_t(1u << ((packed & 0x07u) + 1u));
      if (!readPalette(localPalette_, entries)) return Result::Error;
      palette = localPalette_;
    } else if (globalPaletteEntries_ == 0) {
      return Result::Error;
    }

    if (pendingGce_.disposal == 3) savePreviousCanvas(renderer);
    const Gce frameGce = pendingGce_;
    if (!decodeImage(renderer, left, top, width, height, interlaced, palette, frameGce)) {
      return Result::Error;
    }

    previousDisposal_ = frameGce.disposal;
    previousLeft_ = left;
    previousTop_ = top;
    previousWidth_ = width;
    previousHeight_ = height;
    delayMs = uint16_t(frameGce.delayCs * 10u);
    if (delayMs < 10) delayMs = 10;
    pendingGce_ = Gce{};
    return Result::Frame;
  }
}
