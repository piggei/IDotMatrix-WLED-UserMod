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
  void renderClock(
    uint8_t hour,
    uint8_t minute,
    uint8_t day,
    uint8_t month,
    uint8_t style,
    bool use24Hour,
    bool renderDate,
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    uint32_t animationMillis
  );
  bool beginText(
    uint8_t glyphCount,
    uint8_t glyphWidth,
    uint8_t glyphHeight,
    uint8_t glyphBytes,
    uint8_t motionEffect,
    uint8_t speed,
    uint8_t colorMode,
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    bool backgroundEnabled,
    uint8_t backgroundRed,
    uint8_t backgroundGreen,
    uint8_t backgroundBlue,
    uint32_t now
  );
  bool setTextGlyph(uint8_t index, const uint8_t* bitmap, size_t bitmapLength);
  void renderText(uint32_t now);
  bool beginRawImage(size_t byteLength);
  bool writeRawImage(size_t offset, const uint8_t* data, size_t length);
  bool completeRawImage(bool crcValid);
  void cancelRawImage();
  bool beginAnimation();
  void clearAnimation();
  bool setAnimationPixel(uint8_t x, uint8_t y, uint8_t red, uint8_t green, uint8_t blue);
  bool publishAnimationFrame();
  void endAnimation();

  void setVisible(bool visible) { visible_ = visible; }
  bool isVisible() const { return visible_; }
  bool isReady() const { return pixels_ != nullptr; }
  uint8_t width() const { return width_; }
  uint8_t height() const { return height_; }
  size_t pixelCount() const { return pixelCount_; }
  bool isTextReady() const { return textValid_; }
  uint8_t textGlyphCount() const { return textGlyphCount_; }
  uint8_t textGlyphWidth() const { return textGlyphWidth_; }
  uint8_t textGlyphHeight() const { return textGlyphHeight_; }
  uint8_t textSpeed() const { return textSpeed_; }
  bool isRawImagePending() const { return rawImagePixels_ != nullptr; }
  const Pixel* pixels() const { return pixels_; }
  const Pixel* pixel(uint8_t x, uint8_t y) const;

private:
  static uint8_t dimensionForScreenType(uint8_t screenType);

  Pixel* pixels_ = nullptr;
  size_t pixelCount_ = 0;
  uint8_t width_ = 0;
  uint8_t height_ = 0;
  bool visible_ = false;
  uint8_t* textBitmaps_ = nullptr;
  size_t textBitmapCapacity_ = 0;
  bool textValid_ = false;
  bool textFrameRendered_ = false;
  uint8_t textGlyphCount_ = 0;
  uint8_t textGlyphWidth_ = 0;
  uint8_t textGlyphHeight_ = 0;
  uint8_t textGlyphBytes_ = 0;
  uint8_t textMotionEffect_ = 0;
  uint8_t textSpeed_ = 5;
  uint8_t textColorMode_ = 1;
  Pixel textColor_{255, 255, 255};
  bool textBackgroundEnabled_ = false;
  Pixel textBackground_{0, 0, 0};
  int16_t textOffsetX_ = 0;
  int16_t textOffsetY_ = 0;
  uint32_t textAnimationStart_ = 0;
  uint32_t textLastFrame_ = 0;
  uint32_t textLastMove_ = 0;
  Pixel* rawImagePixels_ = nullptr;
  size_t rawImageBytes_ = 0;
  Pixel* animationPixels_ = nullptr;
};
