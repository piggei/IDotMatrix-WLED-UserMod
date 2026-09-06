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
      constexpr uint8_t stripeWidth = 4;
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
      constexpr uint8_t stripeWidth = 4;
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
      constexpr uint8_t colorWidth = 5;
      constexpr uint8_t blackWidth = 4;
      constexpr uint8_t blockWidth = colorWidth + blackWidth;
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
void drawTimerIcon(Pixel* canvas, uint8_t phase) {
  const Pixel rim = color(255, 145, 0);
  const Pixel hand = color(255, 45, 20);
  const Pixel center = color(255, 220, 120);
  constexpr int8_t cx = 7;
  constexpr int8_t cy = 4;

  // 9x9 pixel-art timer from the latest standalone emulator (BUILD80).
  putPixel(canvas, 6, 0, rim); putPixel(canvas, 7, 0, rim); putPixel(canvas, 8, 0, rim);
  putPixel(canvas, 7, 1, rim);
  putPixel(canvas, 4, 1, rim); putPixel(canvas, 10, 1, rim);
  putPixel(canvas, 3, 2, rim); putPixel(canvas, 11, 2, rim);
  putPixel(canvas, 2, 3, rim); putPixel(canvas, 12, 3, rim);
  putPixel(canvas, 2, 4, rim); putPixel(canvas, 12, 4, rim);
  putPixel(canvas, 2, 5, rim); putPixel(canvas, 12, 5, rim);
  putPixel(canvas, 3, 6, rim); putPixel(canvas, 11, 6, rim);
  putPixel(canvas, 4, 7, rim); putPixel(canvas, 10, 7, rim);
  putPixel(canvas, 5, 8, rim); putPixel(canvas, 6, 8, rim); putPixel(canvas, 7, 8, rim);
  putPixel(canvas, 8, 8, rim); putPixel(canvas, 9, 8, rim);

  static const int8_t handX[8] = {7, 10, 11, 10, 7, 4, 3, 4};
  static const int8_t handY[8] = {1, 2, 4, 6, 7, 6, 4, 2};
  const int8_t endX = handX[phase & 7u];
  const int8_t endY = handY[phase & 7u];
  putPixel(canvas, cx, cy, center);
  putPixel(canvas, (cx + endX) / 2, (cy + endY) / 2, hand);
  putPixel(canvas, endX, endY, hand);
}

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

void audioLine(Pixel* base, int x0, int y0, int x1, int y1, const Pixel& value) {
  const int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  const int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int error = dx + dy;
  while (true) {
    putPixel(base, x0, y0, value);
    if (x0 == x1 && y0 == y1) break;
    const int twice = 2 * error;
    if (twice >= dy) { error += dy; x0 += sx; }
    if (twice <= dx) { error += dx; y0 += sy; }
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
  bool fft, uint8_t mode, uint8_t level, const uint8_t bands[8], uint32_t now
) {
  if (pixels_ == nullptr) return;
  Pixel base[16 * 16]{};
  const Pixel white = color(255, 255, 255);
  level = audioBand(level);

  if (!fft) switch (mode > 4 ? 4 : mode) {
    case 0: {
      const uint8_t pose = uint8_t((now / 150u + level) % 6u);
      const Pixel green = color(45,255,25), green2 = color(15,150,15);
      for (int x=0;x<16;x++) if (((x+pose)&1)==0) { putPixel(base,x,0,green); putPixel(base,x,1,green2); }
      struct Pose { int hx,hy,sx,sy,px,py,lx,ly,rx,ry,lfx,lfy,rfx,rfy; };
      static const Pose poses[6] = {
        {8,3,8,6,8,10,4,7,12,8,5,14,11,14}, {7,3,8,6,8,10,3,9,12,5,4,13,12,14},
        {9,4,8,7,7,10,3,5,13,10,2,14,10,13}, {5,7,7,8,9,10,3,11,10,5,4,14,13,12},
        {8,11,8,9,8,7,4,12,12,12,5,4,11,4}, {10,3,9,6,8,10,5,5,13,7,4,14,10,13}
      };
      const Pose& p = poses[pose];
      for (int yy=-1;yy<=1;yy++) for (int xx=-1;xx<=1;xx++) putPixel(base,p.hx+xx,p.hy+yy,white);
      audioLine(base,p.sx,p.sy,p.px,p.py,white); audioLine(base,p.sx,p.sy,p.lx,p.ly,white);
      audioLine(base,p.sx,p.sy,p.rx,p.ry,white); audioLine(base,p.px,p.py,p.lfx,p.lfy,white);
      audioLine(base,p.px,p.py,p.rfx,p.rfy,white);
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
      for(int x=1;x<15;x+=2){putPixel(base,x,0,frame);putPixel(base,x,15,frame);}
      for(int y=1;y<15;y+=2){putPixel(base,0,y,frame);putPixel(base,15,y,frame);}
      const uint8_t strength = level < 1 ? 1 : level > 7 ? 7 : level;
      const uint32_t tick=now/95u;
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
      const Pixel red=color(255,20,25), cyan=color(0,235,255), blue=color(15,55,255), dark=color(4,15,90);
      for(int y=3;y<=7;y++){for(int x=1;x<=5;x++)putPixel(base,x,y,red);for(int x=10;x<=14;x++)putPixel(base,x,y,red);}
      for(int y=4;y<=6;y++){for(int x=2;x<=3;x++)putPixel(base,x,y,cyan);for(int x=12;x<=13;x++)putPixel(base,x,y,cyan);}
      const int drift=level>=5?1:0;putPixel(base,3+drift,5,white);putPixel(base,12+drift,5,white);
      for(int x=3;x<=12;x++) putPixel(base,x,11,blue);
      for(int x=4;x<=11;x++) putPixel(base,x,12,level>=5?blue:dark);
      if(level>=7)for(int x=5;x<=10;x++)putPixel(base,x,13,blue);
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

void IDotMatrixRenderer::renderCountdown(uint32_t remainingMillis) {
  if (pixels_ == nullptr) return;

  Pixel base[16 * 16]{};
  const uint32_t remainingSeconds = (remainingMillis + 999u) / 1000u;
  const Pixel digits = remainingSeconds <= 5u
    ? color(255, 0, 0)
    : color(255, 255, 255);
  const uint8_t minutes = uint8_t((remainingSeconds / 60u) % 100u);
  const uint8_t seconds = uint8_t(remainingSeconds % 60u);

  // Remaining time decreases.  Inverting the 125 ms phase reproduces the
  // standalone emulator's timer-hand progression.
  const uint8_t phase = uint8_t((8u - ((remainingMillis / 125u) & 7u)) & 7u);
  drawTimerIcon(base, phase);
  drawDigit(base, minutes / 10u, 0, 10, digits);
  drawDigit(base, minutes % 10u, 3, 10, digits);
  drawSeparator(base, 7, 10, digits, false);
  drawDigit(base, seconds / 10u, 9, 10, digits);
  drawDigit(base, seconds % 10u, 12, 10, digits);

  scaleLegacyCanvas(base, pixels_, width_, height_);
  visible_ = true;
}

void IDotMatrixRenderer::renderStopwatch(uint32_t elapsedMillis) {
  if (pixels_ == nullptr) return;

  Pixel base[16 * 16]{};
  const uint32_t elapsedSeconds = elapsedMillis / 1000u;
  const uint8_t minutes = uint8_t((elapsedSeconds / 60u) % 100u);
  const uint8_t seconds = uint8_t(elapsedSeconds % 60u);
  const Pixel digits = color(255, 255, 255);

  const uint8_t phase = uint8_t((elapsedMillis / 125u) & 7u);
  drawTimerIcon(base, phase);
  drawDigit(base, minutes / 10u, 0, 10, digits);
  drawDigit(base, minutes % 10u, 3, 10, digits);
  drawSeparator(base, 7, 10, digits, false);
  drawDigit(base, seconds / 10u, 9, 10, digits);
  drawDigit(base, seconds % 10u, 12, 10, digits);

  scaleLegacyCanvas(base, pixels_, width_, height_);
  visible_ = true;
}

void IDotMatrixRenderer::renderScoreboard(uint16_t scoreA, uint16_t scoreB) {
  if (pixels_ == nullptr) return;

  Pixel base[16 * 16]{};
  auto drawScore = [&base](uint16_t score, int16_t x, const Pixel& value) {
    score %= 100u;
    if (score >= 10u) drawDigit(base, uint8_t(score / 10u), x, 5, value);
    drawDigit(base, uint8_t(score % 10u), x + 3, 5, value);
  };

  drawScore(scoreA, 0, color(0, 0, 255));
  drawSeparator(base, 7, 5, color(255, 255, 255), false);
  drawScore(scoreB, 9, color(255, 0, 0));

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
  textLastMove_ = now;
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
      textOffsetX_ = 0;
      textOffsetY_ = logicalHeight_;
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
  // The app uses the full 0..100 field, but common presets sit near the slow
  // end (for example speed=5).  The former 140..20 ms mapping made most of the
  // slider feel almost identical.  Use a deliberately wider 500..15 ms range:
  // movement now spans from clearly readable to approximately one pixel per
  // WLED effect frame at the fast end.
  const uint16_t moveInterval = 500u - uint16_t(boundedSpeed) * 485u / 100u;

  // Visual effects (blink/colour animation) may need frequent redraws, but they
  // must not change the text movement cadence.  The previous implementation
  // clamped the common interval to 45 ms, effectively defeating the speed
  // slider whenever one of those effects was selected.
  const bool animatedVisual = textMotionEffect_ >= 5 || textColorMode_ >= 2;
  const uint16_t renderInterval = animatedVisual ? 45u : moveInterval;
  if (textFrameRendered_ && uint32_t(now - textLastFrame_) < renderInterval) return;

  const int16_t textWidth = int16_t(textGlyphCount_) * textGlyphWidth_;
  const bool moveNow = !textFrameRendered_ || uint32_t(now - textLastMove_) >= moveInterval;
  if (textFrameRendered_ && moveNow) {
    switch (textMotionEffect_) {
      case 1:
        if (--textOffsetX_ < -textWidth) textOffsetX_ = logicalWidth_;
        break;
      case 2:
        if (++textOffsetX_ > logicalWidth_) textOffsetX_ = -textWidth;
        break;
      case 3:
        if (--textOffsetY_ < -int16_t(textGlyphHeight_)) textOffsetY_ = logicalHeight_;
        break;
      case 4:
        if (++textOffsetY_ > logicalHeight_) textOffsetY_ = -int16_t(textGlyphHeight_);
        break;
      default:
        break;
    }
    textLastMove_ = now;
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
  }

  if (textMotionEffect_ == 7) {
    const uint16_t phase = elapsed / 100u;
    for (uint8_t index = 0; index < 8; ++index) {
      const uint8_t x = uint8_t((index * 5u + index * index * 3u) % logicalWidth_);
      const uint8_t y = uint8_t((phase + index * 3u) % logicalHeight_);
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
