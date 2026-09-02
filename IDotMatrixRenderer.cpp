#include "IDotMatrixRenderer.h"

#include <cstring>
#include <new>

static_assert(sizeof(IDotMatrixRenderer::Pixel) == 3, "RGB framebuffer must use three bytes per pixel");

IDotMatrixRenderer::~IDotMatrixRenderer() {
  delete[] pixels_;
}

bool IDotMatrixRenderer::begin(uint8_t screenType) {
  const uint8_t dimension = dimensionForScreenType(screenType);
  const size_t requestedPixelCount = size_t(dimension) * dimension;

  if (pixels_ != nullptr && requestedPixelCount == pixelCount_) {
    clear();
    visible_ = false;
    acceptedPixelUpdates_ = 0;
    return true;
  }

  Pixel* replacement = new (std::nothrow) Pixel[requestedPixelCount];
  if (replacement == nullptr) return false;

  delete[] pixels_;
  pixels_ = replacement;
  pixelCount_ = requestedPixelCount;
  width_ = dimension;
  height_ = dimension;
  visible_ = false;
  acceptedPixelUpdates_ = 0;
  clear();
  return true;
}

void IDotMatrixRenderer::clear() {
  if (pixels_ == nullptr) return;
  memset(pixels_, 0, pixelCount_ * sizeof(Pixel));
}

bool IDotMatrixRenderer::setPixel(
  uint8_t x,
  uint8_t y,
  uint8_t red,
  uint8_t green,
  uint8_t blue
) {
  if (pixels_ == nullptr || x >= width_ || y >= height_) return false;

  Pixel& target = pixels_[size_t(y) * width_ + x];
  target.red = red;
  target.green = green;
  target.blue = blue;
  ++acceptedPixelUpdates_;
  return true;
}

const IDotMatrixRenderer::Pixel* IDotMatrixRenderer::pixel(uint8_t x, uint8_t y) const {
  if (pixels_ == nullptr || x >= width_ || y >= height_) return nullptr;
  return &pixels_[size_t(y) * width_ + x];
}

uint8_t IDotMatrixRenderer::dimensionForScreenType(uint8_t screenType) {
  switch (screenType) {
    case 0x03: return 32;
    case 0x04: return 64;
    case 0x01:
    default: return 16;
  }
}
