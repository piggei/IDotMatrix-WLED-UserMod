#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace TestPreferencesStore {
using Namespace = std::map<std::string, std::vector<uint8_t>>;
inline std::map<std::string, Namespace>& all() {
  static std::map<std::string, Namespace> storage;
  return storage;
}
inline std::map<std::string, int>& failPut() {
  static std::map<std::string, int> failures;
  return failures;
}
inline void clear() { all().clear(); failPut().clear(); }
inline void failNextPut(const std::string& qualifiedKey, int count = 1) {
  failPut()[qualifiedKey] = count;
}
inline size_t bytesLength(const std::string& ns, const std::string& key) {
  auto n = all().find(ns);
  if (n == all().end()) return 0;
  auto k = n->second.find(key);
  return k == n->second.end() ? 0 : k->second.size();
}
inline std::vector<uint8_t> bytes(const std::string& ns, const std::string& key) {
  auto n = all().find(ns);
  if (n == all().end()) return {};
  auto k = n->second.find(key);
  return k == n->second.end() ? std::vector<uint8_t>{} : k->second;
}
inline void setRaw(const std::string& ns, const std::string& key, const std::vector<uint8_t>& value) {
  all()[ns][key] = value;
}
}

class Preferences {
public:
  bool begin(const char* name, bool) {
    if (!name) return false;
    namespace_ = name;
    begun_ = true;
    return true;
  }
  void end() { begun_ = false; }
  size_t getBytesLength(const char* key) const {
    if (!begun_ || !key) return 0;
    return TestPreferencesStore::bytesLength(namespace_, key);
  }
  size_t getBytes(const char* key, void* dst, size_t n) const {
    if (!begun_ || !key || !dst) return 0;
    const auto value = TestPreferencesStore::bytes(namespace_, key);
    if (value.size() != n) return 0;
    std::memcpy(dst, value.data(), n);
    return n;
  }
  size_t putBytes(const char* key, const void* src, size_t n) {
    if (!begun_ || !key || (!src && n != 0)) return 0;
    const std::string qualified = namespace_ + "/" + key;
    auto fail = TestPreferencesStore::failPut().find(qualified);
    if (fail != TestPreferencesStore::failPut().end() && fail->second > 0) {
      --fail->second;
      return 0;
    }
    const auto* bytes = static_cast<const uint8_t*>(src);
    TestPreferencesStore::all()[namespace_][key] = std::vector<uint8_t>(bytes, bytes + n);
    return n;
  }
  uint8_t getUChar(const char* key, uint8_t fallback = 0) const {
    if (!begun_ || !key) return fallback;
    const auto value = TestPreferencesStore::bytes(namespace_, key);
    return value.size() == 1 ? value[0] : fallback;
  }
  size_t putUChar(const char* key, uint8_t value) {
    return putBytes(key, &value, 1);
  }
  bool remove(const char* key) {
    if (!begun_ || !key) return false;
    return TestPreferencesStore::all()[namespace_].erase(key) > 0;
  }
private:
  std::string namespace_;
  bool begun_ = false;
};
