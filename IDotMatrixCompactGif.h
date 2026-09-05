#pragma once

#include <cstddef>
#include <cstdint>

class IDotMatrixRenderer;

class IDotMatrixCompactGif {
public:
  using ReadFn = int32_t (*)(void* context, uint8_t* buffer, size_t length);
  using SeekFn = bool (*)(void* context, uint32_t position);

  enum class Result : uint8_t { Frame, End, Error };

  IDotMatrixCompactGif() = default;
  ~IDotMatrixCompactGif();

  IDotMatrixCompactGif(const IDotMatrixCompactGif&) = delete;
  IDotMatrixCompactGif& operator=(const IDotMatrixCompactGif&) = delete;

  bool open(
    void* context,
    ReadFn readFn,
    SeekFn seekFn,
    uint8_t logicalWidth,
    uint8_t logicalHeight,
    uint8_t storageWidth,
    uint8_t storageHeight,
    size_t frameBytes
  );
  Result decodeNextFrame(IDotMatrixRenderer& renderer, uint16_t& delayMs);
  void close();

  bool isOpen() const { return opened_; }
  size_t workspaceBytes() const { return workspaceBytes_; }
  static size_t requiredWorkspaceBytes(size_t frameBytes) {
    return 6144u + 4096u + 4096u + 512u + 512u + frameBytes;
  }

private:
  struct Gce {
    uint8_t disposal = 0;
    bool transparent = false;
    uint8_t transparentIndex = 0;
    uint16_t delayCs = 1;
  };

  bool readExact(uint8_t* dst, size_t length);
  bool readByte(uint8_t& value);
  bool skipBytes(size_t length);
  bool readPalette(uint16_t* target, uint16_t entries);
  bool skipSubBlocks();
  bool readSubByte(uint8_t& value);
  bool readCode(uint8_t width, uint16_t& code);
  bool decodeImage(
    IDotMatrixRenderer& renderer,
    uint16_t left,
    uint16_t top,
    uint16_t width,
    uint16_t height,
    bool interlaced,
    const uint16_t* palette,
    const Gce& gce
  );
  void applyPreviousDisposal(IDotMatrixRenderer& renderer);
  void savePreviousCanvas(const IDotMatrixRenderer& renderer);
  uint16_t prefixGet(uint16_t index) const;
  void prefixSet(uint16_t index, uint16_t value);
  void resetDictionary(uint8_t minCodeSize);
  bool emitIndex(
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
  );
  void advanceRow(uint16_t height, bool interlaced, uint16_t& y, uint8_t& pass) const;

  void* context_ = nullptr;
  ReadFn readFn_ = nullptr;
  SeekFn seekFn_ = nullptr;
  bool opened_ = false;
  uint32_t position_ = 0;

  uint8_t logicalWidth_ = 0;
  uint8_t logicalHeight_ = 0;
  uint8_t storageWidth_ = 0;
  uint8_t storageHeight_ = 0;
  size_t frameBytes_ = 0;

  uint8_t* workspace_ = nullptr;
  size_t workspaceBytes_ = 0;
  uint8_t* prefixPacked_ = nullptr;
  uint8_t* suffix_ = nullptr;
  uint8_t* stack_ = nullptr;
  uint16_t* globalPalette_ = nullptr;
  uint16_t* localPalette_ = nullptr;
  uint8_t* previousCanvas_ = nullptr;

  uint16_t globalPaletteEntries_ = 0;
  uint16_t backgroundColor_ = 0;
  Gce pendingGce_{};

  uint8_t previousDisposal_ = 0;
  uint16_t previousLeft_ = 0;
  uint16_t previousTop_ = 0;
  uint16_t previousWidth_ = 0;
  uint16_t previousHeight_ = 0;
  bool previousCanvasValid_ = false;

  uint16_t clearCode_ = 0;
  uint16_t endCode_ = 0;
  uint16_t nextCode_ = 0;
  uint8_t codeSize_ = 0;
  uint8_t minCodeSize_ = 0;

  uint8_t subRemaining_ = 0;
  bool subEnded_ = false;
  uint32_t bitBuffer_ = 0;
  uint8_t bitCount_ = 0;
};
