#pragma once

#include <cstddef>
#include <cstdint>

#ifndef IDOT_GIF_PSRAM_STAGE_MAX
#if defined(IDOT_WAVESHARE_S3_RGB_MATRIX)
#define IDOT_GIF_PSRAM_STAGE_MAX (2u * 1024u * 1024u)
#else
#define IDOT_GIF_PSRAM_STAGE_MAX 0u
#endif
#endif

#ifndef IDOT_GIF_PSRAM_STAGE_RESERVE
#define IDOT_GIF_PSRAM_STAGE_RESERVE (4u * 1024u * 1024u)
#endif

#ifndef IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_BYTES
#if defined(IDOT_WAVESHARE_S3_RGB_MATRIX)
#define IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_BYTES (1u * 1024u * 1024u)
#else
#define IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_BYTES 0u
#endif
#endif

#ifndef IDOT_GIF_PSRAM_SOURCE_CACHE_ENTRY_MAX
#if defined(IDOT_WAVESHARE_S3_RGB_MATRIX)
#define IDOT_GIF_PSRAM_SOURCE_CACHE_ENTRY_MAX (512u * 1024u)
#else
#define IDOT_GIF_PSRAM_SOURCE_CACHE_ENTRY_MAX 0u
#endif
#endif

#ifndef IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_ENTRIES
#if defined(IDOT_WAVESHARE_S3_RGB_MATRIX)
// Carousel owns at most twelve slots. Keeping the metadata capacity equal to
// that limit avoids artificial entry-count churn; the byte budget remains the
// primary admission/eviction guard.
#define IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_ENTRIES 12u
#else
#define IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_ENTRIES 0u
#endif
#endif

// Owns the active whole-file GIF source image used by direct AnimatedGIF
// playback. Dev.10 extends the dev.9 single-stage owner with a bounded PSRAM
// reuse cache for durable Carousel GIF files. Dev.11 can also warm one future
// durable source without changing the currently active decoder image. Transient/
// app-upload GIFs retain the dev.8/dev.9 one-play lifetime. The decoder callback
// contract is unchanged: data() always points at the currently active source
// image while active().
class IDotMatrixGifSourceStage final {
public:
  IDotMatrixGifSourceStage() = default;
  ~IDotMatrixGifSourceStage();

  IDotMatrixGifSourceStage(const IDotMatrixGifSourceStage&) = delete;
  IDotMatrixGifSourceStage& operator=(const IDotMatrixGifSourceStage&) = delete;

  bool stage(const char* path, bool allowPersistentReuse = false);
  bool prefetch(const char* path, size_t expectedBytes = 0);
  void release();
  void invalidate(const char* path);
  void clearCache();

  bool active() const { return activeBuffer_ != nullptr; }
  bool activeFromCache() const { return activeEntry_ >= 0; }
  const uint8_t* data() const { return activeBuffer_; }
  size_t bytes() const { return activeBytes_; }
  // Largest source image that has been active for AnimatedGIF since boot.
  // Includes normal staging and activation from the persistent/prefetched cache.
  size_t peakBytes() const { return peakBytes_; }
  uint32_t attempts() const { return attempts_; }
  uint32_t successes() const { return successes_; }
  uint32_t fallbacks() const { return fallbacks_; }

  bool cacheEnabled() const {
    return IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_BYTES > 0 &&
      IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_ENTRIES > 0;
  }
  uint8_t cacheEntries() const;
  size_t cacheBytes() const { return cacheBytes_; }
  uint32_t cacheHits() const { return cacheHits_; }
  uint32_t cacheMisses() const { return cacheMisses_; }
  uint32_t cacheStores() const { return cacheStores_; }
  uint32_t cacheEvictions() const { return cacheEvictions_; }
  uint32_t cacheInvalidations() const { return cacheInvalidations_; }
  uint32_t prefetchAttempts() const { return prefetchAttempts_; }
  uint32_t prefetchSuccesses() const { return prefetchSuccesses_; }
  uint32_t prefetchAlreadyCached() const { return prefetchAlreadyCached_; }
  uint32_t prefetchFailures() const { return prefetchFailures_; }
  size_t prefetchBytes() const { return prefetchBytes_; }

  static constexpr size_t maxBytes() { return IDOT_GIF_PSRAM_STAGE_MAX; }
  static constexpr size_t reserveBytes() { return IDOT_GIF_PSRAM_STAGE_RESERVE; }
  static constexpr size_t cacheMaxBytes() { return IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_BYTES; }
  static constexpr size_t cacheEntryMaxBytes() { return IDOT_GIF_PSRAM_SOURCE_CACHE_ENTRY_MAX; }
  static constexpr uint8_t cacheMaxEntries() {
    return static_cast<uint8_t>(IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_ENTRIES);
  }

private:
  static constexpr size_t CACHE_PATH_BYTES = 48u;
  static constexpr size_t CACHE_STORAGE_ENTRIES =
    IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_ENTRIES > 0
      ? IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_ENTRIES : 1u;
  static_assert(IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_ENTRIES <= 127u,
    "GIF source cache entry count must fit the signed active-entry index");

  struct CacheEntry {
    uint8_t* buffer = nullptr;
    size_t bytes = 0;
    uint32_t lastUse = 0;
    bool valid = false;
    char path[CACHE_PATH_BYTES] = {0};
  };

  int findValidEntry(const char* path) const;
  int findFreeEntry() const;
  int findLruEntry() const;
  void freeEntry(size_t index, bool eviction);
  bool storeActiveInCache(const char* path, size_t sourceBytes);
  bool storeBufferInCache(const char* path, uint8_t* buffer, size_t sourceBytes);
  bool makeCacheRoom(size_t sourceBytes);
  void touch(CacheEntry& entry);

  CacheEntry cache_[CACHE_STORAGE_ENTRIES]{};
  uint8_t* activeBuffer_ = nullptr;
  size_t activeBytes_ = 0;
  int8_t activeEntry_ = -1;
  size_t peakBytes_ = 0;
  size_t cacheBytes_ = 0;
  uint32_t useSerial_ = 0;
  uint32_t attempts_ = 0;
  uint32_t successes_ = 0;
  uint32_t fallbacks_ = 0;
  uint32_t cacheHits_ = 0;
  uint32_t cacheMisses_ = 0;
  uint32_t cacheStores_ = 0;
  uint32_t cacheEvictions_ = 0;
  uint32_t cacheInvalidations_ = 0;
  uint32_t prefetchAttempts_ = 0;
  uint32_t prefetchSuccesses_ = 0;
  uint32_t prefetchAlreadyCached_ = 0;
  uint32_t prefetchFailures_ = 0;
  size_t prefetchBytes_ = 0;
};
