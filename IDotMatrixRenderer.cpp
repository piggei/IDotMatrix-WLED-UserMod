#include "IDotMatrixRenderer.h"

#include <cstring>
#include <new>
#include <cstdlib>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#include <esp32-hal-psram.h>
#endif

static_assert(sizeof(IDotMatrixRenderer::Pixel) == 3, "RGB framebuffer must use three bytes per pixel");

namespace {
IDotMatrixRenderer::Pixel* allocatePixelBuffer(size_t count) {
  if (count == 0) return nullptr;
#if defined(ARDUINO_ARCH_ESP32)
  const size_t bytes = count * sizeof(IDotMatrixRenderer::Pixel);
  if (psramFound()) {
    void* external = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (external != nullptr) return static_cast<IDotMatrixRenderer::Pixel*>(external);
  }
  return static_cast<IDotMatrixRenderer::Pixel*>(heap_caps_malloc(bytes, MALLOC_CAP_8BIT));
#else
  return new (std::nothrow) IDotMatrixRenderer::Pixel[count];
#endif
}

void freePixelBuffer(IDotMatrixRenderer::Pixel* pixels) {
#if defined(ARDUINO_ARCH_ESP32)
  heap_caps_free(pixels);
#else
  delete[] pixels;
#endif
}
}

IDotMatrixRenderer::~IDotMatrixRenderer() {
  cancelRawImage();
  freePixelBuffer(pixels_);
  delete[] textBitmaps_;
}

bool IDotMatrixRenderer::begin(uint8_t screenType, uint8_t storageWidth, uint8_t storageHeight) {
  const uint8_t dimension = dimensionForScreenType(screenType);
  logicalWidth_ = dimension;
  logicalHeight_ = dimension;
  if (storageWidth == 0 || storageWidth > dimension) storageWidth = dimension;
  if (storageHeight == 0 || storageHeight > dimension) storageHeight = dimension;
  const size_t requestedPixelCount = size_t(storageWidth) * storageHeight;
  textValid_ = false;
  textFrameRendered_ = false;
  cancelRawImage();
  endAnimation();

  if (pixels_ != nullptr && requestedPixelCount == pixelCount_ &&
      width_ == storageWidth && height_ == storageHeight) {
    clear();
    visible_ = false;
    return true;
  }

  Pixel* replacement = allocatePixelBuffer(requestedPixelCount);
  if (replacement == nullptr) return false;

  freePixelBuffer(pixels_);
  pixels_ = replacement;
  pixelCount_ = requestedPixelCount;
  width_ = storageWidth;
  height_ = storageHeight;
  visible_ = false;
  clear();
  return true;
}

void IDotMatrixRenderer::clear() {
  if (pixels_ == nullptr) return;
  memset(pixels_, 0, pixelCount_ * sizeof(Pixel));
}

void IDotMatrixRenderer::fill(uint8_t red, uint8_t green, uint8_t blue) {
  if (pixels_ == nullptr) return;
  const Pixel value{red, green, blue};
  for (size_t i = 0; i < pixelCount_; ++i) pixels_[i] = value;
}

bool IDotMatrixRenderer::beginAnimation() {
  if (pixels_ == nullptr || pixelCount_ == 0) return false;
  clearAnimation();
  return true;
}

void IDotMatrixRenderer::clearAnimation() {
  clear();
}

bool IDotMatrixRenderer::setAnimationPixel(
  uint8_t x, uint8_t y, uint8_t red, uint8_t green, uint8_t blue
) {
  return setPixel(x, y, red, green, blue);
}

bool IDotMatrixRenderer::setAnimationSourcePixel(
  uint8_t x, uint8_t y, uint8_t red, uint8_t green, uint8_t blue
) {
  if (pixels_ == nullptr || x >= logicalWidth_ || y >= logicalHeight_) return false;
  if (!lowMemoryRescale()) return setAnimationPixel(x, y, red, green, blue);

  const uint8_t targetX = uint8_t(uint16_t(x) * width_ / logicalWidth_);
  const uint8_t targetY = uint8_t(uint16_t(y) * height_ / logicalHeight_);
  if (targetX >= width_ || targetY >= height_) return false;
  // Match the nearest-neighbour sampling previously performed by the WLED
  // adapter: only source pixels selected by the destination grid are stored.
  if (uint16_t(targetX) * logicalWidth_ / width_ != x ||
      uint16_t(targetY) * logicalHeight_ / height_ != y) return true;
  return setAnimationPixel(targetX, targetY, red, green, blue);
}

bool IDotMatrixRenderer::publishAnimationFrame() {
  if (pixels_ == nullptr) return false;
  // playFrame() is synchronous inside the WLED loop, so the display effect
  // cannot observe a half-decoded frame. Reusing the visible canvas removes
  // a second 12,288-byte framebuffer at the 64x64 profile.
  visible_ = true;
  return true;
}

void IDotMatrixRenderer::endAnimation() {
  // The animation canvas is the normal logical framebuffer.
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

void drawClockSeparator(
  Pixel* canvas,
  int16_t x,
  int16_t y,
  const Pixel& value,
  bool renderDate,
  bool separatorVisible
) {
  // DD/MM always keeps its slash visible.  Only the HH:MM colon blinks.
  if (renderDate || separatorVisible) drawSeparator(canvas, x, y, value, renderDate);
}

void drawTwoRows(
  Pixel* canvas,
  uint8_t top,
  uint8_t bottom,
  const Pixel& topColor,
  const Pixel& bottomColor,
  bool renderDate,
  bool separatorVisible,
  int8_t timeSeparatorShift = 0
) {
  drawDigit(canvas, top / 10, 6, 2, topColor);
  drawDigit(canvas, top % 10, 10, 2, topColor);
  drawDigit(canvas, bottom / 10, 6, 9, bottomColor);
  drawDigit(canvas, bottom % 10, 10, 9, bottomColor);
  // Date keeps the previously validated slash position.  The time colon can
  // be micro-positioned independently, matching the emulator artwork.
  const int16_t separatorX = renderDate ? 2 : (2 + timeSeparatorShift);
  drawClockSeparator(canvas, separatorX, 9, bottomColor, renderDate, separatorVisible);
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

bool IDotMatrixRenderer::beginLightEffect(
  uint8_t effect,
  uint8_t speed,
  uint8_t colorCount,
  const Pixel* colors,
  uint32_t now
) {
  if (pixels_ == nullptr) return false;

  lightEffectId_ = effect;
  lightEffectSpeed_ = speed;
  lightEffectColorCount_ = colorCount < MAX_LIGHT_EFFECT_COLORS
    ? colorCount
    : MAX_LIGHT_EFFECT_COLORS;
  for (uint8_t i = 0; i < lightEffectColorCount_; ++i) {
    lightEffectColors_[i] = colors != nullptr ? colors[i] : Pixel{255, 255, 255};
  }
  lightEffectStartMillis_ = now;
  lightEffectLastFrameMillis_ = now;
  lightEffectScrollOffset_ = 0;
  lightEffectFrameRendered_ = false;
  lightEffectValid_ = true;
  visible_ = true;
  renderLightEffect(now);
  return true;
}

void IDotMatrixRenderer::renderLightEffect(uint32_t now) {
  if (!lightEffectValid_ || pixels_ == nullptr || width_ == 0 || height_ == 0) return;

  const uint8_t boundedSpeed = lightEffectSpeed_ > 100 ? 100 : lightEffectSpeed_;
  const uint16_t frameInterval = lightEffectId_ == 6
    ? 40u
    : uint16_t(360u - (uint16_t(boundedSpeed) * 290u) / 100u);
  const bool firstFrame = !lightEffectFrameRendered_;
  if (!firstFrame && uint32_t(now - lightEffectLastFrameMillis_) < frameInterval) return;

  // Scrolling-band effects deliberately advance exactly one physical pixel per
  // accepted render. Speed controls only the interval between renders. This
  // prevents a delayed WLED loop iteration from turning elapsed time into a
  // multi-pixel jump and keeps motion visually uniform.
  if (!firstFrame && lightEffectId_ >= 3 && lightEffectId_ <= 5) {
    ++lightEffectScrollOffset_;
  }

  lightEffectLastFrameMillis_ = now;
  lightEffectFrameRendered_ = true;

  auto effectHash = [](uint32_t value) -> uint32_t {
    value ^= value >> 16;
    value *= 0x7FEB352DUL;
    value ^= value >> 15;
    value *= 0x846CA68BUL;
    value ^= value >> 16;
    return value;
  };

  auto effectColor = [this](uint8_t index) -> Pixel {
    if (lightEffectColorCount_ == 0) return Pixel{255, 255, 255};
    return lightEffectColors_[index % lightEffectColorCount_];
  };

  auto blendPixel = [](const Pixel& a, const Pixel& b, uint8_t fraction) -> Pixel {
    const uint16_t inverse = uint16_t(255u - fraction);
    return Pixel{
      uint8_t((uint16_t(a.red) * inverse + uint16_t(b.red) * fraction + 127u) / 255u),
      uint8_t((uint16_t(a.green) * inverse + uint16_t(b.green) * fraction + 127u) / 255u),
      uint8_t((uint16_t(a.blue) * inverse + uint16_t(b.blue) * fraction + 127u) / 255u)
    };
  };

  auto paletteGradient = [this, &blendPixel](uint8_t position) -> Pixel {
    if (lightEffectColorCount_ == 0) return Pixel{0, 0, 0};
    if (lightEffectColorCount_ == 1) return lightEffectColors_[0];
    const uint16_t scaled = uint16_t(position) * lightEffectColorCount_;
    const uint8_t index = uint8_t(scaled >> 8);
    const uint8_t fraction = uint8_t(scaled & 0xFFu);
    return blendPixel(
      lightEffectColors_[index % lightEffectColorCount_],
      lightEffectColors_[(index + 1u) % lightEffectColorCount_],
      fraction
    );
  };

  auto scaleVideo = [](const Pixel& value, uint8_t scale) -> Pixel {
    auto channel = [scale](uint8_t input) -> uint8_t {
      if (input == 0 || scale == 0) return 0;
      return uint8_t((uint16_t(input) * scale) / 256u + 1u);
    };
    return Pixel{channel(value.red), channel(value.green), channel(value.blue)};
  };

  auto easeCubic = [](uint8_t input) -> uint8_t {
    const uint32_t x2 = (uint32_t(input) * input + 127u) / 255u;
    const uint32_t x3 = (x2 * input + 127u) / 255u;
    const int32_t eased = int32_t(3u * x2) - int32_t(2u * x3);
    return eased < 0 ? 0 : eased > 255 ? 255 : uint8_t(eased);
  };

  const uint16_t multiplier = uint16_t(4u + lightEffectSpeed_ / 3u);
  const uint32_t phase = (uint32_t(now - lightEffectStartMillis_) * multiplier) / 100u;

  switch (lightEffectId_) {
    case 0: {
      for (uint16_t y = 0; y < height_; ++y) {
        for (uint16_t x = 0; x < width_; ++x) {
          const uint8_t position = uint8_t((uint32_t(y) * 5u + x + phase / 4u) / 3u);
          pixels_[size_t(y) * width_ + x] = paletteGradient(position);
        }
      }
      break;
    }

    case 1: {
      clear();
      const uint32_t frame = phase / 8u;
      for (uint8_t i = 0; i < 22; ++i) {
        const uint32_t hash = effectHash(uint32_t(i) * 173u + frame * 31u);
        const uint8_t x = uint8_t(hash % width_);
        const uint8_t y = uint8_t((hash >> 8) % height_);
        Pixel value = effectColor(uint8_t((hash >> 8) %
          (lightEffectColorCount_ == 0 ? 1u : lightEffectColorCount_)));
        value = scaleVideo(value, uint8_t(100u + ((hash >> 16) & 0x9Fu)));
        setPixel(x, y, value.red, value.green, value.blue);
      }
      break;
    }

    case 2: {
      const uint32_t localPhase = phase / 3u;
      for (uint16_t y = 0; y < height_; ++y) {
        for (uint16_t x = 0; x < width_; ++x) {
          const uint8_t position = uint8_t(localPhase + x * 2u + y * 2u);
          pixels_[size_t(y) * width_ + x] = scaleVideo(paletteGradient(position), 190);
        }
      }
      const uint32_t frame = localPhase / 4u;
      for (uint8_t i = 0; i < 18; ++i) {
        const uint32_t hash = effectHash(uint32_t(i) * 223u + frame * 19u);
        setPixel(uint8_t(hash % width_), uint8_t((hash >> 8) % height_), 255, 255, 255);
      }
      break;
    }

    case 3: {
      const uint32_t localPhase = lightEffectScrollOffset_;
      const uint8_t count = lightEffectColorCount_ == 0 ? 1 : lightEffectColorCount_;
      const uint8_t resolutionScale = width_ >= 64 ? 4u : width_ >= 32 ? 2u : 1u;
      const uint8_t stripeWidth = uint8_t(4u * resolutionScale);
      for (uint16_t y = 0; y < height_; ++y) {
        for (uint16_t x = 0; x < width_; ++x) {
          pixels_[size_t(y) * width_ + x] = effectColor(
            uint8_t(((x + localPhase) / stripeWidth) % count)
          );
        }
      }
      break;
    }

    case 4: {
      const uint32_t localPhase = lightEffectScrollOffset_;
      const uint8_t count = lightEffectColorCount_ == 0 ? 1 : lightEffectColorCount_;
      const uint8_t resolutionScale = width_ >= 64 ? 4u : width_ >= 32 ? 2u : 1u;
      const uint8_t stripeWidth = uint8_t(4u * resolutionScale);
      for (uint16_t y = 0; y < height_; ++y) {
        for (uint16_t x = 0; x < width_; ++x) {
          pixels_[size_t(y) * width_ + x] = effectColor(
            uint8_t(((x + y + localPhase) / stripeWidth) % count)
          );
        }
      }
      break;
    }

    case 5: {
      clear();
      const uint32_t localPhase = lightEffectScrollOffset_;
      const uint8_t count = lightEffectColorCount_ == 0 ? 1 : lightEffectColorCount_;
      const uint8_t resolutionScale = width_ >= 64 ? 4u : width_ >= 32 ? 2u : 1u;
      const uint8_t colorWidth = uint8_t(5u * resolutionScale);
      const uint8_t blackWidth = uint8_t(4u * resolutionScale);
      const uint8_t blockWidth = uint8_t(colorWidth + blackWidth);
      for (uint16_t y = 0; y < height_; ++y) {
        for (uint16_t x = 0; x < width_; ++x) {
          const uint32_t distance = x + y + localPhase;
          const uint8_t within = uint8_t(distance % blockWidth);
          if (within < colorWidth) {
            const Pixel value = effectColor(uint8_t((distance / blockWidth) % count));
            setPixel(uint8_t(x), uint8_t(y), value.red, value.green, value.blue);
          }
        }
      }
      break;
    }

    case 6: {
      const uint8_t count = lightEffectColorCount_ == 0 ? 1 : lightEffectColorCount_;
      if (count == 1) {
        const Pixel value = effectColor(0);
        fill(value.red, value.green, value.blue);
        break;
      }

      const uint32_t fadeMillis = 6000u - (uint32_t(boundedSpeed) * 5300u) / 100u;
      const uint32_t elapsed = now - lightEffectStartMillis_;
      const uint32_t cycleMillis = fadeMillis * count;
      for (uint16_t y = 0; y < height_; ++y) {
        for (uint16_t x = 0; x < width_; ++x) {
          const uint32_t index = uint32_t(y) * width_ + x;
          const uint32_t seed = effectHash(index * 977u + 0x51EDu);
          const uint32_t local = (elapsed + seed % cycleMillis) % cycleMillis;
          const uint8_t a = uint8_t(local / fadeMillis);
          const uint8_t b = uint8_t((a + 1u) % count);
          const uint32_t within = local % fadeMillis;
          const uint8_t linear = uint8_t((within * 255u) / (fadeMillis - 1u));
          pixels_[size_t(y) * width_ + x] = blendPixel(
            effectColor(a), effectColor(b), easeCubic(linear)
          );
        }
      }
      break;
    }

    default:
      clear();
      break;
  }

  visible_ = true;
}

namespace {
void scaleLegacyCanvas(
  const Pixel* base,
  Pixel* destination,
  uint8_t width,
  uint8_t height
) {
  for (uint16_t y = 0; y < height; ++y) {
    const uint8_t sourceY = uint16_t(y) * 16u / height;
    for (uint16_t x = 0; x < width; ++x) {
      const uint8_t sourceX = uint16_t(x) * 16u / width;
      destination[size_t(y) * width + x] = base[size_t(sourceY) * 16u + sourceX];
    }
  }
}

Pixel audioRainbow(uint8_t hue) {
  const uint8_t region = hue / 43u;
  const uint8_t remainder = uint8_t((hue - region * 43u) * 6u);
  const uint8_t q = uint8_t(255u - remainder);
  const uint8_t t = remainder;
  switch (region) {
    case 0: return color(255, t, 0);
    case 1: return color(q, 255, 0);
    case 2: return color(0, 255, t);
    case 3: return color(0, q, 255);
    case 4: return color(t, 0, 255);
    default: return color(255, 0, q);
  }
}

void audioHeart(Pixel* base, int16_t ox, int16_t oy, bool pulse,
                const Pixel& outline, const Pixel& fillValue) {
  static const uint16_t rows[8] = {
    0x06Cu, 0x0FEu, 0x1FFu, 0x1FFu, 0x0FEu, 0x07Cu, 0x038u, 0x010u
  };
  for (uint8_t y = 0; y < 8; ++y) for (uint8_t x = 0; x < 9; ++x) {
    if ((rows[y] & (1u << (8u - x))) == 0) continue;
    bool edge = x == 0 || x == 8 || y == 0 || y == 7;
    if (!edge) {
      const bool left = rows[y] & (1u << (9u - x));
      const bool right = rows[y] & (1u << (7u - x));
      const bool up = rows[y - 1] & (1u << (8u - x));
      const bool down = rows[y + 1] & (1u << (8u - x));
      edge = !(left && right && up && down);
    }
    Pixel value = edge ? outline : fillValue;
    if (pulse && !edge && x > 1 && x < 7 && y > 1 && y < 6) {
      value.red = uint8_t(value.red > 215 ? 255 : value.red + 40);
      value.green = uint8_t(value.green > 215 ? 255 : value.green + 40);
      value.blue = uint8_t(value.blue > 215 ? 255 : value.blue + 40);
    }
    putPixel(base, ox + x, oy + y, value);
  }
}

uint8_t audioBand(uint8_t value) { return value > 12 ? 12 : value; }
}

void IDotMatrixRenderer::renderAudio(
  bool fft, uint8_t mode, uint8_t level, const uint8_t bands[8], uint32_t now,
  uint32_t packetCounter, uint32_t lastPacketMillis
) {
  if (pixels_ == nullptr) return;
  Pixel base[16 * 16]{};
  const Pixel white = color(255, 255, 255);
  level = audioBand(level);

  if (!fft) switch (mode > 4 ? 4 : mode) {
    case 0: {
      // B154 manufacturer-style breakdancer. The captured sprites are an atlas
      // of independent body-part alternatives rather than a fixed frame loop.
      auto dancerRand = [&]() -> uint32_t {
        uint32_t x = audioDancerRng_;
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        audioDancerRng_ = x ? x : 0x6D2B79F5u;
        return audioDancerRng_;
      };
      auto drawPoints = [&](const int8_t (*pts)[2], uint8_t count, const Pixel& value) {
        for (uint8_t i = 0; i < count; ++i) putPixel(base, pts[i][0], pts[i][1], value);
      };

      if (!audioDancerInitialized_) {
        audioDancerInitialized_ = true;
        audioDancerRng_ = 0xA341316Cu ^ now ^ (packetCounter * 0x9E3779B9u);
        audioDancerNextChangeMs_ = 0;
      }

      const bool audioFresh = uint32_t(now - lastPacketMillis) <= 350u;
      const bool audioActive = audioFresh && level > 1u;
      const bool newAudioPacket = packetCounter != audioDancerLastProcessedPacket_;
      if (audioActive && newAudioPacket && int32_t(now - audioDancerNextChangeMs_) >= 0) {
        audioDancerLastProcessedPacket_ = packetCounter;
        int chanceValue = 68 + int(level) * 5;
        if (chanceValue > 99) chanceValue = 99;
        const uint8_t chance = uint8_t(chanceValue);
        if ((dancerRand() % 100u) < chance) audioDancerHead_ = uint8_t(dancerRand() % 3u);
        if ((dancerRand() % 100u) < chance) audioDancerLeftArm_ = uint8_t(dancerRand() % 2u);
        if ((dancerRand() % 100u) < chance) audioDancerRightArm_ = uint8_t(dancerRand() % 2u);
        if ((dancerRand() % 100u) < chance) audioDancerLeftLeg_ = uint8_t(dancerRand() % 2u);
        if ((dancerRand() % 100u) < chance) audioDancerRightLeg_ = uint8_t(dancerRand() % 2u);
        if ((dancerRand() % 100u) < chance) audioDancerBackground_ = uint8_t(dancerRand() % 3u);
        int wait = 145 - int(level) * 8;
        if (wait < 45) wait = 45;
        if (wait > 145) wait = 145;
        audioDancerNextChangeMs_ = now + uint32_t(wait);
      } else if (newAudioPacket) {
        // Consume silent/too-fast packets without changing pose, matching B154.
        audioDancerLastProcessedPacket_ = packetCounter;
      }

      static const Pixel backgrounds[3] = {
        {90,215,70}, {247,255,100}, {46,228,222}
      };
      const Pixel bg = backgrounds[audioDancerBackground_ % 3u];
      for (int y = 0; y < 3; ++y) for (int x = 0; x < 16; ++x)
        if (((x + y) & 1) == 1) putPixel(base, x, y, bg);

      static const int8_t torso[][2] = {
        {7,5},{8,5},{7,6},{8,6},{7,7},{8,7},{7,8},{8,8},{7,9},{8,9},{7,10},{8,10}
      };
      static const int8_t head0[][2]={{6,1},{7,1},{8,1},{9,1},{6,2},{7,2},{8,2},{9,2},{6,3},{7,3},{8,3},{9,3},{10,3},{6,4},{7,4},{8,4},{9,4}};
      static const int8_t head1[][2]={{7,1},{8,1},{9,1},{10,1},{7,2},{8,2},{9,2},{10,2},{7,3},{8,3},{9,3},{10,3},{7,4},{8,4},{9,4},{10,4}};
      static const int8_t head2[][2]={{5,1},{6,1},{7,1},{8,1},{5,2},{6,2},{7,2},{8,2},{5,3},{6,3},{7,3},{8,3},{5,4},{6,4},{7,4},{8,4}};
      static const int8_t leftArm0[][2]={{6,6},{5,7},{4,8},{3,9}};
      static const int8_t leftArm1[][2]={{6,7},{5,7},{4,7},{3,7},{2,8}};
      static const int8_t rightArm0[][2]={{9,6},{10,7},{11,8},{12,9}};
      static const int8_t rightArm1[][2]={{9,7},{10,7},{11,7},{12,7},{13,8}};
      static const int8_t leftLeg0[][2]={{7,11},{6,12},{5,12},{5,13},{5,14},{6,12}};
      static const int8_t leftLeg1[][2]={{6,11},{7,11},{6,12},{6,13},{6,14},{5,15},{6,15}};
      static const int8_t rightLeg0[][2]={{9,11},{9,12},{9,13},{9,14},{9,15},{10,15}};
      static const int8_t rightLeg1[][2]={{8,11},{9,11},{9,12},{9,13},{9,14},{9,15},{10,15}};

      drawPoints(torso, uint8_t(sizeof(torso)/sizeof(torso[0])), white);
      if (audioDancerHead_ == 1) drawPoints(head1, uint8_t(sizeof(head1)/sizeof(head1[0])), white);
      else if (audioDancerHead_ == 2) drawPoints(head2, uint8_t(sizeof(head2)/sizeof(head2[0])), white);
      else drawPoints(head0, uint8_t(sizeof(head0)/sizeof(head0[0])), white);
      drawPoints(audioDancerLeftArm_ ? leftArm1 : leftArm0, audioDancerLeftArm_ ? 5 : 4, white);
      drawPoints(audioDancerRightArm_ ? rightArm1 : rightArm0, audioDancerRightArm_ ? 5 : 4, white);
      drawPoints(audioDancerLeftLeg_ ? leftLeg1 : leftLeg0, audioDancerLeftLeg_ ? 7 : 6, white);
      drawPoints(audioDancerRightLeg_ ? rightLeg1 : rightLeg0, audioDancerRightLeg_ ? 7 : 6, white);
      break;
    }
    case 1: {
      const uint16_t hotValue = 80u + uint16_t(level) * 24u;
      audioHeart(base,3,4,level>=5,white,color(uint8_t(hotValue>255?255:hotValue),0,20));
      if (level>=6) { putPixel(base,1,7,color(255,0,30)); putPixel(base,14,7,color(255,0,30)); }
      break;
    }
    case 2: {
      const Pixel frame=color(0,235,255);
      // LEVEL 3 perimeter: one cyan pixel followed by two empty positions.
      // Walk the perimeter clockwise and advance the phase positively; using
      // (perimeterIndex + phase) makes the visible dots move one position in
      // the opposite (counter-clockwise) direction every 95 ms.
      const uint32_t tick=now/95u;
      const uint8_t framePhase=uint8_t(tick%3u);
      uint8_t perimeterIndex=0;
      auto framePixel = [&](int x, int y) {
        if (((perimeterIndex + framePhase) % 3u) == 0u) putPixel(base,x,y,frame);
        ++perimeterIndex;
      };
      for(int x=0;x<16;x++) framePixel(x,0);
      for(int y=1;y<16;y++) framePixel(15,y);
      for(int x=14;x>=0;x--) framePixel(x,15);
      for(int y=14;y>0;y--) framePixel(0,y);
      const uint8_t strength = level < 1 ? 1 : level > 7 ? 7 : level;
      for(int x=2;x<=13;x++) {
        const int boost=6-std::abs(x-7); const uint8_t wobble=uint8_t((x*7+tick*3+(x&1)*5)%5);
        int height=int(strength)+boost/2+int(wobble)-1; if(height<2)height=2;if(height>13)height=13;
        for(int n=0;n<height;n++) putPixel(base,x,14-n,audioRainbow(uint8_t(185+x*10+n*7+tick*2)));
      }
      break;
    }
    case 3: {
      const Pixel purple=color(125,20,225), purple2=color(82,8,165), skin=color(248,235,210);
      for(int y=1;y<=13;y++) for(int x=1;x<=14;x++) {
        if((y==1&&(x<4||x>11))||(y==2&&(x<2||x>13))||((x==1||x==14)&&(y<4||y>11))) continue;
        putPixel(base,x,y,((x+y)&1)?purple:purple2);
      }
      for(int y=5;y<=8;y++){for(int x=3;x<=6;x++)putPixel(base,x,y,skin);for(int x=9;x<=12;x++)putPixel(base,x,y,skin);}
      const int shift=int((now/180u+level)%3u)-1; putPixel(base,5+shift,7,color(30,25,65));putPixel(base,10+shift,7,color(30,25,65));
      const int opening=level<1?1:level>7?7:level, half=2+opening/2;
      for(int dx=-half;dx<=half;dx++){const int taper=std::abs(dx);putPixel(base,8+dx,10+(taper>half-2),color(255,115,125));putPixel(base,8+dx,12-(taper>half-2),color(255,35,55));}
      for(int x=9-half;x<=7+half;x++)putPixel(base,x,11,color(35,0,25));
      break;
    }
    default: {
      // B154 observed face: eyes and blue-mouth states evolve independently.
      static const uint16_t eyeRed[4][16] = {
        {0,0,0,0,0x381C,0x4422,0x381C,0,0,0,0,0,0,0,0,0},
        {0,0x381C,0x4422,0x4422,0x4422,0x4422,0x381C,0,0,0,0,0,0,0,0,0},
        {0,0x381C,0x4422,0x4422,0x4422,0x4422,0x381C,0,0,0,0,0,0,0,0,0},
        {0,0x381C,0x4422,0x4422,0x4422,0x4422,0x381C,0,0,0,0,0,0,0,0,0}
      };
      static const uint16_t eyeWhite[4][16] = {
        {0,0,0,0,0,0x381C,0,0,0,0,0,0,0,0,0,0},
        {0,0,0x381C,0x2814,0x2814,0x381C,0,0,0,0,0,0,0,0,0},
        {0,0,0x381C,0x381C,0x381C,0x2010,0,0,0,0,0,0,0,0,0},
        {0,0,0x381C,0x3018,0x3018,0x381C,0,0,0,0,0,0,0,0,0}
      };
      static const uint16_t mouthBlue[4][16] = {
        {0,0,0,0,0,0,0,0,0,0x3FFC,0x7FFE,0,0x7FFE,0x3FFC,0,0},
        {0,0,0,0,0,0,0,0,0x3FFC,0x7FFE,0,0,0,0x7FFE,0x3FFC,0},
        {0,0,0,0,0,0,0,0,0,0,0x3FFC,0x7FFE,0x7FFE,0x3FFC,0,0},
        {0,0,0,0,0,0,0,0,0x3FFC,0x7FFE,0,0,0,0x7FFE,0x3FFC,0}
      };
      static const uint16_t mouthRed[4][16] = {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0x0180,0x0240,0,0,0,0}
      };
      auto faceRand = [&]() -> uint32_t {
        uint32_t x = audioFaceRng_;
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        audioFaceRng_ = x ? x : 0xA341316Cu;
        return audioFaceRng_;
      };
      auto differentState = [&](uint8_t current) -> uint8_t {
        uint8_t next = uint8_t(faceRand() & 0x03u);
        if (next == current) next = uint8_t((next + 1u + (faceRand() % 3u)) & 0x03u);
        return next;
      };
      auto drawMask = [&](const uint16_t rows[16], const Pixel& value) {
        for (uint8_t y = 0; y < 16; ++y) {
          const uint16_t bits = rows[y];
          for (uint8_t x = 0; x < 16; ++x) if (bits & (1u << x)) putPixel(base, x, y, value);
        }
      };

      if (!audioFaceInitialized_) {
        audioFaceInitialized_ = true;
        audioFaceEyes_ = 1;
        audioFaceMouth_ = 0;
        audioFaceNextEyesMs_ = now + 320u;
        audioFaceNextMouthMs_ = now + 420u;
      }
      if (int32_t(now - audioFaceNextEyesMs_) >= 0) {
        audioFaceEyes_ = differentState(audioFaceEyes_);
        int wait = 420 - int(level) * 15;
        if (wait < 180) wait = 180;
        audioFaceNextEyesMs_ = now + uint32_t(wait) + (faceRand() % 160u);
      }
      if (int32_t(now - audioFaceNextMouthMs_) >= 0) {
        audioFaceMouth_ = differentState(audioFaceMouth_);
        int wait = 500 - int(level) * 16;
        if (wait < 220) wait = 220;
        audioFaceNextMouthMs_ = now + uint32_t(wait) + (faceRand() % 200u);
      }

      drawMask(eyeRed[audioFaceEyes_], color(232,0,11));
      drawMask(eyeWhite[audioFaceEyes_], color(255,255,252));
      drawMask(mouthBlue[audioFaceMouth_], color(8,69,247));
      drawMask(mouthRed[audioFaceMouth_], color(232,0,11));
      break;
    }
  } else switch (mode > 4 ? 4 : mode) {
    case 0: case 1: {
      static const Pixel rows[8]={{255,20,20},{255,185,0},{235,255,0},{30,255,40},{0,245,220},{0,120,255},{120,40,255},{255,30,210}};
      for(uint8_t x=0;x<16;x++){const uint8_t v=audioBand(bands[x<8?x:15-x]);uint8_t half=uint8_t((v+1)/2);if(mode==1&&half<1)half=1;
        for(uint8_t n=0;n<half&&n<8;n++){const Pixel c=mode==0?audioRainbow(uint8_t(x*15+n*8)):rows[n];putPixel(base,x,7-n,c);putPixel(base,x,8+n,c);}}
      break;
    }
    case 2: {
      uint16_t sum=0;for(uint8_t i=0;i<8;i++)sum+=audioBand(bands[i]);const uint8_t avg=uint8_t(sum/8);
      static const int8_t left[13]={3,1,0,0,0,1,1,2,3,4,5,6,7};
      static const int8_t right[13]={6,7,7,7,7,7,6,6,5,4,3,2,1};
      const uint32_t phase=now/45u;
      for(uint8_t y=0;y<13;y++){int squeeze=audioBand(bands[y/2>7?7:y/2])>=8?2:audioBand(bands[y/2>7?7:y/2])>=4?1:0;if(avg<=2)squeeze=0;
        if(y<=4){const int hw=3-squeeze/2;for(int x=4-hw;x<=4+hw;x++)putPixel(base,x,y+1,audioRainbow(uint8_t(x*13+y*12+phase)));for(int x=11-hw;x<=11+hw;x++)putPixel(base,x,y+1,audioRainbow(uint8_t(x*13+y*12+phase)));if(y>=2)for(int x=5+squeeze;x<=10-squeeze;x++)putPixel(base,x,y+1,audioRainbow(uint8_t(x*13+y*12+phase)));}
        else {int half=(left[y]>right[y]?left[y]:right[y])-squeeze;if(half<1)half=1;for(int x=7-half;x<=8+half;x++)putPixel(base,x,y+1,audioRainbow(uint8_t(x*13+y*12+phase)));}}
      break;
    }
    case 3: {
      for(int y=0;y<16;y++){putPixel(base,7,y,color(40,140,255));putPixel(base,8,y,color(40,140,255));const uint8_t v=audioBand(bands[y<8?y:15-y]);const uint8_t width=uint8_t((v+1)/2>7?7:(v+1)/2);for(uint8_t n=1;n<=width;n++){const Pixel c=audioRainbow(uint8_t(y*14+n*9));putPixel(base,7-n,y,c);putPixel(base,8+n,y,c);}}
      break;
    }
    default: {
      for(uint8_t x=0;x<16;x++){const uint8_t v=audioBand(bands[x<8?x:15-x]);const uint8_t height=uint8_t((v+1)/2>7?7:(v+1)/2);for(uint8_t n=0;n<height;n++){const Pixel c=audioRainbow(uint8_t(150+x*11+n*7));putPixel(base,x,n,c);putPixel(base,x,15-n,c);}}
      break;
    }
  }
  scaleLegacyCanvas(base, pixels_, width_, height_);
  visible_ = true;
}

void IDotMatrixRenderer::renderMMSS(
  uint32_t seconds,
  uint8_t red,
  uint8_t green,
  uint8_t blue
) {
  if (pixels_ == nullptr) return;

  Pixel base[16 * 16]{};
  const Pixel value = color(red, green, blue);
  const uint8_t minutes = uint8_t((seconds / 60u) % 100u);
  const uint8_t secs = uint8_t(seconds % 60u);
  drawDigit(base, minutes / 10u, 0, 5, value);
  drawDigit(base, minutes % 10u, 3, 5, value);
  drawSeparator(base, 7, 5, value, false);
  drawDigit(base, secs / 10u, 9, 5, value);
  drawDigit(base, secs % 10u, 12, 5, value);

  scaleLegacyCanvas(base, pixels_, width_, height_);
  visible_ = true;
}

constexpr uint8_t COUNTDOWN_HOURGLASS_FRAMES[10][70] = {
  {3,3,3,3,3,3,3, 1,2,2,2,2,2,1, 1,2,2,2,2,2,1, 0,1,2,2,2,1,0, 0,0,1,2,1,0,0, 0,0,1,0,1,0,0, 0,1,0,0,0,1,0, 1,0,0,0,0,0,1, 1,0,0,0,0,0,1, 3,3,3,3,3,3,3},
  {3,3,3,3,3,3,3, 1,2,2,2,2,2,1, 1,2,2,2,2,2,1, 0,1,2,2,2,1,0, 0,0,1,2,1,0,0, 0,0,1,2,1,0,0, 0,1,0,0,0,1,0, 1,0,0,0,0,0,1, 1,0,0,0,0,0,1, 3,3,3,3,3,3,3},
  {3,3,3,3,3,3,3, 1,2,2,2,2,2,1, 1,2,2,2,2,2,1, 0,1,2,2,2,1,0, 0,0,1,2,1,0,0, 0,0,1,2,1,0,0, 0,1,0,2,0,1,0, 1,0,0,0,0,0,1, 1,0,0,0,0,0,1, 3,3,3,3,3,3,3},
  {3,3,3,3,3,3,3, 1,2,2,2,2,2,1, 1,2,2,2,2,2,1, 0,1,2,2,2,1,0, 0,0,1,2,1,0,0, 0,0,1,2,1,0,0, 0,1,0,2,0,1,0, 1,0,0,2,0,0,1, 1,0,0,2,0,0,1, 3,3,3,3,3,3,3},
  {3,3,3,3,3,3,3, 1,2,2,2,2,2,1, 1,2,2,2,2,2,1, 0,1,2,2,2,1,0, 0,0,1,2,1,0,0, 0,0,1,2,1,0,0, 0,1,0,2,0,1,0, 1,0,0,2,0,0,1, 1,0,2,2,2,0,1, 3,3,3,3,3,3,3},
  {3,3,3,3,3,3,3, 1,0,0,0,0,0,1, 1,2,2,2,2,2,1, 0,1,2,2,2,1,0, 0,0,1,2,1,0,0, 0,0,1,2,1,0,0, 0,1,0,2,0,1,0, 1,0,0,2,0,0,1, 1,2,2,2,2,2,1, 3,3,3,3,3,3,3},
  {3,3,3,3,3,3,3, 1,0,0,0,0,0,1, 1,2,2,2,2,2,1, 0,1,2,2,2,1,0, 0,0,1,2,1,0,0, 0,0,1,2,1,0,0, 0,1,0,2,0,1,0, 1,0,2,2,2,0,1, 1,2,2,2,2,2,1, 3,3,3,3,3,3,3},
  {3,3,3,3,3,3,3, 1,0,0,0,0,0,1, 1,0,0,0,0,0,1, 0,1,2,2,2,1,0, 0,0,1,2,1,0,0, 0,0,1,2,1,0,0, 0,1,0,2,0,1,0, 1,2,2,2,2,2,1, 1,2,2,2,2,2,1, 3,3,3,3,3,3,3},
  {3,3,3,3,3,3,3, 1,0,0,0,0,0,1, 1,0,0,0,0,0,1, 0,1,0,0,0,1,0, 0,0,1,2,1,0,0, 0,0,1,2,1,0,0, 0,1,2,2,2,1,0, 1,2,2,2,2,2,1, 1,2,2,2,2,2,1, 3,3,3,3,3,3,3},
  {3,3,3,3,3,3,3, 1,0,0,0,0,0,1, 1,0,0,0,0,0,1, 0,1,0,0,0,1,0, 0,0,1,0,1,0,0, 0,0,1,2,1,0,0, 0,1,2,2,2,1,0, 1,2,2,2,2,2,1, 1,2,2,2,2,2,1, 3,3,3,3,3,3,3}
};

constexpr uint8_t STOPWATCH_FRAMES[8][63] = {
  {0,2,2,2,2,2,0, 0,0,2,2,2,0,0, 0,3,3,3,3,3,0, 3,1,1,1,1,1,3, 3,1,1,4,1,1,3, 3,1,1,4,1,1,3, 3,1,1,1,1,1,3, 3,1,1,1,1,1,3, 0,3,3,3,3,3,0},
  {0,2,2,2,2,2,0, 0,0,2,2,2,0,0, 0,3,3,3,3,3,0, 3,1,1,1,1,1,3, 3,1,1,1,4,1,3, 3,1,1,4,1,1,3, 3,1,1,1,1,1,3, 3,1,1,1,1,1,3, 0,3,3,3,3,3,0},
  {0,2,2,2,2,2,0, 0,0,2,2,2,0,0, 0,3,3,3,3,3,0, 3,1,1,1,1,1,3, 3,1,1,1,1,1,3, 3,1,1,4,4,1,3, 3,1,1,1,1,1,3, 3,1,1,1,1,1,3, 0,3,3,3,3,3,0},
  {0,2,2,2,2,2,0, 0,0,2,2,2,0,0, 0,3,3,3,3,3,0, 3,1,1,1,1,1,3, 3,1,1,1,1,1,3, 3,1,1,4,1,1,3, 3,1,1,1,4,1,3, 3,1,1,1,1,1,3, 0,3,3,3,3,3,0},
  {0,2,2,2,2,2,0, 0,0,2,2,2,0,0, 0,3,3,3,3,3,0, 3,1,1,1,1,1,3, 3,1,1,1,1,1,3, 3,1,1,4,1,1,3, 3,1,1,4,1,1,3, 3,1,1,1,1,1,3, 0,3,3,3,3,3,0},
  {0,2,2,2,2,2,0, 0,0,2,2,2,0,0, 0,3,3,3,3,3,0, 3,1,1,1,1,1,3, 3,1,1,1,1,1,3, 3,1,1,4,1,1,3, 3,1,4,1,1,1,3, 3,1,1,1,1,1,3, 0,3,3,3,3,3,0},
  {0,2,2,2,2,2,0, 0,0,2,2,2,0,0, 0,3,3,3,3,3,0, 3,1,1,1,1,1,3, 3,1,1,1,1,1,3, 3,1,4,4,1,1,3, 3,1,1,1,1,1,3, 3,1,1,1,1,1,3, 0,3,3,3,3,3,0},
  {0,2,2,2,2,2,0, 0,0,2,2,2,0,0, 0,3,3,3,3,3,0, 3,1,1,1,1,1,3, 3,1,4,1,1,1,3, 3,1,1,4,1,1,3, 3,1,1,1,1,1,3, 3,1,1,1,1,1,3, 0,3,3,3,3,3,0}
};

constexpr uint8_t SCORE_DIGITS_4X7[10][7] = {
  {0xF,0x9,0x9,0x9,0x9,0x9,0xF},
  {0x2,0x6,0x2,0x2,0x2,0x2,0x7},
  {0xF,0x1,0x1,0xF,0x8,0x8,0xF},
  {0xF,0x1,0x1,0xF,0x1,0x1,0xF},
  {0x9,0x9,0x9,0xF,0x1,0x1,0x1},
  {0xF,0x8,0x8,0xF,0x1,0x1,0xF},
  {0xF,0x8,0x8,0xF,0x9,0x9,0xF},
  {0xF,0x1,0x1,0x2,0x2,0x4,0x4},
  {0xF,0x9,0x9,0xF,0x9,0x9,0xF},
  {0xF,0x9,0x9,0xF,0x1,0x1,0xF}
};

void drawCountdownHourglass(Pixel* canvas, uint8_t frame) {
  constexpr Pixel palette[4] = {
    color(0,0,0), color(255,255,255), color(242,119,6), color(146,86,61)
  };
  frame %= 10u;
  for (uint8_t y=0; y<10; ++y) {
    for (uint8_t x=0; x<7; ++x) {
      const uint8_t index = COUNTDOWN_HOURGLASS_FRAMES[frame][size_t(y)*7u+x];
      if (index != 0) putPixel(canvas, x, y+4, palette[index]);
    }
  }
}

void drawStopwatchArtwork(Pixel* canvas, uint8_t frame) {
  constexpr Pixel palette[5] = {
    color(0,0,0), color(255,255,255), color(242,119,6), color(164,158,165), color(220,5,39)
  };
  frame &= 7u;
  for (uint8_t y=0; y<9; ++y) {
    for (uint8_t x=0; x<7; ++x) {
      const uint8_t index = STOPWATCH_FRAMES[frame][size_t(y)*7u+x];
      if (index != 0) putPixel(canvas, x, y+4, palette[index]);
    }
  }
}

void drawScoreDigit4x7(Pixel* canvas, uint8_t digit, int16_t x, int16_t y, const Pixel& value) {
  if (digit > 9u) return;
  for (uint8_t row=0; row<7; ++row) {
    const uint8_t bits = SCORE_DIGITS_4X7[digit][row];
    for (uint8_t col=0; col<4; ++col) {
      if ((bits & (1u << (3u-col))) != 0) putPixel(canvas, x+col, y+row, value);
    }
  }
}

void drawScore3Digit(Pixel* canvas, uint16_t score, int16_t y, const Pixel& value) {
  score %= 1000u;
  drawScoreDigit4x7(canvas, uint8_t((score/100u)%10u), 1, y, value);
  drawScoreDigit4x7(canvas, uint8_t((score/10u)%10u), 6, y, value);
  drawScoreDigit4x7(canvas, uint8_t(score%10u), 11, y, value);
}

void IDotMatrixRenderer::renderCountdown(uint32_t remainingMillis, uint32_t animationMillis) {
  if (pixels_ == nullptr) return;

  Pixel base[16 * 16]{};
  const uint32_t remainingSeconds = (remainingMillis + 999u) / 1000u;
  const uint8_t minutes = uint8_t((remainingSeconds / 60u) % 100u);
  const uint8_t seconds = uint8_t(remainingSeconds % 60u);
  const Pixel minuteColor = color(255,255,255);
  const Pixel secondColor = remainingSeconds <= 10u ? color(255,0,0) : color(242,119,6);
  const bool colonVisible = ((animationMillis / 500u) & 1u) == 0u;

  // Original-device hourglass: 10 frames at 200 ms. At 00:00 the final
  // frame remains displayed instead of continuing to animate.
  const uint8_t frame = remainingSeconds == 0u ? 9u : uint8_t((animationMillis / 200u) % 10u);
  drawCountdownHourglass(base, frame);
  drawDigit(base, minutes / 10u, 9, 3, minuteColor);
  drawDigit(base, minutes % 10u, 13, 3, minuteColor);
  drawDigit(base, seconds / 10u, 9, 9, secondColor);
  drawDigit(base, seconds % 10u, 13, 9, secondColor);
  if (colonVisible && width_ == 16u) {
    putPixel(base, 7, 10, secondColor);
    putPixel(base, 7, 12, secondColor);
  }

  scaleLegacyCanvas(base, pixels_, width_, height_);

  // On 32x32/64x64 the separator is shifted right by half of a legacy
  // 16x16 pixel so it sits optically centered between the timer groups.
  if (colonVisible && (width_ == 32u || width_ == 64u) && height_ == width_) {
    const uint8_t scale = uint8_t(width_ / 16u);
    const uint8_t shift = uint8_t(scale / 2u);
    const uint8_t colonX = uint8_t(7u * scale + shift);
    const uint8_t colonRows[2] = {uint8_t(10u * scale), uint8_t(12u * scale)};
    for (uint8_t rowIndex = 0; rowIndex < 2u; ++rowIndex) {
      const uint8_t blockY = colonRows[rowIndex];
      for (uint8_t yy = 0; yy < scale; ++yy) {
        for (uint8_t xx = 0; xx < scale; ++xx) {
          pixels_[size_t(blockY + yy) * width_ + uint8_t(colonX + xx)] = secondColor;
        }
      }
    }
  }
  visible_ = true;
}

void IDotMatrixRenderer::renderStopwatch(uint32_t elapsedMillis) {
  if (pixels_ == nullptr) return;

  Pixel base[16 * 16]{};
  const uint32_t elapsedSeconds = elapsedMillis / 1000u;
  const uint8_t minutes = uint8_t((elapsedSeconds / 60u) % 100u);
  const uint8_t seconds = uint8_t(elapsedSeconds % 60u);
  const Pixel minuteColor = color(255,255,255);
  const Pixel secondColor = color(242,119,6);

  // Eight original hand positions, indexed by elapsed time so pause naturally
  // freezes the artwork and resume continues from the same phase.
  drawStopwatchArtwork(base, uint8_t((elapsedMillis / 100u) & 7u));
  drawDigit(base, minutes / 10u, 9, 3, minuteColor);
  drawDigit(base, minutes % 10u, 13, 3, minuteColor);
  drawDigit(base, seconds / 10u, 9, 9, secondColor);
  drawDigit(base, seconds % 10u, 13, 9, secondColor);
  const bool colonVisible = ((elapsedMillis / 500u) & 1u) == 0u;
  if (colonVisible && width_ == 16u) {
    putPixel(base, 7, 10, secondColor);
    putPixel(base, 7, 12, secondColor);
  }

  scaleLegacyCanvas(base, pixels_, width_, height_);

  // Match Countdown: on larger canvases move the separator right by half
  // of one legacy 16x16 pixel (1 native LED on 32x32, 2 on 64x64).
  if (colonVisible && (width_ == 32u || width_ == 64u) && height_ == width_) {
    const uint8_t scale = uint8_t(width_ / 16u);
    const uint8_t shift = uint8_t(scale / 2u);
    const uint8_t colonX = uint8_t(7u * scale + shift);
    const uint8_t colonRows[2] = {uint8_t(10u * scale), uint8_t(12u * scale)};
    for (uint8_t rowIndex = 0; rowIndex < 2u; ++rowIndex) {
      const uint8_t blockY = colonRows[rowIndex];
      for (uint8_t yy = 0; yy < scale; ++yy) {
        for (uint8_t xx = 0; xx < scale; ++xx) {
          pixels_[size_t(blockY + yy) * width_ + uint8_t(colonX + xx)] = secondColor;
        }
      }
    }
  }
  visible_ = true;
}

void IDotMatrixRenderer::renderScoreboard(uint16_t scoreA, uint16_t scoreB) {
  if (pixels_ == nullptr) return;

  Pixel base[16 * 16]{};
  drawScore3Digit(base, scoreA, 0, color(120,88,248));
  drawScore3Digit(base, scoreB, 9, color(248,32,120));

  scaleLegacyCanvas(base, pixels_, width_, height_);
  visible_ = true;
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

  const Pixel selected = color(red, green, blue);

  // Native 64x64 two-line layouts for the date-capable clocks.  Styles 0
  // (rainbow frame) and 3 (solid blue/background colour) use the extra
  // resolution to keep HH:MM and DD/MM visible together instead of alternating.
  const uint8_t normalizedStyle = style & 0x07u;
  if ((normalizedStyle == 0u || normalizedStyle == 3u) && renderDate &&
      logicalWidth_ == 64u && width_ == 64u && height_ == 64u) {
    if (normalizedStyle == 0u) {
      Pixel border[16 * 16]{};
      drawRainbowBorder(border, animationMillis);
      scaleLegacyCanvas(border, pixels_, width_, height_);
    } else {
      fill(red, green, blue);
    }

    const Pixel foreground = normalizedStyle == 3u ? color(0, 0, 0) : selected;

    auto fillBlock = [&](int16_t x, int16_t y, uint8_t scale, const Pixel& value) {
      for (uint8_t yy = 0; yy < scale; ++yy) {
        for (uint8_t xx = 0; xx < scale; ++xx) {
          setPixel(uint8_t(x + xx), uint8_t(y + yy), value.red, value.green, value.blue);
        }
      }
    };
    auto drawNativeDigit = [&](uint8_t digit, int16_t x, int16_t y, uint8_t scale,
                               const Pixel& value) {
      if (digit > 9u) return;
      for (uint8_t row = 0; row < 5u; ++row) {
        for (uint8_t column = 0; column < 3u; ++column) {
          if ((DIGITS_3X5[digit][row] & (1u << (2u - column))) != 0u) {
            fillBlock(x + int16_t(column) * scale, y + int16_t(row) * scale, scale, value);
          }
        }
      }
    };

    uint8_t hourValue = clockHour;
    if (!use24Hour) {
      if (hourValue == 0u) hourValue = 12u;
      else if (hourValue > 12u) hourValue = uint8_t(hourValue - 12u);
    }

    // HH:MM: 3x5 font at 3x scale. Width = 49 px, centered at x=7.
    constexpr uint8_t timeScale = 3u;
    constexpr int16_t timeY = 14;
    constexpr int16_t timeX = 7;
    drawNativeDigit(hourValue / 10u, timeX, timeY, timeScale, foreground);
    drawNativeDigit(hourValue % 10u, timeX + 11, timeY, timeScale, foreground);
    if ((animationMillis % 1000u) < 500u) {
      fillBlock(timeX + 23, timeY + 3, 3u, foreground);
      fillBlock(timeX + 23, timeY + 9, 3u, foreground);
    }
    drawNativeDigit(clockMinute / 10u, timeX + 29, timeY, timeScale, foreground);
    drawNativeDigit(clockMinute % 10u, timeX + 40, timeY, timeScale, foreground);

    // DD/MM: keep the day fixed and move the slash + complete month field
    // two physical LEDs right on the native 64x64 styles 0/3.
    constexpr uint8_t dateScale = 2u;
    constexpr int16_t dateY = 39;
    constexpr int16_t dateX = 12;
    constexpr int16_t monthShiftX = 2;
    drawNativeDigit(clockDay / 10u, dateX, dateY, dateScale, foreground);
    drawNativeDigit(clockDay % 10u, dateX + 8, dateY, dateScale, foreground);
    // Slash, scaled from a 3x5 diagonal glyph.
    fillBlock(dateX + 18 + monthShiftX, dateY, 2u, foreground);
    fillBlock(dateX + 18 + monthShiftX, dateY + 2, 2u, foreground);
    fillBlock(dateX + 16 + monthShiftX, dateY + 4, 2u, foreground);
    fillBlock(dateX + 14 + monthShiftX, dateY + 6, 2u, foreground);
    fillBlock(dateX + 14 + monthShiftX, dateY + 8, 2u, foreground);
    drawNativeDigit(clockMonth / 10u, dateX + 22 + monthShiftX, dateY, dateScale, foreground);
    drawNativeDigit(clockMonth % 10u, dateX + 30 + monthShiftX, dateY, dateScale, foreground);

    visible_ = true;
    return;
  }

  Pixel base[16 * 16]{};
  uint8_t top = renderDate ? clockDay : clockHour;
  const uint8_t bottom = renderDate ? clockMonth : clockMinute;
  if (!renderDate && !use24Hour) {
    if (top == 0) top = 12;
    else if (top > 12) top -= 12;
  }

  // One complete blink per second: 500 ms on, 500 ms off.  Date separators
  // do not blink and remain continuously visible.
  const bool separatorVisible = renderDate || ((animationMillis % 1000u) < 500u);

  switch (style & 0x07) {
    case 0:
      drawRainbowBorder(base, animationMillis);
      drawTwoRows(base, top, bottom, selected, selected, renderDate, separatorVisible, 2);
      break;
    case 1: {
      const Pixel clockRed = color(255, 0, 0);
      drawDigit(base, top / 10, 2, 1, clockRed);
      drawDigit(base, top % 10, 6, 1, clockRed);
      drawClockSeparator(base, 10, 1, clockRed, renderDate, separatorVisible);
      drawChristmasTree(base);
      drawDigit(base, bottom / 10, 7, 9, clockRed);
      drawDigit(base, bottom % 10, 11, 9, clockRed);
      break;
    }
    case 2: {
      drawRacingBands(base);
      const Pixel orange = color(255, 170, 0);
      const Pixel white = color(255, 255, 255);
      // Micro-positioning copied from the emulator follow-up: both hour
      // digits and the first minute digit move one pixel left.  The separator
      // and the second minute digit stay at their established coordinates.
      drawDigit(base, top / 10, 0, 5, orange);
      drawDigit(base, top % 10, 4, 5, orange);
      if (renderDate) {
        putPixel(base, 9, 5, white);
        putPixel(base, 9, 6, white);
        putPixel(base, 8, 7, white);
        putPixel(base, 8, 8, white);
      } else if (separatorVisible && width_ == 16u) {
        // On 32x32 and 64x64 the separator is overlaid after scaling so it
        // can be shifted by half of a legacy 16x16 pixel: 1 or 2 native LEDs.
        putPixel(base, 8, 6, white);
        putPixel(base, 8, 8, white);
      }
      drawDigit(base, bottom / 10, 9, 5, orange);
      drawDigit(base, bottom % 10, 13, 5, orange);
      break;
    }
    case 3:
      for (Pixel& pixel : base) pixel = selected;
      drawTwoRows(base, top, bottom, color(0, 0, 0), color(0, 0, 0), renderDate, separatorVisible, 2);
      break;
    case 4:
      drawDigit(base, top / 10, 2, 1, selected);
      drawDigit(base, top % 10, 6, 1, selected);
      drawClockSeparator(base, renderDate ? 11 : 10, 1, selected, renderDate, separatorVisible);
      drawHourglass(base);
      drawDigit(base, bottom / 10, 6, 9, selected);
      drawDigit(base, bottom % 10, 10, 9, selected);
      break;
    case 5:
      drawBlueFrame(base, true);
      drawTwoRows(base, top, bottom, color(255, 165, 0), color(255, 165, 0), renderDate, separatorVisible, 2);
      break;
    case 6:
      drawTwoRows(base, top, bottom, selected, selected, renderDate, separatorVisible, 2);
      drawBlueFrame(base, false);
      break;
    case 7:
      drawQuadrantBorder(base);
      drawTwoRows(base, top, bottom, selected, selected, renderDate, separatorVisible, 2);
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

  // Style 2 uses a half-legacy-pixel correction for the time separator on
  // larger logical canvases.  That is 1 native LED on 32x32 and 2 on 64x64,
  // which centers the colon between the hour and minute groups without moving
  // any digit or racing-band artwork.
  if ((style & 0x07u) == 2u && !renderDate && separatorVisible &&
      (width_ == 32u || width_ == 64u) && height_ == width_) {
    const uint8_t scale = uint8_t(width_ / 16u);
    const uint8_t shift = uint8_t(scale / 2u);
    const uint8_t colonX = uint8_t(8u * scale - shift);
    const Pixel white = color(255, 255, 255);
    const uint8_t colonRows[2] = {uint8_t(6u * scale), uint8_t(8u * scale)};
    for (uint8_t rowIndex = 0; rowIndex < 2u; ++rowIndex) {
      const uint8_t blockY = colonRows[rowIndex];
      for (uint8_t yy = 0; yy < scale; ++yy) {
        for (uint8_t xx = 0; xx < scale; ++xx) {
          const uint8_t x = uint8_t(colonX + xx);
          const uint8_t y = uint8_t(blockY + yy);
          pixels_[size_t(y) * width_ + x] = white;
        }
      }
    }
  }
  visible_ = true;
}

bool IDotMatrixRenderer::beginText(
  uint8_t glyphCount,
  uint8_t glyphWidth,
  uint8_t glyphHeight,
  uint16_t glyphBytes,
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
    (glyphWidth == 16 && glyphHeight == 32 && glyphBytes == 64) ||
    (glyphWidth == 32 && glyphHeight == 64 && glyphBytes == 256);
  const size_t required = size_t(glyphCount) * glyphBytes;
  if (pixels_ == nullptr || width_ == 0 || height_ == 0 ||
      !formatValid || glyphCount == 0 || required > 16384) {
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
  textLastMove_ = now;
  textLastPageChange_ = now;
  textFirstVisibleGlyph_ = 0;
  textFrameRendered_ = false;
  textValid_ = true;

  const int16_t textWidth = int16_t(textGlyphCount_) * textGlyphWidth_;
  const int16_t centeredY = logicalHeight_ > textGlyphHeight_
    ? int16_t(logicalHeight_ - textGlyphHeight_) / 2
    : 0;
  switch (textMotionEffect_) {
    case 1:
      textOffsetX_ = logicalWidth_;
      textOffsetY_ = centeredY;
      break;
    case 2:
      textOffsetX_ = -textWidth;
      textOffsetY_ = centeredY;
      break;
    case 3:
    case 4:
      // Vertical text starts with the first page already visible.  Subsequent
      // pages form a continuous tape separated by one logical pixel.
      textOffsetX_ = 0;
      textOffsetY_ = centeredY;
      break;
    default:
      textOffsetX_ = 0;
      textOffsetY_ = centeredY;
      break;
  }
  return true;
}

uint8_t IDotMatrixRenderer::textVisibleCapacity() const {
  if (textGlyphWidth_ == 0 || textGlyphHeight_ == 0 ||
      logicalWidth_ == 0 || logicalHeight_ == 0) return 0;

  uint16_t columns = uint16_t(logicalWidth_) / textGlyphWidth_;
  if (columns == 0) columns = 1;

  // The original iDotMatrix uses the complete matrix height for text effects
  // that do not scroll.  Glyphs fill the page row-by-row, so a 64x64 matrix
  // can show four 16-pixel rows or two 32-pixel rows at once.  Scrolling
  // effects remain a single text line/tape and therefore keep the historical
  // horizontal-only capacity.
  uint16_t rows = 1;
  if (textMotionEffect_ != 1 && textMotionEffect_ != 2 &&
      textMotionEffect_ != 3 && textMotionEffect_ != 4) {
    rows = uint16_t(logicalHeight_) / textGlyphHeight_;
    if (rows == 0) rows = 1;
  }

  const uint16_t capacity = columns * rows;
  return capacity > 255u ? 255u : uint8_t(capacity);
}

uint32_t IDotMatrixRenderer::textPresentationDurationMs() const {
  if (!textValid_ || textGlyphCount_ == 0 || textGlyphWidth_ == 0) return 3000u;
  const uint8_t boundedSpeed = textSpeed_ > 100 ? 100 : textSpeed_;
  const uint32_t moveInterval = 500u - uint32_t(boundedSpeed) * 485u / 100u;
  const uint8_t pageCapacity = textVisibleCapacity();
  const uint32_t textWidth = uint32_t(textGlyphCount_) * textGlyphWidth_;

  if (textMotionEffect_ == 1 || textMotionEffect_ == 2) {
    const uint32_t pixels = uint32_t(logicalWidth_) + textWidth + 1u;
    const uint32_t stepsPerTick = (logicalWidth_ >= 64 && boundedSpeed >= 90) ? 2u : 1u;
    return ((pixels + stepsPerTick - 1u) / stepsPerTick) * moveInterval;
  }

  if (pageCapacity == 0 || textGlyphCount_ <= pageCapacity) return 3000u;
  const uint32_t pages = (uint32_t(textGlyphCount_) + pageCapacity - 1u) / pageCapacity;
  const uint32_t pageStep = (textMotionEffect_ == 3 || textMotionEffect_ == 4)
    ? uint32_t(logicalHeight_ - ((logicalHeight_ > textGlyphHeight_)
      ? (logicalHeight_ - textGlyphHeight_) / 2u : 0u))
    : uint32_t(textGlyphHeight_) + 1u;
  return (pages - 1u) * pageStep * moveInterval + 3000u;
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
  // Keep physical movement independent from visual refresh.  The app speed
  // field therefore has the same meaning for solid and animated colours.
  const uint16_t moveInterval = 500u - uint16_t(boundedSpeed) * 485u / 100u;
  const bool animatedVisual = textMotionEffect_ >= 5 || textColorMode_ >= 2;
  const uint16_t renderInterval = animatedVisual ? 45u : moveInterval;
  if (textFrameRendered_ && uint32_t(now - textLastFrame_) < renderInterval) return;

  const int16_t textWidth = int16_t(textGlyphCount_) * textGlyphWidth_;
  const uint8_t pageCapacity = textVisibleCapacity();
  const bool multiplePages = pageCapacity > 0 && textGlyphCount_ > pageCapacity;
  const int16_t centeredY = logicalHeight_ > textGlyphHeight_
    ? int16_t(logicalHeight_ - textGlyphHeight_) / 2
    : 0;
  // Page-based effects keep the historical glyph-height cadence. Vertical
  // UP/DOWN motion is different: each logical page is centered in the
  // viewport, so the following page must start fully outside the viewport.
  // Using glyphHeight here makes part of the next page already visible on
  // 64x64 (for example a 16x32 glyph page centered at y=16), which appears as
  // a block entering at once instead of one raster row at a time.
  const int16_t pageStep = int16_t(textGlyphHeight_) + 1;
  const int16_t verticalPageStep = int16_t(logicalHeight_) - centeredY;

  auto nextPageStart = [&]() -> uint8_t {
    if (!multiplePages) return 0;
    const uint16_t candidate = uint16_t(textFirstVisibleGlyph_) + pageCapacity;
    return candidate >= textGlyphCount_ ? 0 : uint8_t(candidate);
  };

  const bool moveNow = !textFrameRendered_ || uint32_t(now - textLastMove_) >= moveInterval;
  if (textFrameRendered_ && moveNow) {
    // At the top end of the app slider a native 64x64 canvas otherwise hits
    // the WLED render-rate ceiling: 15 ms/pixel is already shorter than one
    // ~43 FPS frame, so reducing the interval further would have no visible
    // effect.  Allow a second logical pixel per accepted render only for the
    // 64-pixel profile and only in the final 10% of the speed range.  Lower
    // speeds and 16/32 profiles retain the historical one-pixel cadence.
    const uint8_t moveSteps = (logicalWidth_ >= 64 && boundedSpeed >= 90) ? 2u : 1u;
    for (uint8_t step = 0; step < moveSteps; ++step) {
      switch (textMotionEffect_) {
        case 1:
          if (--textOffsetX_ < -textWidth) textOffsetX_ = logicalWidth_;
          break;
        case 2:
          if (++textOffsetX_ > logicalWidth_) textOffsetX_ = -textWidth;
          break;
        case 3: // UP: next page enters from just below the logical viewport.
          --textOffsetY_;
          if (textOffsetY_ <= centeredY - verticalPageStep) {
            textFirstVisibleGlyph_ = nextPageStart();
            textOffsetY_ += verticalPageStep;
            textLastPageChange_ = now;
          }
          break;
        case 4: // DOWN: next page enters from just above the logical viewport.
          ++textOffsetY_;
          if (textOffsetY_ >= centeredY + verticalPageStep) {
            textFirstVisibleGlyph_ = nextPageStart();
            textOffsetY_ -= verticalPageStep;
            textLastPageChange_ = now;
          }
          break;
        default:
          step = moveSteps; // no positional motion for this effect
          break;
      }
    }
    textLastMove_ = now;
  }

  // Page-based effects (including the stationary presentation) must consume
  // the complete glyph stream.  Use the same page+gap travel time as vertical
  // motion so the app speed value remains the sole paging cadence control.
  if (textMotionEffect_ != 1 && textMotionEffect_ != 2 &&
      textMotionEffect_ != 3 && textMotionEffect_ != 4 && multiplePages) {
    const uint32_t pageInterval = uint32_t(moveInterval) * uint32_t(pageStep);
    if (uint32_t(now - textLastPageChange_) >= pageInterval) {
      textFirstVisibleGlyph_ = nextPageStart();
      textLastPageChange_ = now;
    }
  }

  textLastFrame_ = now;
  textFrameRendered_ = true;

  const Pixel background = textBackgroundEnabled_ ? textBackground_ : Pixel{0, 0, 0};
  for (size_t index = 0; index < pixelCount_; ++index) pixels_[index] = background;

  const uint32_t elapsed = now - textAnimationStart_;
  const bool blinkHidden = textMotionEffect_ == 5 && ((elapsed / 350u) & 1u) != 0;
  const int16_t laserRow = textMotionEffect_ == 8
    ? int16_t((elapsed / 70u) % logicalHeight_)
    : -1;
  uint8_t brightnessScale = 255;
  if (textMotionEffect_ == 6) {
    brightnessScale = 40u + uint16_t(triangleWave(uint8_t(elapsed / 8u))) * 215u / 255u;
  } else if (textMotionEffect_ == 8) {
    brightnessScale = 110;
  }

  auto drawGlyphRange = [&](uint8_t firstGlyph, uint8_t count, int16_t baseX, int16_t baseY) {
    const uint8_t bytesPerRow = (textGlyphWidth_ + 7u) / 8u;
    for (uint8_t pageIndex = 0; pageIndex < count; ++pageIndex) {
      const uint16_t glyphIndex = uint16_t(firstGlyph) + pageIndex;
      if (glyphIndex >= textGlyphCount_) break;
      const uint8_t* bitmap = textBitmaps_ + size_t(glyphIndex) * textGlyphBytes_;
      const int16_t glyphX = baseX + int16_t(pageIndex) * textGlyphWidth_;
      for (uint8_t row = 0; row < textGlyphHeight_; ++row) {
        for (uint8_t column = 0; column < textGlyphWidth_; ++column) {
          const uint16_t offset = uint16_t(row) * bytesPerRow + (column >> 3);
          if (offset >= textGlyphBytes_ ||
              (bitmap[offset] & (1u << (column & 7u))) == 0) {
            continue;
          }
          const int16_t x = glyphX + column;
          const int16_t y = baseY + row;
          if (x < 0 || y < 0 || x >= logicalWidth_ || y >= logicalHeight_) continue;

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
          if (lowMemoryRescale()) {
            setAnimationSourcePixel(uint8_t(x), uint8_t(y), value.red, value.green, value.blue);
          } else {
            pixels_[size_t(y) * width_ + x] = value;
          }
        }
      }
    }
  };

  if (!blinkHidden) {
    if (textMotionEffect_ == 1 || textMotionEffect_ == 2) {
      // Horizontal motion remains one continuous full text line.
      drawGlyphRange(0, textGlyphCount_, textOffsetX_, textOffsetY_);
    } else if (textMotionEffect_ == 3 || textMotionEffect_ == 4) {
      // Vertical motion is a continuous tape: the following logical page starts
      // fully outside the viewport and enters one logical raster row at a time.
      drawGlyphRange(textFirstVisibleGlyph_, pageCapacity, 0, textOffsetY_);
      const uint8_t following = nextPageStart();
      const int16_t followingY = textMotionEffect_ == 3
        ? textOffsetY_ + verticalPageStep
        : textOffsetY_ - verticalPageStep;
      drawGlyphRange(following, pageCapacity, 0, followingY);
    } else {
      // Non-scrolling text effects use the full matrix as a page, matching the
      // original device: 64x64 shows 1x64, 2x32 or 4x16 rows; 32x32 shows
      // 1x32 or 2x16 rows; 16x16 remains a single 16-pixel row.  Center the
      // block of actually used rows vertically, including short final pages.
      uint8_t columns = textGlyphWidth_ == 0 ? 1u : uint8_t(logicalWidth_ / textGlyphWidth_);
      if (columns == 0) columns = 1;
      uint8_t maxRows = textGlyphHeight_ == 0 ? 1u : uint8_t(logicalHeight_ / textGlyphHeight_);
      if (maxRows == 0) maxRows = 1;
      const uint8_t remaining = textFirstVisibleGlyph_ < textGlyphCount_
        ? uint8_t(textGlyphCount_ - textFirstVisibleGlyph_) : 0u;
      const uint8_t glyphsOnPage = remaining < pageCapacity ? remaining : pageCapacity;
      uint8_t rowsUsed = glyphsOnPage == 0 ? 1u : uint8_t((glyphsOnPage + columns - 1u) / columns);
      if (rowsUsed > maxRows) rowsUsed = maxRows;
      const uint16_t blockHeight = uint16_t(rowsUsed) * textGlyphHeight_;
      const int16_t pageY = logicalHeight_ > blockHeight
        ? int16_t(logicalHeight_ - blockHeight) / 2
        : 0;

      for (uint8_t row = 0; row < rowsUsed; ++row) {
        const uint16_t consumed = uint16_t(row) * columns;
        if (consumed >= glyphsOnPage) break;
        const uint8_t rowCount = uint8_t(
          (glyphsOnPage - consumed) < columns ? (glyphsOnPage - consumed) : columns
        );
        drawGlyphRange(
          uint8_t(textFirstVisibleGlyph_ + consumed),
          rowCount,
          0,
          int16_t(pageY + uint16_t(row) * textGlyphHeight_)
        );
      }
    }
  }

  if (textMotionEffect_ == 7) {
    const uint16_t phase = elapsed / 100u;
    // The old eight-particle pattern used a regular index*3 Y offset.  On a
    // 16x16 logical canvas (especially when upscaled 4x) this aligned the
    // flakes into a visible travelling band followed by a largely empty
    // interval.  Use deterministic per-particle phase offsets and slightly
    // different fall rates instead, while scaling density with canvas width.
    const uint8_t flakeCount = logicalWidth_ >= 64 ? 48u :
      (logicalWidth_ >= 32 ? 24u : 16u);
    auto snowHash = [](uint32_t value) -> uint32_t {
      value ^= value >> 16;
      value *= 0x7FEB352DUL;
      value ^= value >> 15;
      value *= 0x846CA68BUL;
      value ^= value >> 16;
      return value;
    };
    for (uint8_t index = 0; index < flakeCount; ++index) {
      const uint32_t seed = snowHash(uint32_t(index) * 0x9E3779B9UL + 0x51ED270Bu);
      const uint8_t x = uint8_t(seed % logicalWidth_);
      const uint16_t startY = uint16_t((seed >> 8) % logicalHeight_);
      const uint8_t rate = uint8_t(1u + ((seed >> 20) & 0x01u));
      const uint8_t y = uint8_t((startY + uint32_t(phase) * rate) % logicalHeight_);
      if (lowMemoryRescale()) setAnimationSourcePixel(x, y, 255, 255, 255);
      else pixels_[size_t(y) * width_ + x] = Pixel{255, 255, 255};
    }
  } else if (textMotionEffect_ == 8 && laserRow >= 0) {
    const Pixel laser = scaledColor(Pixel{255, 0, 0}, 120);
    if (lowMemoryRescale()) {
      for (uint8_t x = 0; x < logicalWidth_; ++x) {
        setAnimationSourcePixel(x, uint8_t(laserRow), laser.red, laser.green, laser.blue);
      }
    } else {
      for (uint8_t x = 0; x < width_; ++x) {
        pixels_[size_t(laserRow) * width_ + x] = laser;
      }
    }
  }
  visible_ = true;
}

bool IDotMatrixRenderer::beginRawImage(size_t byteLength) {
  cancelRawImage();
  const size_t sourceRequired = size_t(logicalWidth_) * logicalHeight_ * sizeof(Pixel);
  if (pixels_ == nullptr || sourceRequired == 0 || byteLength != sourceRequired) return false;

  rawImagePixels_ = allocatePixelBuffer(pixelCount_);
  rawImageInPlace_ = rawImagePixels_ == nullptr;
  if (rawImageInPlace_) {
    rawImagePixels_ = pixels_;
    visible_ = false;
  } else {
    memset(rawImagePixels_, 0, pixelCount_ * sizeof(Pixel));
  }
  // This is the source byte count, not the size of the downscaled storage.
  rawImageBytes_ = sourceRequired;
  return rawImagePixels_ != nullptr;
}

bool IDotMatrixRenderer::writeRawImage(
  size_t offset,
  const uint8_t* data,
  size_t length
) {
  if (rawImagePixels_ == nullptr || data == nullptr ||
      offset > rawImageBytes_ || length > rawImageBytes_ - offset) {
    return false;
  }

  if (!lowMemoryRescale()) {
    memcpy(reinterpret_cast<uint8_t*>(rawImagePixels_) + offset, data, length);
    return true;
  }

  // Stream a logical 32/64 image directly into the smaller physical canvas.
  // RGB components may cross BLE chunk boundaries, so process byte-by-byte.
  for (size_t i = 0; i < length; ++i) {
    const size_t sourceByte = offset + i;
    const size_t sourcePixel = sourceByte / sizeof(Pixel);
    const uint8_t channel = uint8_t(sourceByte % sizeof(Pixel));
    const uint8_t sourceX = uint8_t(sourcePixel % logicalWidth_);
    const uint8_t sourceY = uint8_t(sourcePixel / logicalWidth_);
    const uint8_t targetX = uint8_t(uint16_t(sourceX) * width_ / logicalWidth_);
    const uint8_t targetY = uint8_t(uint16_t(sourceY) * height_ / logicalHeight_);
    if (targetX >= width_ || targetY >= height_) continue;
    if (uint16_t(targetX) * logicalWidth_ / width_ != sourceX ||
        uint16_t(targetY) * logicalHeight_ / height_ != sourceY) continue;
    uint8_t* target = reinterpret_cast<uint8_t*>(
      &rawImagePixels_[size_t(targetY) * width_ + targetX]
    );
    target[channel] = data[i];
  }
  return true;
}

bool IDotMatrixRenderer::completeRawImage(bool crcValid) {
  if (rawImagePixels_ == nullptr) return false;
  if (!crcValid) {
    if (rawImageInPlace_) {
      rawImagePixels_ = nullptr;
      rawImageBytes_ = 0;
      rawImageInPlace_ = false;
      clear();
      visible_ = false;
    } else {
      cancelRawImage();
    }
    return false;
  }

  if (rawImageInPlace_) {
    rawImagePixels_ = nullptr;
    rawImageBytes_ = 0;
    rawImageInPlace_ = false;
  } else {
    Pixel* previous = pixels_;
    pixels_ = rawImagePixels_;
    rawImagePixels_ = nullptr;
    rawImageBytes_ = 0;
    freePixelBuffer(previous);
  }
  textValid_ = false;
  textFrameRendered_ = false;
  visible_ = true;
  return true;
}

void IDotMatrixRenderer::cancelRawImage() {
  if (rawImagePixels_ != nullptr && !rawImageInPlace_) {
    freePixelBuffer(rawImagePixels_);
  }
  rawImagePixels_ = nullptr;
  rawImageBytes_ = 0;
  rawImageInPlace_ = false;
}

uint8_t IDotMatrixRenderer::dimensionForScreenType(uint8_t screenType) {
  switch (screenType) {
    case 0x03: return 32;
    case 0x04: return 64;
    case 0x01:
    default: return 16;
  }
}
