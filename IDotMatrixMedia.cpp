#include "IDotMatrixMedia.h"

#include "IDotMatrixRenderer.h"
#include "wled.h"
#include <AnimatedGIF.h>
#include <cstring>
#include <new>

#if __has_include(<rom/miniz.h>)
#include <rom/miniz.h>
#elif __has_include(<miniz.h>)
#include <miniz.h>
#else
#error "ESP32 miniz is required for iDotMatrix PNG decoding"
#endif

namespace {
constexpr char GIF_PLAY[] = "/idot_play.gif";
constexpr const char* GIF_RX[] = {"/idot_rx0.tmp", "/idot_rx1.tmp"};
IDotMatrixMedia* activeMedia = nullptr;
File gifRxFile;

uint32_t be32(const uint8_t* value) {
  return (uint32_t(value[0]) << 24) | (uint32_t(value[1]) << 16) |
    (uint32_t(value[2]) << 8) | value[3];
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


bool IDotMatrixMedia::beginGif(size_t byteLength) {
  cancelGifReceive();
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
  // Teardown/promotion and decoder construction intentionally happen on
  // different WLED loop iterations.
  if (promotePending_) {
    promotePending_ = false;
    if (promoteGif()) openPending_ = true;
    return;
  }
  if (openPending_) {
    openPending_ = false;
    openGif();
    return;
  }
  if (!gifPlaying_ || decoder_ == nullptr || int32_t(now - nextFrameAt_) < 0) return;
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
  if (width == 0 || height == 0 || width > 16 || height > 16) return false;
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
    WLED_FS.remove(rx);
    pendingSlot_ = -1;
    return false;
  }
  destroyDecoder();
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

bool IDotMatrixMedia::openGif() {
  if (!WLED_FS.exists(GIF_PLAY) || !inspectGifFile(GIF_PLAY)) {
    return false;
  }
  if (!renderer_.beginAnimation()) return false;
  decoder_ = new (decoderStorage_) AnimatedGIF();
  decoder_->begin(LITTLE_ENDIAN_PIXELS);
  decoder_->setDrawType(GIF_DRAW_RAW);
  if (!decoder_->open(GIF_PLAY, openFile, closeFile, readFile, seekFile, drawGif)) {
    destroyDecoder();
    return false;
  }
  decoder_->reset();
  gifPlaying_ = true;
  restartPending_ = false;
  nextFrameAt_ = millis();
  return true;
}

void IDotMatrixMedia::destroyDecoder() {
  gifPlaying_ = false;
  restartPending_ = false;
  frameDrawn_ = false;
  if (decoder_ != nullptr) {
    decoder_->close();
    decoder_->~AnimatedGIF();
    decoder_ = nullptr;
  }
  renderer_.endAnimation();
}

void IDotMatrixMedia::stopPlayback() {
  promotePending_ = false;
  openPending_ = false;
  if (pendingSlot_ >= 0) {
    WLED_FS.remove(GIF_RX[uint8_t(pendingSlot_)]);
    pendingSlot_ = -1;
  }
  destroyDecoder();
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

void IDotMatrixMedia::drawGif(GIFDRAW* draw) {
  if (activeMedia == nullptr || draw == nullptr) return;
  IDotMatrixMedia& self = *activeMedia;
  self.frameDrawn_ = true;
  const int y = draw->iY + draw->y;
  if (y < 0 || y >= self.renderer_.height()) return;
  int width = draw->iWidth;
  if (draw->iX + width > self.renderer_.width()) width = self.renderer_.width() - draw->iX;
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
    if (targetX < 0 || targetX >= self.renderer_.width()) continue;
    const uint8_t index = indexes[x];
    if (draw->ucHasTransparency && index == draw->ucTransparent) continue;
    const uint16_t rgb = draw->pPalette[index];
    self.renderer_.setAnimationPixel(
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
  if (width != renderer_.width() || height != renderer_.height() || depth != 8 ||
      compression != 0 || filter != 0 || interlace != 0 ||
      (colorType != 2 && colorType != 6) || idatBytes == 0) {
        return false;
  }
  const uint8_t bpp = colorType == 6 ? 4 : 3;
  const size_t rowBytes = size_t(width) * bpp;
  const size_t rawSize = (rowBytes + 1) * height;
  uint8_t* idat = new (std::nothrow) uint8_t[idatBytes];
  uint8_t* raw = new (std::nothrow) uint8_t[rawSize];
  tinfl_decompressor* inflater = static_cast<tinfl_decompressor*>(malloc(sizeof(tinfl_decompressor)));
  if (idat == nullptr || raw == nullptr || inflater == nullptr) {
    delete[] idat; delete[] raw; free(inflater); return false;
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
  free(inflater);
  delete[] idat;
  if (output != idatBytes || status != TINFL_STATUS_DONE || outputBytes != rawSize) {
    delete[] raw; return false;
  }
  for (uint32_t y = 0; y < height; ++y) {
    uint8_t* row = raw + size_t(y) * (rowBytes + 1);
    uint8_t* current = row + 1;
    uint8_t* previous = y ? raw + size_t(y - 1) * (rowBytes + 1) + 1 : nullptr;
    if (row[0] > 4) { delete[] raw; return false; }
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
  delete[] raw;
  ok = renderer_.completeRawImage(ok);
  return ok;
}
