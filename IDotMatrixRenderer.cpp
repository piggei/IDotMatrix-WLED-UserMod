#include "IDotMatrixRenderer.h"

#include <cstring>
#include <new>

static_assert(sizeof(IDotMatrixRenderer::Pixel) == 3, "RGB framebuffer must use three bytes per pixel");

IDotMatrixRenderer::~IDotMatrixRenderer() {
  delete[] pixels_;
  delete[] textBitmaps_;
}

bool IDotMatrixRenderer::begin(uint8_t screenType) {
  const uint8_t dimension = dimensionForScreenType(screenType);
  const size_t requestedPixelCount = size_t(dimension) * dimension;
  textValid_ = false;
  textFrameRendered_ = false;

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

namespace {
using Pixel = IDotMatrixRenderer::Pixel;

constexpr Pixel color(uint8_t red, uint8_t green, uint8_t blue) {
  return Pixel{red, green, blue};
}

void putPixel(Pixel* canvas, int16_t x, int16_t y, const Pixel& value) {
  if (x < 0 || x >= 16 || y < 0 || y >= 16) return;
  canvas[size_t(y) * 16 + x] = value;
}

constexpr uint8_t DIGITS_3X5[10][5] = {
  {7, 5, 5, 5, 7},
  {2, 6, 2, 2, 7},
  {7, 1, 7, 4, 7},
  {7, 1, 7, 1, 7},
  {5, 5, 7, 1, 1},
  {7, 4, 7, 1, 7},
  {7, 4, 7, 5, 7},
  {7, 1, 2, 2, 2},
  {7, 5, 7, 5, 7},
  {7, 5, 7, 1, 7}
};

void drawDigit(Pixel* canvas, uint8_t digit, int16_t x, int16_t y, const Pixel& value) {
  if (digit > 9) return;
  for (uint8_t row = 0; row < 5; ++row) {
    for (uint8_t column = 0; column < 3; ++column) {
      if ((DIGITS_3X5[digit][row] & (1u << (2u - column))) != 0) {
        putPixel(canvas, x + column, y + row, value);
      }
    }
  }
}

void drawSeparator(
  Pixel* canvas,
  int16_t x,
  int16_t y,
  const Pixel& value,
  bool renderDate
) {
  if (renderDate) {
    putPixel(canvas, x + 1, y, value);
    putPixel(canvas, x + 1, y + 1, value);
    putPixel(canvas, x, y + 2, value);
    putPixel(canvas, x, y + 3, value);
  } else {
    putPixel(canvas, x, y + 1, value);
    putPixel(canvas, x, y + 3, value);
  }
}

void drawTwoRows(
  Pixel* canvas,
  uint8_t top,
  uint8_t bottom,
  const Pixel& topColor,
  const Pixel& bottomColor,
  bool renderDate
) {
  drawDigit(canvas, top / 10, 6, 2, topColor);
  drawDigit(canvas, top % 10, 10, 2, topColor);
  drawDigit(canvas, bottom / 10, 6, 9, bottomColor);
  drawDigit(canvas, bottom % 10, 10, 9, bottomColor);
  drawSeparator(canvas, 2, 9, bottomColor, renderDate);
}

Pixel hsv(uint8_t hue) {
  const uint8_t region = hue / 43;
  const uint8_t remainder = (hue - region * 43) * 6;
  const uint8_t down = 255 - remainder;
  switch (region) {
    case 0: return color(255, remainder, 0);
    case 1: return color(down, 255, 0);
    case 2: return color(0, 255, remainder);
    case 3: return color(0, down, 255);
    case 4: return color(remainder, 0, 255);
    default: return color(255, 0, down);
  }
}

Pixel hsvAdjusted(uint8_t hue, uint8_t saturation, uint8_t value) {
  Pixel full = hsv(hue);
  Pixel result;
  result.red = static_cast<uint8_t>(
    (uint16_t(255 - saturation) * value + uint16_t(full.red) * saturation * value / 255u) / 255u
  );
  result.green = static_cast<uint8_t>(
    (uint16_t(255 - saturation) * value + uint16_t(full.green) * saturation * value / 255u) / 255u
  );
  result.blue = static_cast<uint8_t>(
    (uint16_t(255 - saturation) * value + uint16_t(full.blue) * saturation * value / 255u) / 255u
  );
  return result;
}

uint8_t triangleWave(uint8_t phase) {
  return phase < 128 ? uint8_t(phase * 2u) : uint8_t((255u - phase) * 2u);
}

Pixel scaledColor(const Pixel& value, uint8_t scale) {
  return color(
    static_cast<uint8_t>((uint16_t(value.red) * scale + 254u) / 255u),
    static_cast<uint8_t>((uint16_t(value.green) * scale + 254u) / 255u),
    static_cast<uint8_t>((uint16_t(value.blue) * scale + 254u) / 255u)
  );
}

void drawRainbowBorder(Pixel* canvas, uint32_t animationMillis) {
  const uint8_t hue = uint8_t(animationMillis / 20u);
  for (uint8_t x = 0; x < 16; ++x) {
    putPixel(canvas, x, 0, hsv(hue + x * 10));
    putPixel(canvas, 15 - x, 15, hsv(hue + 160 + x * 10));
  }
  for (uint8_t y = 1; y < 15; ++y) {
    putPixel(canvas, 0, y, hsv(hue + 40 + y * 10));
    putPixel(canvas, 15, 15 - y, hsv(hue + 200 + y * 10));
  }
}

void drawChristmasTree(Pixel* canvas) {
  const Pixel green = color(0, 180, 20);
  const Pixel darkGreen = color(0, 100, 0);
  const Pixel yellow = color(255, 210, 0);
  const Pixel red = color(255, 0, 0);
  const Pixel magenta = color(255, 0, 150);
  putPixel(canvas, 2, 7, yellow);
  for (uint8_t x = 1; x <= 3; ++x) putPixel(canvas, x, 8, green);
  for (uint8_t x = 1; x <= 3; ++x) putPixel(canvas, x, 9, green);
  for (uint8_t x = 0; x <= 4; ++x) putPixel(canvas, x, 10, green);
  for (uint8_t x = 0; x <= 4; ++x) putPixel(canvas, x, 11, green);
  for (uint8_t x = 0; x <= 5; ++x) putPixel(canvas, x, 12, green);
  for (uint8_t x = 0; x <= 5; ++x) putPixel(canvas, x, 13, darkGreen);
  putPixel(canvas, 2, 14, color(90, 45, 0));
  putPixel(canvas, 3, 14, color(90, 45, 0));
  putPixel(canvas, 1, 10, red);
  putPixel(canvas, 3, 11, magenta);
  putPixel(canvas, 2, 12, yellow);
  putPixel(canvas, 4, 12, red);
}

void drawRacingBands(Pixel* canvas) {
  const Pixel cyan = color(0, 255, 255);
  const Pixel violet = color(145, 0, 255);
  const Pixel fuchsia = color(255, 0, 170);
  for (uint8_t x = 0; x < 16; ++x) {
    putPixel(canvas, x, 0, cyan);
    putPixel(canvas, x, 1, violet);
    putPixel(canvas, x, 2, fuchsia);
    putPixel(canvas, x, 13, fuchsia);
    putPixel(canvas, x, 14, violet);
    putPixel(canvas, x, 15, cyan);
  }
}

void drawBlueFrame(Pixel* canvas, bool cornerBlocks) {
  const Pixel cyan = color(0, 255, 255);
  const Pixel blue = color(0, 0, 255);
  if (cornerBlocks) {
    for (uint8_t x = 2; x <= 13; ++x) {
      putPixel(canvas, x, 1, cyan);
      putPixel(canvas, x, 14, cyan);
    }
    for (uint8_t y = 2; y <= 13; ++y) {
      putPixel(canvas, 1, y, cyan);
      putPixel(canvas, 14, y, cyan);
    }
    for (uint8_t y = 0; y < 3; ++y) {
      for (uint8_t x = 0; x < 3; ++x) {
        putPixel(canvas, x, y, blue);
        putPixel(canvas, 15 - x, y, blue);
        putPixel(canvas, x, 15 - y, blue);
        putPixel(canvas, 15 - x, 15 - y, blue);
      }
    }
    return;
  }

  for (uint8_t x = 0; x < 16; ++x) {
    putPixel(canvas, x, 0, blue);
    putPixel(canvas, x, 15, blue);
  }
  for (uint8_t y = 0; y < 16; ++y) {
    putPixel(canvas, 0, y, blue);
    putPixel(canvas, 15, y, blue);
  }
  for (uint8_t x = 1; x < 15; ++x) {
    putPixel(canvas, x, 1, cyan);
    putPixel(canvas, x, 14, cyan);
  }
  for (uint8_t y = 1; y < 15; ++y) {
    putPixel(canvas, 1, y, cyan);
    putPixel(canvas, 14, y, cyan);
  }
}

void drawQuadrantBorder(Pixel* canvas) {
  const Pixel red = color(255, 30, 20);
  const Pixel yellow = color(255, 255, 40);
  const Pixel green = color(70, 255, 50);
  const Pixel blue = color(30, 80, 255);
  for (uint8_t x = 0; x < 8; ++x) putPixel(canvas, x, 0, red);
  for (uint8_t x = 8; x < 16; ++x) putPixel(canvas, x, 0, yellow);
  for (uint8_t y = 0; y < 8; ++y) putPixel(canvas, 0, y, red);
  for (uint8_t y = 8; y < 16; ++y) putPixel(canvas, 0, y, blue);
  for (uint8_t y = 0; y < 8; ++y) putPixel(canvas, 15, y, yellow);
  for (uint8_t y = 8; y < 16; ++y) putPixel(canvas, 15, y, green);
  for (uint8_t x = 0; x < 8; ++x) putPixel(canvas, x, 15, blue);
  for (uint8_t x = 8; x < 16; ++x) putPixel(canvas, x, 15, green);
}

void drawHourglass(Pixel* canvas) {
  const Pixel orange = color(255, 155, 0);
  const Pixel sand = color(255, 220, 80);
  const Pixel white = color(255, 255, 255);
  for (uint8_t x = 0; x <= 4; ++x) {
    putPixel(canvas, x, 8, orange);
    putPixel(canvas, x, 14, orange);
  }
  putPixel(canvas, 0, 9, orange);
  putPixel(canvas, 4, 9, orange);
  putPixel(canvas, 1, 10, orange);
  putPixel(canvas, 3, 10, orange);
  putPixel(canvas, 2, 11, white);
  for (uint8_t x = 1; x <= 3; ++x) putPixel(canvas, x, 12, white);
  for (uint8_t x = 0; x <= 4; ++x) putPixel(canvas, x, 13, sand);
}
}

void IDotMatrixRenderer::renderClock(
  uint8_t clockHour,
  uint8_t clockMinute,
  uint8_t clockDay,
  uint8_t clockMonth,
  uint8_t style,
  bool use24Hour,
  bool renderDate,
  uint8_t red,
  uint8_t green,
  uint8_t blue,
  uint32_t animationMillis
) {
  if (pixels_ == nullptr) return;

  Pixel base[16 * 16]{};
  const Pixel selected = color(red, green, blue);
  uint8_t top = renderDate ? clockDay : clockHour;
  const uint8_t bottom = renderDate ? clockMonth : clockMinute;
  if (!renderDate && !use24Hour) {
    if (top == 0) top = 12;
    else if (top > 12) top -= 12;
  }

  switch (style & 0x07) {
    case 0:
      drawRainbowBorder(base, animationMillis);
      drawTwoRows(base, top, bottom, selected, selected, renderDate);
      break;
    case 1: {
      const Pixel clockRed = color(255, 0, 0);
      drawDigit(base, top / 10, 2, 1, clockRed);
      drawDigit(base, top % 10, 6, 1, clockRed);
      drawSeparator(base, 10, 1, clockRed, renderDate);
      drawChristmasTree(base);
      drawDigit(base, bottom / 10, 7, 9, clockRed);
      drawDigit(base, bottom % 10, 11, 9, clockRed);
      break;
    }
    case 2: {
      drawRacingBands(base);
      const Pixel orange = color(255, 170, 0);
      const Pixel white = color(255, 255, 255);
      drawDigit(base, top / 10, 1, 5, orange);
      drawDigit(base, top % 10, 5, 5, orange);
      if (renderDate) {
        putPixel(base, 9, 5, white);
        putPixel(base, 9, 6, white);
        putPixel(base, 8, 7, white);
        putPixel(base, 8, 8, white);
      } else {
        putPixel(base, 8, 6, white);
        putPixel(base, 8, 8, white);
      }
      drawDigit(base, bottom / 10, 10, 5, orange);
      drawDigit(base, bottom % 10, 13, 5, orange);
      break;
    }
    case 3:
      for (Pixel& pixel : base) pixel = selected;
      drawTwoRows(base, top, bottom, color(0, 0, 0), color(0, 0, 0), renderDate);
      break;
    case 4:
      drawDigit(base, top / 10, 2, 1, selected);
      drawDigit(base, top % 10, 6, 1, selected);
      drawSeparator(base, 11, 1, selected, renderDate);
      drawHourglass(base);
      drawDigit(base, bottom / 10, 6, 9, selected);
      drawDigit(base, bottom % 10, 10, 9, selected);
      break;
    case 5:
      drawBlueFrame(base, true);
      drawTwoRows(base, top, bottom, color(255, 165, 0), color(255, 165, 0), renderDate);
      break;
    case 6:
      drawTwoRows(base, top, bottom, selected, selected, renderDate);
      drawBlueFrame(base, false);
      break;
    case 7:
      drawQuadrantBorder(base);
      drawTwoRows(base, top, bottom, selected, selected, renderDate);
      break;
  }

  // The reconstructed clock artwork uses the verified 16x16 reference canvas.
  // Preserve it for larger advertised profiles with nearest-neighbour scaling.
  for (uint16_t y = 0; y < height_; ++y) {
    const uint8_t sourceY = uint16_t(y) * 16u / height_;
    for (uint16_t x = 0; x < width_; ++x) {
      const uint8_t sourceX = uint16_t(x) * 16u / width_;
      pixels_[size_t(y) * width_ + x] = base[size_t(sourceY) * 16 + sourceX];
    }
  }
  visible_ = true;
}

bool IDotMatrixRenderer::beginText(
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
) {
  const bool formatValid =
    (glyphWidth == 8 && glyphHeight == 16 && glyphBytes == 16) ||
    (glyphWidth == 16 && glyphHeight == 32 && glyphBytes == 64);
  const size_t required = size_t(glyphCount) * glyphBytes;
  if (pixels_ == nullptr || width_ == 0 || height_ == 0 ||
      !formatValid || glyphCount == 0 || required > 4096) {
    return false;
  }

  if (required > textBitmapCapacity_) {
    uint8_t* replacement = new (std::nothrow) uint8_t[required];
    if (replacement == nullptr) return false;
    delete[] textBitmaps_;
    textBitmaps_ = replacement;
    textBitmapCapacity_ = required;
  }
  memset(textBitmaps_, 0, required);

  textGlyphCount_ = glyphCount;
  textGlyphWidth_ = glyphWidth;
  textGlyphHeight_ = glyphHeight;
  textGlyphBytes_ = glyphBytes;
  textMotionEffect_ = motionEffect;
  textSpeed_ = speed;
  textColorMode_ = colorMode;
  textColor_ = Pixel{red, green, blue};
  textBackgroundEnabled_ = backgroundEnabled;
  textBackground_ = Pixel{backgroundRed, backgroundGreen, backgroundBlue};
  textAnimationStart_ = now;
  textLastFrame_ = now;
  textFrameRendered_ = false;
  textValid_ = true;

  const int16_t textWidth = int16_t(textGlyphCount_) * textGlyphWidth_;
  const int16_t centeredY = height_ > textGlyphHeight_
    ? int16_t(height_ - textGlyphHeight_) / 2
    : 0;
  switch (textMotionEffect_) {
    case 1:
      textOffsetX_ = width_;
      textOffsetY_ = centeredY;
      break;
    case 2:
      textOffsetX_ = -textWidth;
      textOffsetY_ = centeredY;
      break;
    case 3:
      textOffsetX_ = 0;
      textOffsetY_ = height_;
      break;
    case 4:
      textOffsetX_ = 0;
      textOffsetY_ = -int16_t(textGlyphHeight_);
      break;
    default:
      textOffsetX_ = 0;
      textOffsetY_ = centeredY;
      break;
  }
  return true;
}

bool IDotMatrixRenderer::setTextGlyph(
  uint8_t index,
  const uint8_t* bitmap,
  size_t bitmapLength
) {
  if (!textValid_ || textBitmaps_ == nullptr || index >= textGlyphCount_ ||
      bitmap == nullptr || bitmapLength != textGlyphBytes_) {
    return false;
  }
  memcpy(textBitmaps_ + size_t(index) * textGlyphBytes_, bitmap, textGlyphBytes_);
  return true;
}

void IDotMatrixRenderer::renderText(uint32_t now) {
  if (!textValid_ || textBitmaps_ == nullptr || pixels_ == nullptr) return;

  const uint8_t boundedSpeed = textSpeed_ > 100 ? 100 : textSpeed_;
  uint16_t interval = 140u - uint16_t(boundedSpeed) * 120u / 100u;
  if ((textMotionEffect_ >= 5 || textColorMode_ >= 2) && interval > 45) interval = 45;
  if (textFrameRendered_ && uint32_t(now - textLastFrame_) < interval) return;

  const int16_t textWidth = int16_t(textGlyphCount_) * textGlyphWidth_;
  if (textFrameRendered_) {
    switch (textMotionEffect_) {
      case 1:
        if (--textOffsetX_ < -textWidth) textOffsetX_ = width_;
        break;
      case 2:
        if (++textOffsetX_ > width_) textOffsetX_ = -textWidth;
        break;
      case 3:
        if (--textOffsetY_ < -int16_t(textGlyphHeight_)) textOffsetY_ = height_;
        break;
      case 4:
        if (++textOffsetY_ > height_) textOffsetY_ = -int16_t(textGlyphHeight_);
        break;
      default:
        break;
    }
  }
  textLastFrame_ = now;
  textFrameRendered_ = true;

  const Pixel background = textBackgroundEnabled_ ? textBackground_ : Pixel{0, 0, 0};
  for (size_t index = 0; index < pixelCount_; ++index) pixels_[index] = background;

  const uint32_t elapsed = now - textAnimationStart_;
  const bool blinkHidden = textMotionEffect_ == 5 && ((elapsed / 350u) & 1u) != 0;
  const int16_t laserRow = textMotionEffect_ == 8
    ? int16_t((elapsed / 70u) % height_)
    : -1;
  uint8_t brightnessScale = 255;
  if (textMotionEffect_ == 6) {
    brightnessScale = 40u + uint16_t(triangleWave(uint8_t(elapsed / 8u))) * 215u / 255u;
  } else if (textMotionEffect_ == 8) {
    brightnessScale = 110;
  }

  if (!blinkHidden) {
    const uint8_t bytesPerRow = (textGlyphWidth_ + 7u) / 8u;
    for (uint8_t glyph = 0; glyph < textGlyphCount_; ++glyph) {
      const uint8_t* bitmap = textBitmaps_ + size_t(glyph) * textGlyphBytes_;
      const int16_t glyphX = textOffsetX_ + int16_t(glyph) * textGlyphWidth_;
      for (uint8_t row = 0; row < textGlyphHeight_; ++row) {
        for (uint8_t column = 0; column < textGlyphWidth_; ++column) {
          const uint16_t offset = uint16_t(row) * bytesPerRow + (column >> 3);
          if (offset >= textGlyphBytes_ ||
              (bitmap[offset] & (1u << (column & 7u))) == 0) {
            continue;
          }
          const int16_t x = glyphX + column;
          const int16_t y = textOffsetY_ + row;
          if (x < 0 || y < 0 || x >= width_ || y >= height_) continue;

          Pixel value = textColor_;
          if (textColorMode_ == 2) {
            value = hsvAdjusted(uint8_t(x * 18 + now / 18u), 255, 255);
          } else if (textColorMode_ == 3) {
            value = hsvAdjusted(uint8_t(y * 19 - int32_t(now / 22u)), 175, 255);
          } else if (textColorMode_ == 4) {
            const uint8_t wave = triangleWave(uint8_t(x * 20 + y * 12 + now / 8u));
            value = hsvAdjusted(uint8_t(uint16_t(wave) * 42u / 255u), 220, 255);
          } else if (textColorMode_ > 4) {
            const uint8_t wave = triangleWave(uint8_t(x * 16 - y * 10 + now / 10u));
            value = hsvAdjusted(uint8_t(125u + uint16_t(wave) * 80u / 255u), 220, 255);
          }
          value = scaledColor(value, brightnessScale);
          if (laserRow >= 0 && y == laserRow) value = Pixel{255, 255, 255};
          pixels_[size_t(y) * width_ + x] = value;
        }
      }
    }
  }

  if (textMotionEffect_ == 7) {
    const uint16_t phase = elapsed / 100u;
    for (uint8_t index = 0; index < 8; ++index) {
      const uint8_t x = uint8_t((index * 5u + index * index * 3u) % width_);
      const uint8_t y = uint8_t((phase + index * 3u) % height_);
      pixels_[size_t(y) * width_ + x] = Pixel{255, 255, 255};
    }
  } else if (textMotionEffect_ == 8 && laserRow >= 0) {
    const Pixel laser = scaledColor(Pixel{255, 0, 0}, 120);
    for (uint8_t x = 0; x < width_; ++x) {
      pixels_[size_t(laserRow) * width_ + x] = laser;
    }
  }
  visible_ = true;
}

uint8_t IDotMatrixRenderer::dimensionForScreenType(uint8_t screenType) {
  switch (screenType) {
    case 0x03: return 32;
    case 0x04: return 64;
    case 0x01:
    default: return 16;
  }
}
