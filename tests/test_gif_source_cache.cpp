#include "../IDotMatrixGifSourceStage.h"

#include <cassert>
#include <cstdint>

#include "wled.h"

TestFS WLED_FS;

static void writeFile(const char* path, uint8_t seed, size_t bytes) {
  File file = WLED_FS.open(path, "w");
  assert(file);
  for (size_t i = 0; i < bytes; ++i) {
    const uint8_t value = uint8_t(seed + i);
    assert(file.write(&value, 1) == 1);
  }
  file.close();
}

int main() {
  WLED_FS.clear();
  writeFile("/a.gif", 1, 12);
  writeFile("/b.gif", 2, 12);
  writeFile("/c.gif", 3, 20);

  IDotMatrixGifSourceStage stage;
  assert(stage.cacheEnabled());
  assert(stage.cacheEntries() == 0);

  // First durable play is a miss + real filesystem stage + cache store.
  assert(stage.stage("/a.gif", true));
  assert(stage.active());
  assert(stage.activeFromCache());
  assert(stage.bytes() == 12);
  assert(stage.attempts() == 1);
  assert(stage.successes() == 1);
  assert(stage.fallbacks() == 0);
  assert(stage.cacheMisses() == 1);
  assert(stage.cacheHits() == 0);
  assert(stage.cacheStores() == 1);
  assert(stage.cacheEntries() == 1);
  assert(stage.cacheBytes() == 12);
  stage.release();
  assert(!stage.active());
  assert(stage.cacheEntries() == 1);

  // Reopening the same durable path reuses PSRAM and must not count as a new
  // stage attempt or filesystem-copy success.
  assert(stage.stage("/a.gif", true));
  assert(stage.activeFromCache());
  assert(stage.cacheHits() == 1);
  assert(stage.cacheMisses() == 1);
  assert(stage.attempts() == 1);
  assert(stage.successes() == 1);

  // Dev.11 look-ahead can warm another durable source without disturbing the
  // decoder-equivalent active source. The 32-byte test budget can retain A+B.
  assert(stage.prefetch("/b.gif", 12));
  assert(stage.active());
  assert(stage.activeFromCache());
  assert(stage.bytes() == 12);
  assert(stage.cacheEntries() == 2);
  assert(stage.cacheBytes() == 24);
  assert(stage.prefetchAttempts() == 1);
  assert(stage.prefetchSuccesses() == 1);
  assert(stage.prefetchAlreadyCached() == 0);
  assert(stage.prefetchFailures() == 0);
  assert(stage.prefetchBytes() == 12);
  stage.release();

  // Re-prefetching a resident generation is a cheap no-copy success.
  assert(stage.prefetch("/b.gif", 12));
  assert(stage.prefetchAttempts() == 1);
  assert(stage.prefetchSuccesses() == 1);
  assert(stage.prefetchAlreadyCached() == 1);

  // Adding a 20-byte C to resident 12-byte A+B requires one LRU eviction.
  assert(stage.stage("/c.gif", true));
  assert(stage.cacheMisses() == 2);
  assert(stage.cacheStores() == 3);
  assert(stage.cacheEvictions() == 1);
  assert(stage.cacheEntries() == 2);
  assert(stage.cacheBytes() == 32);
  stage.release();

  // A was evicted, so it stages again.
  assert(stage.stage("/a.gif", true));
  assert(stage.cacheMisses() == 3);
  assert(stage.attempts() == 3);
  assert(stage.successes() == 3);
  assert(stage.cacheEvictions() == 2);

  // Invalidation is safe while the decoder-equivalent consumer still owns the
  // active cached source. Retirement is deferred until release().
  stage.invalidate("/a.gif");
  assert(stage.cacheInvalidations() == 1);
  assert(stage.cacheEntries() == 2);
  assert(stage.active());
  stage.release();
  assert(stage.cacheEntries() == 1);
  assert(stage.cacheBytes() == 20);

  // Non-durable media keeps the original one-play staging lifetime and never
  // enters the persistent source cache.
  assert(stage.stage("/b.gif", false));
  assert(stage.active());
  assert(!stage.activeFromCache());
  assert(stage.cacheEntries() == 1);
  stage.release();
  assert(stage.cacheEntries() == 1);

  // Size mismatch is a prefetch failure and never replaces a valid generation.
  assert(!stage.prefetch("/b.gif", 99));
  assert(stage.prefetchFailures() == 1);

  // Dev.12 telemetry semantics: a source may first enter PSRAM through
  // look-ahead prefetch and only later become the active AnimatedGIF source.
  // Activating that cached source must update peakBytes even though no normal
  // stage attempt/copy occurs, so the reported invariant peak >= current bytes
  // remains true.
  writeFile("/peak.gif", 4, 24);
  IDotMatrixGifSourceStage peakStage;
  assert(peakStage.prefetch("/peak.gif", 24));
  assert(peakStage.peakBytes() == 0);
  assert(peakStage.stage("/peak.gif", true));
  assert(peakStage.activeFromCache());
  assert(peakStage.bytes() == 24);
  assert(peakStage.peakBytes() == 24);
  assert(peakStage.attempts() == 0);
  assert(peakStage.successes() == 0);
  assert(peakStage.cacheHits() == 1);

  return 0;
}
