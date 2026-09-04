#pragma once
#include <cstdint>
struct GIFFILE { void* fHandle; int32_t iSize; int32_t iPos; };
struct GIFDRAW {
  int iY, y, iWidth, iX;
  uint8_t* pPixels;
  uint16_t* pPalette;
  uint8_t ucDisposalMethod, ucTransparent, ucBackground, ucHasTransparency;
};
#define LITTLE_ENDIAN_PIXELS 0
#define GIF_DRAW_RAW 0
class AnimatedGIF {
public:
  void begin(int) {}
  void setDrawType(int) {}
  bool open(const char*, void* (*)(const char*, int32_t*), void (*)(void*),
    int32_t (*)(GIFFILE*, uint8_t*, int32_t), int32_t (*)(GIFFILE*, int32_t),
    void (*)(GIFDRAW*)) { return true; }
  void reset() {}
  int playFrame(bool, int*) { return 1; }
  void close() {}
};
