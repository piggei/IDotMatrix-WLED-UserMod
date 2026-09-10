#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <map>
#include <string>
#include <utility>
#include <vector>

class File {
public:
  File() = default;
  File(std::vector<uint8_t>* data, bool writable, bool failWrite = false)
    : data_(data), writable_(writable), failWrite_(failWrite) {}
  explicit operator bool() const { return data_ != nullptr; }
  size_t write(const uint8_t* src, size_t n) {
    if (!data_ || !writable_ || !src || failWrite_) return 0;
    if (pos_ + n > data_->size()) data_->resize(pos_ + n);
    for (size_t i = 0; i < n; ++i) (*data_)[pos_ + i] = src[i];
    pos_ += n;
    return n;
  }
  size_t read(uint8_t* dst, size_t n) {
    if (!data_ || !dst || pos_ >= data_->size()) return 0;
    if (n > data_->size() - pos_) n = data_->size() - pos_;
    for (size_t i = 0; i < n; ++i) dst[i] = (*data_)[pos_ + i];
    pos_ += n;
    return n;
  }
  size_t size() const { return data_ ? data_->size() : 0; }
  void flush() {}
  void close() { data_ = nullptr; pos_ = 0; writable_ = false; failWrite_ = false; }
private:
  std::vector<uint8_t>* data_ = nullptr;
  size_t pos_ = 0;
  bool writable_ = false;
  bool failWrite_ = false;
};

class TestFS {
public:
  File open(const char* path, const char* mode) {
    if (!path || !mode) return File();
    const std::string key(path);
    if (mode[0] == 'w') {
      if (failOpenWrite_.count(key) && failOpenWrite_[key] > 0) {
        --failOpenWrite_[key];
        return File();
      }
      auto& data = files_[key];
      data.clear();
      const bool failWrite = failWrite_.count(key) && failWrite_[key] > 0;
      if (failWrite) --failWrite_[key];
      return File(&data, true, failWrite);
    }
    auto it = files_.find(key);
    return it == files_.end() ? File() : File(&it->second, false);
  }

  bool remove(const char* path) {
    return path ? files_.erase(std::string(path)) > 0 : false;
  }

  bool exists(const char* path) const {
    return path && files_.find(std::string(path)) != files_.end();
  }

  bool rename(const char* from, const char* to) {
    if (!from || !to) return false;
    const std::pair<std::string, std::string> key{from, to};
    auto fail = failRename_.find(key);
    if (fail != failRename_.end() && fail->second > 0) {
      --fail->second;
      return false;
    }
    auto it = files_.find(std::string(from));
    if (it == files_.end()) return false;
    files_[std::string(to)] = it->second;
    files_.erase(it);
    return true;
  }

  void set(const std::string& path, const std::vector<uint8_t>& data) { files_[path] = data; }
  std::vector<uint8_t> get(const std::string& path) const {
    auto it = files_.find(path);
    return it == files_.end() ? std::vector<uint8_t>{} : it->second;
  }
  void clear() {
    files_.clear();
    failRename_.clear();
    failOpenWrite_.clear();
    failWrite_.clear();
  }
  void failRename(const std::string& from, const std::string& to, int count = 1) {
    failRename_[{from, to}] = count;
  }
  void failOpenWrite(const std::string& path, int count = 1) { failOpenWrite_[path] = count; }
  void failWrite(const std::string& path, int count = 1) { failWrite_[path] = count; }

private:
  std::map<std::string, std::vector<uint8_t>> files_;
  std::map<std::pair<std::string, std::string>, int> failRename_;
  std::map<std::string, int> failOpenWrite_;
  std::map<std::string, int> failWrite_;
};

extern TestFS WLED_FS;
extern time_t localTime;
extern uint32_t testMillis;
extern uint8_t effectCurrent;
extern int testYear;
extern int testMonth;
extern int testDay;
extern int testHour;
extern int testMinute;
extern int testSecond;

constexpr uint8_t FX_MODE_STATIC = 0;
constexpr uint8_t CALL_MODE_DIRECT_CHANGE = 1;

class Segment {
public:
  uint8_t mode = 0;
  void setMode(uint8_t value) { mode = value; }
};
class TestStrip {
public:
  Segment& getFirstSelectedSeg() { return segment_; }
  void trigger() {}
private:
  Segment segment_;
};
extern TestStrip strip;

inline uint32_t millis() { return testMillis; }
inline int year(time_t) { return testYear; }
inline int month(time_t) { return testMonth; }
inline int day(time_t) { return testDay; }
inline int hour(time_t) { return testHour; }
inline int minute(time_t) { return testMinute; }
inline int second(time_t) { return testSecond; }
inline void stateUpdated(uint8_t) {}
