#include "IDotMatrixGifSourceStage.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#if defined(ARDUINO_ARCH_ESP32)
#include "wled.h"
#include <esp_heap_caps.h>
#include <esp32-hal-psram.h>
#elif defined(IDOT_GIF_SOURCE_CACHE_HOST_TEST)
#include "wled.h"
#endif

namespace {

#if (defined(ARDUINO_ARCH_ESP32) || defined(IDOT_GIF_SOURCE_CACHE_HOST_TEST)) && IDOT_GIF_PSRAM_STAGE_MAX > 0
bool stagePlatformAvailable() {
#if defined(ARDUINO_ARCH_ESP32)
  return psramFound();
#elif defined(IDOT_GIF_SOURCE_CACHE_HOST_TEST)
  return true;
#else
  return false;
#endif
}

bool stageMemoryGuardPasses(size_t sourceBytes) {
#if defined(ARDUINO_ARCH_ESP32)
  const size_t freeBytes = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  const size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
  return freeBytes >= sourceBytes + IDOT_GIF_PSRAM_STAGE_RESERVE && largest >= sourceBytes;
#elif defined(IDOT_GIF_SOURCE_CACHE_HOST_TEST)
  (void)sourceBytes;
  return true;
#else
  (void)sourceBytes;
  return false;
#endif
}

uint8_t* allocateStageBuffer(size_t bytes) {
#if defined(ARDUINO_ARCH_ESP32)
  return static_cast<uint8_t*>(
    heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
  );
#elif defined(IDOT_GIF_SOURCE_CACHE_HOST_TEST)
  return static_cast<uint8_t*>(std::malloc(bytes));
#else
  (void)bytes;
  return nullptr;
#endif
}

#endif

void freeStageBuffer(uint8_t* buffer) {
  if (buffer == nullptr) return;
#if defined(ARDUINO_ARCH_ESP32)
  heap_caps_free(buffer);
#elif defined(IDOT_GIF_SOURCE_CACHE_HOST_TEST)
  std::free(buffer);
#else
  (void)buffer;
#endif
}

}  // namespace

IDotMatrixGifSourceStage::~IDotMatrixGifSourceStage() {
  release();
  clearCache();
}

void IDotMatrixGifSourceStage::touch(CacheEntry& entry) {
  ++useSerial_;
  if (useSerial_ == 0) ++useSerial_;
  entry.lastUse = useSerial_;
}

int IDotMatrixGifSourceStage::findValidEntry(const char* path) const {
  if (path == nullptr || path[0] == '\0' || !cacheEnabled()) return -1;
  for (size_t i = 0; i < CACHE_STORAGE_ENTRIES; ++i) {
    const CacheEntry& entry = cache_[i];
    if (entry.buffer != nullptr && entry.valid && strcmp(entry.path, path) == 0) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

int IDotMatrixGifSourceStage::findFreeEntry() const {
  if (!cacheEnabled()) return -1;
  for (size_t i = 0; i < CACHE_STORAGE_ENTRIES; ++i) {
    if (cache_[i].buffer == nullptr) return static_cast<int>(i);
  }
  return -1;
}

int IDotMatrixGifSourceStage::findLruEntry() const {
  if (!cacheEnabled()) return -1;
  int selected = -1;
  uint32_t oldest = 0;
  for (size_t i = 0; i < CACHE_STORAGE_ENTRIES; ++i) {
    const CacheEntry& entry = cache_[i];
    if (entry.buffer == nullptr || static_cast<int>(i) == activeEntry_) continue;
    if (selected < 0 || entry.lastUse < oldest) {
      selected = static_cast<int>(i);
      oldest = entry.lastUse;
    }
  }
  return selected;
}

void IDotMatrixGifSourceStage::freeEntry(size_t index, bool eviction) {
  if (index >= CACHE_STORAGE_ENTRIES) return;
  CacheEntry& entry = cache_[index];
  if (entry.buffer == nullptr || static_cast<int>(index) == activeEntry_) return;
  if (cacheBytes_ >= entry.bytes) cacheBytes_ -= entry.bytes;
  else cacheBytes_ = 0;
  freeStageBuffer(entry.buffer);
  entry = CacheEntry{};
  if (eviction) ++cacheEvictions_;
}

uint8_t IDotMatrixGifSourceStage::cacheEntries() const {
  uint8_t count = 0;
  if (!cacheEnabled()) return count;
  for (size_t i = 0; i < CACHE_STORAGE_ENTRIES; ++i) {
    if (cache_[i].buffer != nullptr) ++count;
  }
  return count;
}

void IDotMatrixGifSourceStage::release() {
  if (activeBuffer_ == nullptr) {
    activeBytes_ = 0;
    activeEntry_ = -1;
    return;
  }

  if (activeEntry_ >= 0) {
    const size_t index = static_cast<size_t>(activeEntry_);
    activeBuffer_ = nullptr;
    activeBytes_ = 0;
    activeEntry_ = -1;
    // Invalidating a source while AnimatedGIF is still consuming it is safe:
    // the buffer is retained until close()/release(), then retired here.
    if (index < CACHE_STORAGE_ENTRIES && !cache_[index].valid) {
      freeEntry(index, false);
    }
    return;
  }

  freeStageBuffer(activeBuffer_);
  activeBuffer_ = nullptr;
  activeBytes_ = 0;
}

void IDotMatrixGifSourceStage::invalidate(const char* path) {
  if (path == nullptr || path[0] == '\0' || !cacheEnabled()) return;
  for (size_t i = 0; i < CACHE_STORAGE_ENTRIES; ++i) {
    CacheEntry& entry = cache_[i];
    if (entry.buffer == nullptr || !entry.valid || strcmp(entry.path, path) != 0) continue;
    entry.valid = false;
    ++cacheInvalidations_;
    if (static_cast<int>(i) != activeEntry_) freeEntry(i, false);
  }
}

void IDotMatrixGifSourceStage::clearCache() {
  if (!cacheEnabled()) return;
  for (size_t i = 0; i < CACHE_STORAGE_ENTRIES; ++i) {
    CacheEntry& entry = cache_[i];
    if (entry.buffer == nullptr) continue;
    if (entry.valid) ++cacheInvalidations_;
    entry.valid = false;
    if (static_cast<int>(i) != activeEntry_) freeEntry(i, false);
  }
}

bool IDotMatrixGifSourceStage::storeActiveInCache(const char* path, size_t sourceBytes) {
  if (!cacheEnabled() || activeBuffer_ == nullptr || activeEntry_ >= 0 ||
      path == nullptr || path[0] == '\0' || sourceBytes == 0 ||
      sourceBytes > IDOT_GIF_PSRAM_SOURCE_CACHE_ENTRY_MAX ||
      sourceBytes > IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_BYTES) {
    return false;
  }

  uint8_t* staged = activeBuffer_;
  if (!storeBufferInCache(path, staged, sourceBytes)) return false;
  for (size_t i = 0; i < CACHE_STORAGE_ENTRIES; ++i) {
    if (cache_[i].buffer == staged) {
      activeEntry_ = static_cast<int8_t>(i);
      return true;
    }
  }

  // Defensive rollback: storeBufferInCache() must either retain this exact
  // allocation in one cache entry or fail. If metadata ever becomes
  // inconsistent, do not let release() free a buffer still referenced by the
  // cache.
  for (size_t i = 0; i < CACHE_STORAGE_ENTRIES; ++i) {
    if (cache_[i].buffer == staged) freeEntry(i, false);
  }
  return false;
}

bool IDotMatrixGifSourceStage::makeCacheRoom(size_t sourceBytes) {
  if (!cacheEnabled() || sourceBytes == 0 ||
      sourceBytes > IDOT_GIF_PSRAM_SOURCE_CACHE_ENTRY_MAX ||
      sourceBytes > IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_BYTES) {
    return false;
  }

  while (cacheBytes_ + sourceBytes > IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_BYTES) {
    const int victim = findLruEntry();
    if (victim < 0) return false;
    freeEntry(static_cast<size_t>(victim), true);
  }

  int slot = findFreeEntry();
  if (slot < 0) {
    slot = findLruEntry();
    if (slot < 0) return false;
    freeEntry(static_cast<size_t>(slot), true);
  }

  return true;
}

bool IDotMatrixGifSourceStage::storeBufferInCache(
  const char* path,
  uint8_t* buffer,
  size_t sourceBytes
) {
  if (buffer == nullptr || path == nullptr || path[0] == '\0') return false;
  if (!makeCacheRoom(sourceBytes)) return false;

  int slot = findFreeEntry();
  if (slot < 0) return false;

  CacheEntry& entry = cache_[static_cast<size_t>(slot)];
  entry.buffer = buffer;
  entry.bytes = sourceBytes;
  entry.valid = true;
  std::snprintf(entry.path, sizeof(entry.path), "%s", path);
  touch(entry);
  cacheBytes_ += sourceBytes;
  ++cacheStores_;
  return true;
}

bool IDotMatrixGifSourceStage::prefetch(const char* path, size_t expectedBytes) {
#if (defined(ARDUINO_ARCH_ESP32) || defined(IDOT_GIF_SOURCE_CACHE_HOST_TEST)) && IDOT_GIF_PSRAM_STAGE_MAX > 0
  if (!cacheEnabled() || path == nullptr || path[0] == '\0' || !stagePlatformAvailable()) {
    return false;
  }

  int cachedIndex = findValidEntry(path);
  if (cachedIndex >= 0) {
    CacheEntry& entry = cache_[static_cast<size_t>(cachedIndex)];
    if (expectedBytes == 0 || entry.bytes == expectedBytes) {
      touch(entry);
      ++prefetchAlreadyCached_;
      return true;
    }
    invalidate(path);
  }

  ++prefetchAttempts_;
  File source = WLED_FS.open(path, "r");
  if (!source) {
    ++prefetchFailures_;
    return false;
  }

  const size_t sourceBytes = source.size();
  if ((expectedBytes != 0 && sourceBytes != expectedBytes) || sourceBytes < 6 ||
      sourceBytes > IDOT_GIF_PSRAM_STAGE_MAX ||
      sourceBytes > IDOT_GIF_PSRAM_SOURCE_CACHE_ENTRY_MAX ||
      sourceBytes > IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_BYTES) {
    source.close();
    ++prefetchFailures_;
    return false;
  }

  while (!stageMemoryGuardPasses(sourceBytes)) {
    const int victim = findLruEntry();
    if (victim < 0) break;
    freeEntry(static_cast<size_t>(victim), true);
  }
  if (!stageMemoryGuardPasses(sourceBytes) || !makeCacheRoom(sourceBytes)) {
    source.close();
    ++prefetchFailures_;
    return false;
  }

  uint8_t* staged = allocateStageBuffer(sourceBytes);
  while (staged == nullptr) {
    const int victim = findLruEntry();
    if (victim < 0) break;
    freeEntry(static_cast<size_t>(victim), true);
    if (!stageMemoryGuardPasses(sourceBytes) || !makeCacheRoom(sourceBytes)) continue;
    staged = allocateStageBuffer(sourceBytes);
  }
  if (staged == nullptr) {
    source.close();
    ++prefetchFailures_;
    return false;
  }

  size_t offset = 0;
  while (offset < sourceBytes) {
    const size_t remaining = sourceBytes - offset;
    const size_t chunk = remaining < 4096u ? remaining : 4096u;
    const size_t got = source.read(staged + offset, chunk);
    if (got != chunk) {
      source.close();
      freeStageBuffer(staged);
      ++prefetchFailures_;
      return false;
    }
    offset += got;
#if defined(ARDUINO_ARCH_ESP32)
    yield();
#endif
  }
  source.close();

  if (!storeBufferInCache(path, staged, sourceBytes)) {
    freeStageBuffer(staged);
    ++prefetchFailures_;
    return false;
  }

  ++prefetchSuccesses_;
  prefetchBytes_ += sourceBytes;
  return true;
#else
  (void)path;
  (void)expectedBytes;
  return false;
#endif
}

bool IDotMatrixGifSourceStage::stage(const char* path, bool allowPersistentReuse) {
  release();
#if (defined(ARDUINO_ARCH_ESP32) || defined(IDOT_GIF_SOURCE_CACHE_HOST_TEST)) && IDOT_GIF_PSRAM_STAGE_MAX > 0
  if (path == nullptr || path[0] == '\0' || !stagePlatformAvailable()) return false;

  const bool cacheRequested = allowPersistentReuse && cacheEnabled();
  int cachedIndex = cacheRequested ? findValidEntry(path) : -1;
  File source = WLED_FS.open(path, "r");
  if (!source) {
    if (cachedIndex >= 0) invalidate(path);
    if (cacheRequested) ++cacheMisses_;
    ++attempts_;
    ++fallbacks_;
    return false;
  }

  const size_t sourceBytes = source.size();
  if (cachedIndex >= 0) {
    CacheEntry& entry = cache_[static_cast<size_t>(cachedIndex)];
    if (entry.bytes == sourceBytes) {
      source.close();
      touch(entry);
      activeBuffer_ = entry.buffer;
      activeBytes_ = entry.bytes;
      activeEntry_ = static_cast<int8_t>(cachedIndex);
      // peakBytes_ describes the largest source image ever made active for
      // AnimatedGIF, regardless of whether it arrived through a normal stage
      // copy or through the persistent/prefetched PSRAM cache. Keep the
      // diagnostic invariant peakBytes() >= bytes() true on cache hits too.
      if (activeBytes_ > peakBytes_) peakBytes_ = activeBytes_;
      ++cacheHits_;
      return true;
    }
    invalidate(path);
    cachedIndex = -1;
  }
  if (cacheRequested) ++cacheMisses_;
  ++attempts_;

  if (sourceBytes < 6 || sourceBytes > IDOT_GIF_PSRAM_STAGE_MAX) {
    source.close();
    ++fallbacks_;
    return false;
  }

  // Persistent reuse is expendable optimization state. If later firmware
  // activity reduces PSRAM headroom, retire least-recently-used entries before
  // giving up the already-qualified one-play stage path. This prevents the
  // cache itself from turning a would-have-staged GIF into a filesystem fallback.
  while (!stageMemoryGuardPasses(sourceBytes)) {
    const int victim = findLruEntry();
    if (victim < 0) break;
    freeEntry(static_cast<size_t>(victim), true);
  }
  if (!stageMemoryGuardPasses(sourceBytes)) {
    source.close();
    ++fallbacks_;
    return false;
  }

  uint8_t* staged = allocateStageBuffer(sourceBytes);
  while (staged == nullptr) {
    const int victim = findLruEntry();
    if (victim < 0) break;
    freeEntry(static_cast<size_t>(victim), true);
    if (!stageMemoryGuardPasses(sourceBytes)) continue;
    staged = allocateStageBuffer(sourceBytes);
  }
  if (staged == nullptr) {
    source.close();
    ++fallbacks_;
    return false;
  }

  size_t offset = 0;
  while (offset < sourceBytes) {
    const size_t remaining = sourceBytes - offset;
    const size_t chunk = remaining < 4096u ? remaining : 4096u;
    const size_t got = source.read(staged + offset, chunk);
    if (got != chunk) {
      source.close();
      freeStageBuffer(staged);
      ++fallbacks_;
      return false;
    }
    offset += got;
#if defined(ARDUINO_ARCH_ESP32)
    yield();
#endif
  }
  source.close();

  activeBuffer_ = staged;
  activeBytes_ = sourceBytes;
  activeEntry_ = -1;
  if (activeBytes_ > peakBytes_) peakBytes_ = activeBytes_;
  ++successes_;

  if (cacheRequested) storeActiveInCache(path, sourceBytes);
  return true;
#else
  (void)path;
  (void)allowPersistentReuse;
  return false;
#endif
}
