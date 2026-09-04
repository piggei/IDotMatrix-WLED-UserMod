#include "../IDotMatrixMedia.h"
#include "../IDotMatrixRenderer.h"

#include <cassert>
#include <cstring>
#include <vector>
#include <zlib.h>

#include "wled.h"

TestFS WLED_FS;

static void appendBE32(std::vector<uint8_t>& bytes, uint32_t value) {
  bytes.push_back(uint8_t(value >> 24));
  bytes.push_back(uint8_t(value >> 16));
  bytes.push_back(uint8_t(value >> 8));
  bytes.push_back(uint8_t(value));
}

static void appendChunk(
  std::vector<uint8_t>& png, const char type[4],
  const uint8_t* data, size_t length
) {
  appendBE32(png, uint32_t(length));
  png.insert(png.end(), type, type + 4);
  if (length > 0) png.insert(png.end(), data, data + length);
  appendBE32(png, 0);  // The lightweight decoder does not consume chunk CRCs.
}

static std::vector<uint8_t> makeRgb16Png() {
  const uint8_t signature[] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  std::vector<uint8_t> png(signature, signature + sizeof(signature));
  uint8_t ihdr[13]{};
  ihdr[3] = 16;
  ihdr[7] = 16;
  ihdr[8] = 8;
  ihdr[9] = 2;
  appendChunk(png, "IHDR", ihdr, sizeof(ihdr));

  std::vector<uint8_t> raw(16u * (1u + 16u * 3u), 0);
  raw[1] = 10;
  raw[2] = 20;
  raw[3] = 30;
  uLongf compressedLength = compressBound(static_cast<uLong>(raw.size()));
  std::vector<uint8_t> compressed(compressedLength);
  assert(compress2(
    compressed.data(), &compressedLength, raw.data(),
    static_cast<uLong>(raw.size()), Z_BEST_SPEED
  ) == Z_OK);
  compressed.resize(static_cast<size_t>(compressedLength));
  appendChunk(png, "IDAT", compressed.data(), compressed.size());
  appendChunk(png, "IEND", nullptr, 0);
  return png;
}

int main() {
  IDotMatrixRenderer renderer;
  assert(renderer.begin(0x01));
  IDotMatrixMedia media(renderer);

  const uint8_t notPng[] = {1, 2, 3, 4};
  assert(!media.decodePng(notPng, sizeof(notPng)));

  const std::vector<uint8_t> png = makeRgb16Png();
  assert(media.decodePng(png.data(), png.size()));
  const IDotMatrixRenderer::Pixel* decoded = renderer.pixel(0, 0);
  assert(decoded != nullptr);
  assert(decoded->red == 10 && decoded->green == 20 && decoded->blue == 30);

  const uint8_t gifA[] = {'G', 'I', 'F', '8', '9', 'a'};
  assert(media.beginGif(sizeof(gifA)));
  assert(media.writeGif(0, gifA, 3));
  assert(media.writeGif(3, gifA + 3, 3));
  assert(media.completeGif(true));
  media.loop(0);  // RX -> PLAY promotion
  media.loop(0);  // decoder construction/open
  assert(media.gifActive());

  // A failed replacement must not be published.
  assert(media.beginGif(sizeof(gifA)));
  assert(media.writeGif(0, gifA, sizeof(gifA)));
  assert(!media.completeGif(false));

  // Two valid transfers can complete before loop() promotes either one. The
  // newest complete file wins without confusing the RX and pending slots.
  assert(media.beginGif(sizeof(gifA)));
  assert(media.writeGif(0, gifA, sizeof(gifA)));
  assert(media.completeGif(true));
  assert(media.beginGif(sizeof(gifA)));
  assert(media.writeGif(0, gifA, sizeof(gifA)));
  assert(media.completeGif(true));
  media.loop(1);
  media.loop(1);
  assert(media.gifActive());

  media.stopPlayback();
  assert(!media.gifActive());
}
