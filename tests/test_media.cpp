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


static void advanceGifOpen(IDotMatrixMedia& media, uint32_t now) {
  media.loop(now);  // promote
  media.loop(now);  // staging delay
  media.loop(now);  // open/cache construction
  for (int i = 0; i < 8 && !media.gifActive(); ++i) media.loop(now);
}


static void writeStubFile(const char* path, const uint8_t* data, size_t length) {
  File file = WLED_FS.open(path, "w");
  assert(file);
  assert(file.write(data, length) == length);
  file.close();
}

static void testTransientBootRecovery() {
  WLED_FS.clear();
  const uint8_t oldPlay[] = {'G','I','F','8','9','a'};
  const uint8_t oldCache[] = {'I','D','C','1',16,16,0,3, 10,0, 1,2,3};
  const uint8_t candidate[] = {'G','I','F','8','7','a'};
  writeStubFile("/idot_play.bak", oldPlay, sizeof(oldPlay));
  writeStubFile("/idot_cache.bak", oldCache, sizeof(oldCache));
  writeStubFile("/idot_play.gif", candidate, sizeof(candidate));
  writeStubFile("/idot_cache.new", candidate, sizeof(candidate));
  writeStubFile("/idot_rx0.tmp", candidate, sizeof(candidate));

  IDotMatrixRenderer renderer;
  assert(renderer.begin(0x01));
  {
    IDotMatrixMedia media(renderer);
    assert(WLED_FS.exists("/idot_play.gif"));
    assert(WLED_FS.exists("/idot_cache.bin"));
    assert(!WLED_FS.exists("/idot_play.bak"));
    assert(!WLED_FS.exists("/idot_cache.bak"));
    assert(!WLED_FS.exists("/idot_cache.new"));
    assert(!WLED_FS.exists("/idot_rx0.tmp"));
  }
}

static void testGifPromotionFailures() {
  const uint8_t gif[] = {'G', 'I', 'F', '8', '9', 'a'};

  // Direct rename succeeds.
  {
    WLED_FS.clear();
    IDotMatrixRenderer renderer;
    assert(renderer.begin(0x01));
    IDotMatrixMedia media(renderer);
    assert(media.beginGif(sizeof(gif)));
    assert(media.writeGif(0, gif, sizeof(gif)));
    assert(media.completeGif(true));
    advanceGifOpen(media, 100);
    assert(media.gifActive());
    assert(media.lastError() == IDotMatrixMedia::Error::None);
  }

  // Direct rename fails, streamed-copy fallback succeeds.
  {
    WLED_FS.clear();
    IDotMatrixRenderer renderer;
    assert(renderer.begin(0x01));
    IDotMatrixMedia media(renderer);
    assert(media.beginGif(sizeof(gif)));
    assert(media.writeGif(0, gif, sizeof(gif)));
    assert(media.completeGif(true));
    WLED_FS.failRename("/idot_rx0.tmp", "/idot_play.gif");
    advanceGifOpen(media, 200);
    assert(media.gifActive());
    assert(media.lastError() == IDotMatrixMedia::Error::None);
  }

  // Both promotion mechanisms fail.  The failure must be observable and a
  // following valid GIF must still be able to stage and play.
  {
    WLED_FS.clear();
    IDotMatrixRenderer renderer;
    assert(renderer.begin(0x01));
    IDotMatrixMedia media(renderer);
    assert(media.beginGif(sizeof(gif)));
    assert(media.writeGif(0, gif, sizeof(gif)));
    assert(media.completeGif(true));
    WLED_FS.failRename("/idot_rx0.tmp", "/idot_play.gif");
    WLED_FS.failOpenWrite("/idot_play.gif");
    advanceGifOpen(media, 300);
    assert(!media.gifActive());
    assert(media.lastError() == IDotMatrixMedia::Error::GifCacheIo);

    WLED_FS.resetFailures();
    assert(media.beginGif(sizeof(gif)));
    assert(media.writeGif(0, gif, sizeof(gif)));
    assert(media.completeGif(true));
    advanceGifOpen(media, 301);
    assert(media.gifActive());
    assert(media.lastError() == IDotMatrixMedia::Error::None);
  }
}

static void testPersistentCarouselCacheReuse() {
#if IDOT_GIF_BITS >= 12
  WLED_FS.clear();
  const uint8_t gif[] = {'G', 'I', 'F', '8', '9', 'a'};
  {
    File source = WLED_FS.open("/idot_a0.gif", "w");
    assert(source);
    assert(source.write(gif, sizeof(gif)) == sizeof(gif));
    source.close();
  }

  IDotMatrixRenderer renderer;
  assert(renderer.begin(0x01));
  IDotMatrixMedia media(renderer);
  assert(media.queueStoredGif("/idot_a0.gif", "/idot_c0.bin"));
  for (int i = 0; i < 12 && !media.gifActive(); ++i) media.loop(100);
  assert(media.gifActive());
  assert(media.gifCacheBuildCount() == 1);
  assert(media.gifCacheReuseCount() == 0);
  assert(WLED_FS.exists("/idot_c0.bin"));

  media.stopPlayback();
  assert(WLED_FS.exists("/idot_c0.bin"));
  assert(media.queueStoredGif("/idot_a0.gif", "/idot_c0.bin"));
  for (int i = 0; i < 6 && !media.gifActive(); ++i) media.loop(200);
  assert(media.gifActive());
  assert(media.gifCacheBuildCount() == 1);
  assert(media.gifCacheReuseCount() == 1);
#endif
}

int main() {
  testTransientBootRecovery();
  testGifPromotionFailures();
  testPersistentCarouselCacheReuse();
  WLED_FS.clear();

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
  media.loop(0);  // one loop reserved for staging
  media.loop(0);  // decoder construction/open or cache construction
  for (int i = 0; i < 4 && !media.gifActive(); ++i) media.loop(0);
  assert(media.gifActive());
#if IDOT_GIF_BITS >= 12
  assert(media.gifCachedFrames() == 1);
  media.loop(0);  // play first cached frame
  const IDotMatrixRenderer::Pixel* cached = renderer.pixel(0, 0);
  assert(cached != nullptr);
  assert(cached->red == 255 && cached->green == 255 && cached->blue == 255);
#endif

  // A failed replacement must not be published.
  assert(media.beginGif(sizeof(gifA)));
  assert(media.writeGif(0, gifA, sizeof(gifA)));
  assert(!media.completeGif(false));
#if IDOT_GIF_BITS >= 12
  assert(media.gifActive());
  assert(media.gifCachedFrames() == 1);
#endif

#if IDOT_GIF_BITS >= 12
  // If a replacement reaches commit but promotion still fails, the previous
  // known-good committed GIF/cache pair is restored and remains playable.
  WLED_FS.failRename("/idot_rx0.tmp", "/idot_play.gif");
  WLED_FS.failRename("/idot_rx1.tmp", "/idot_play.gif");
  WLED_FS.failOpenWrite("/idot_play.gif");
  assert(media.beginGif(sizeof(gifA)));
  assert(media.writeGif(0, gifA, sizeof(gifA)));
  assert(media.completeGif(true));
  for (int i = 0; i < 12; ++i) media.loop(9);
  assert(media.gifActive());
  assert(media.lastError() == IDotMatrixMedia::Error::GifCacheIo);
  assert(WLED_FS.exists("/idot_play.gif"));
  assert(WLED_FS.exists("/idot_cache.bin"));
  WLED_FS.resetFailures();

  // A valid replacement is staged without destroying the current cached GIF.
  // Only after the replacement cache has been built and validated is the
  // committed /idot_play.gif + /idot_cache.bin pair swapped.
  for (int cycle = 0; cycle < 12; ++cycle) {
    assert(media.beginGif(sizeof(gifA)));
    assert(media.writeGif(0, gifA, sizeof(gifA)));
    assert(media.completeGif(true));
    assert(media.gifActive());
    assert(media.gifCachedFrames() == 1);
    for (int i = 0; i < 12; ++i) media.loop(uint32_t(10 + cycle));
    assert(media.gifActive());
    assert(media.gifCachedFrames() == 1);
    assert(WLED_FS.exists("/idot_play.gif"));
    assert(WLED_FS.exists("/idot_cache.bin"));
  }
#endif

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
  media.loop(1);
  for (int i = 0; i < 4 && !media.gifActive(); ++i) media.loop(1);
  assert(media.gifActive());

  media.stopPlayback();
  assert(!media.gifActive());
}
