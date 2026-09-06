#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <map>
#include <string>
#include <vector>

class File {
public:
  File() = default;
  File(std::vector<uint8_t>* data, bool writable) : data_(data), writable_(writable) {}
  explicit operator bool() const { return data_ != nullptr; }
  size_t write(const uint8_t* src, size_t n) { if (!data_ || !writable_) return 0; if (pos_+n>data_->size()) data_->resize(pos_+n); for(size_t i=0;i<n;i++)(*data_)[pos_+i]=src[i]; pos_+=n; return n; }
  size_t read(uint8_t* dst, size_t n) { if(!data_||!dst||pos_>=data_->size())return 0; if(n>data_->size()-pos_)n=data_->size()-pos_; for(size_t i=0;i<n;i++)dst[i]=(*data_)[pos_+i]; pos_+=n; return n; }
  size_t size() const { return data_?data_->size():0; }
  void flush() {}
  void close() { data_=nullptr; pos_=0; writable_=false; }
private:
  std::vector<uint8_t>* data_=nullptr; size_t pos_=0; bool writable_=false;
};
class TestFS {
public:
  File open(const char* p,const char* m){ if(!p||!m)return File(); std::string k(p); if(m[0]=='w'){auto& d=f_[k]; d.clear(); return File(&d,true);} auto i=f_.find(k); return i==f_.end()?File():File(&i->second,false); }
  bool remove(const char* p){return p?f_.erase(std::string(p))>0:false;}
  bool rename(const char* a,const char* b){if(!a||!b)return false;auto i=f_.find(a);if(i==f_.end())return false;f_[b]=i->second;f_.erase(i);return true;}
private: std::map<std::string,std::vector<uint8_t>> f_;
};
extern TestFS WLED_FS;
extern time_t localTime;
extern uint32_t testMillis;
extern uint8_t effectCurrent;
constexpr uint8_t FX_MODE_STATIC=0;
constexpr uint8_t CALL_MODE_DIRECT_CHANGE=1;
class Segment { public: uint8_t mode=0; void setMode(uint8_t m){mode=m;} };
class TestStrip { public: Segment& getFirstSelectedSeg(){return seg;} void trigger(){} private: Segment seg; };
extern TestStrip strip;
inline uint32_t millis(){return testMillis;}
inline int year(time_t){return 2026;}
inline int month(time_t){return 9;}
inline int day(time_t){return 5;}
inline int hour(time_t){return 12;}
inline int minute(time_t){return 0;}
inline int second(time_t){return 0;}
inline void stateUpdated(uint8_t){}
