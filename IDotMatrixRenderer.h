#pragma once

#include <cstddef>
#include <cstdint>

class IDotMatrixRenderer {
public:
  struct Pixel {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
  };

  IDotMatrixRenderer() = default;
  ~IDotMatrixRenderer();

  IDotMatrixRenderer(const IDotMatrixRenderer&) = delete;
  IDotMatrixRenderer& operator=(const IDotMatrixRenderer&) = delete;

  bool begin(uint8_t screenType);
  void clear();
  bool setPixel(uint8_t x, uint8_t y, uint8_t red, uint8_t green, uint8_t blue);

  void setVisible(bool visible) { visible_ = visible; }
  bool isVisible() const { return visible_; }
  bool isReady() const { return pixels_ != nullptr; }
  uint8_t width() const { return width_; }
  uint8_t height() const { return height_; }
  size_t pixelCount() const { return pixelCount_; }
  uint32_t acceptedPixelUpdates() const { return acceptedPixelUpdates_; }
  const Pixel* pixels() const { return pixels_; }
  const Pixel* pixel(uint8_t x, uint8_t y) const;

private:
  static uint8_t dimensionForScreenType(uint8_t screenType);

  Pixel* pixels_ = nullptr;
  size_t pixelCount_ = 0;
  uint8_t width_ = 0;
  uint8_t height_ = 0;
  bool visible_ = false;
  uint32_t acceptedPixelUpdates_ = 0;
};
