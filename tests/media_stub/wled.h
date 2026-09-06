#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

enum SeekMode { SeekSet };

class File {
public:
  File() = default;
  File(std::vector<uint8_t>* data, bool writable) : data_(data), writable_(writable) {}
  explicit operator bool() const { return data_ != nullptr; }
  size_t write(const uint8_t* src, size_t n) {
    if (!data_ || !writable_ || !src) return 0;
    if (pos_ + n > data_->size()) data_->resize(pos_ + n);
    for (size_t i = 0; i < n; ++i) (*data_)[pos_ + i] = src[i];
    pos_ += n;
    return n;
  }
  size_t read(uint8_t* dst, size_t n) {
    if (!data_ || !dst || pos_ >= data_->size()) return 0;
    const size_t remaining = data_->size() - pos_;
    if (n > remaining) n = remaining;
    for (size_t i = 0; i < n; ++i) dst[i] = (*data_)[pos_ + i];
    pos_ += n;
    return n;
  }
  size_t size() const { return data_ ? data_->size() : 0; }
  size_t position() const { return pos_; }
  bool seek(uint32_t p, SeekMode) {
    if (!data_ || p > data_->size()) return false;
    pos_ = p;
    return true;
  }
  void flush() {}
  void close() { data_ = nullptr; pos_ = 0; writable_ = false; }
private:
  std::vector<uint8_t>* data_ = nullptr;
  size_t pos_ = 0;
  bool writable_ = false;
};

class TestFS {
public:
  File open(const char* path, const char* mode) {
    if (!path || !mode) return File();
    const std::string key(path);
    if (mode[0] == 'w') {
      auto& data = files_[key];
      data.clear();
      return File(&data, true);
    }
    auto it = files_.find(key);
    if (it == files_.end()) return File();
    return File(&it->second, false);
  }
  bool remove(const char* path) { return path ? files_.erase(std::string(path)) > 0 : false; }
  bool exists(const char* path) const { return path && files_.find(std::string(path)) != files_.end(); }
  bool rename(const char* from, const char* to) {
    if (!from || !to) return false;
    auto it = files_.find(std::string(from));
    if (it == files_.end()) return false;
    files_[std::string(to)] = it->second;
    files_.erase(it);
    return true;
  }
private:
  std::map<std::string, std::vector<uint8_t>> files_;
};

extern TestFS WLED_FS;
inline uint32_t millis() { return 0; }
