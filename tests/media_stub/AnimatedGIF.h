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
    void (*draw)(GIFDRAW*)) { draw_ = draw; return true; }
  void reset() {}
  int playFrame(bool, int* delay) {
    if (delay) *delay = 50;
    if (draw_) {
      uint8_t pixel = 0;
      uint16_t palette[1] = {0xFFFF};
      GIFDRAW d{};
      d.iY = 0; d.y = 0; d.iWidth = 1; d.iX = 0;
      d.pPixels = &pixel; d.pPalette = palette;
      draw_(&d);
    }
    return 0;
  }
  void close() {}
private:
  void (*draw_)(GIFDRAW*) = nullptr;
};
