#include "../IDotMatrixRenderer.h"

#include <cassert>

static void expectBlack(const IDotMatrixRenderer::Pixel* pixel) {
  assert(pixel != nullptr);
  assert(pixel->red == 0 && pixel->green == 0 && pixel->blue == 0);
}

int main() {
  IDotMatrixRenderer renderer;
  assert(!renderer.isReady());
  assert(!renderer.isVisible());
  assert(!renderer.beginText(
    1, 8, 16, 16, 0, 50, 1,
    255, 255, 255, false, 0, 0, 0, 0
  ));

  assert(renderer.begin(0x01));
  assert(renderer.isReady());
  assert(renderer.width() == 16 && renderer.height() == 16);
  assert(renderer.pixelCount() == 256);
  expectBlack(renderer.pixel(0, 0));

  assert(renderer.setPixel(15, 15, 0x12, 0x34, 0x56));
  const IDotMatrixRenderer::Pixel* pixel = renderer.pixel(15, 15);
  assert(pixel != nullptr);
  assert(pixel->red == 0x12 && pixel->green == 0x34 && pixel->blue == 0x56);
  assert(!renderer.setPixel(16, 15, 1, 2, 3));

  uint8_t rawImage[16 * 16 * 3]{};
  rawImage[0] = 0xA1;
  rawImage[1] = 0xB2;
  rawImage[2] = 0xC3;
  rawImage[sizeof(rawImage) - 3] = 4;
  rawImage[sizeof(rawImage) - 2] = 5;
  rawImage[sizeof(rawImage) - 1] = 6;
  assert(!renderer.beginRawImage(sizeof(rawImage) - 1));
  assert(renderer.beginRawImage(sizeof(rawImage)));
  assert(renderer.writeRawImage(0, rawImage, 400));
  assert(renderer.writeRawImage(400, rawImage + 400, sizeof(rawImage) - 400));
  assert(renderer.completeRawImage(true));
  pixel = renderer.pixel(0, 0);
  assert(pixel != nullptr);
  assert(pixel->red == 0xA1 && pixel->green == 0xB2 && pixel->blue == 0xC3);
  pixel = renderer.pixel(15, 15);
  assert(pixel != nullptr);
  assert(pixel->red == 4 && pixel->green == 5 && pixel->blue == 6);

  assert(renderer.beginRawImage(sizeof(rawImage)));
  const uint8_t replacement[] = {9, 9, 9};
  assert(renderer.writeRawImage(0, replacement, sizeof(replacement)));
  assert(!renderer.completeRawImage(false));
  pixel = renderer.pixel(0, 0);
  assert(pixel != nullptr);
  assert(pixel->red == 0xA1 && pixel->green == 0xB2 && pixel->blue == 0xC3);

  renderer.setVisible(true);
  assert(renderer.isVisible());
  renderer.clear();
  expectBlack(renderer.pixel(15, 15));

  assert(renderer.begin(0x03));
  assert(renderer.width() == 32 && renderer.height() == 32);
  assert(renderer.pixelCount() == 1024);
  assert(!renderer.isVisible());

  renderer.renderClock(23, 45, 2, 9, 3, true, false, 10, 20, 30, 0);
  assert(renderer.isVisible());
  // The 16x16 clock canvas is scaled 2x onto a 32x32 logical profile.
  const IDotMatrixRenderer::Pixel* clockPixel = renderer.pixel(12, 4);
  assert(clockPixel != nullptr);
  assert(clockPixel->red == 0 && clockPixel->green == 0 && clockPixel->blue == 0);
  const IDotMatrixRenderer::Pixel* clockBackground = renderer.pixel(0, 0);
  assert(clockBackground != nullptr);
  assert(clockBackground->red == 10 && clockBackground->green == 20 &&
    clockBackground->blue == 30);

  assert(renderer.begin(0x04));
  assert(renderer.width() == 64 && renderer.height() == 64);
  assert(renderer.pixelCount() == 4096);
  assert(renderer.setPixel(63, 63, 255, 1, 2));
  assert(!renderer.setPixel(64, 63, 255, 1, 2));
  for (uint8_t style = 0; style < 8; ++style) {
    renderer.renderClock(13, 27, 31, 12, style, false, false, 40, 50, 60, 1234);
    assert(renderer.isVisible());
    renderer.renderClock(13, 27, 31, 12, style, true, true, 40, 50, 60, 4321);
    assert(renderer.isVisible());
  }

  assert(renderer.beginText(
    1, 8, 16, 16, 0, 50, 1,
    12, 34, 56, false, 0, 0, 0, 1000
  ));
  uint8_t glyph8x16[16]{};
  glyph8x16[0] = 0x01;
  glyph8x16[15] = 0x80;
  assert(renderer.setTextGlyph(0, glyph8x16, sizeof(glyph8x16)));
  renderer.renderText(1000);
  const IDotMatrixRenderer::Pixel* textTop = renderer.pixel(0, 24);
  assert(textTop != nullptr);
  assert(textTop->red == 12 && textTop->green == 34 && textTop->blue == 56);
  const IDotMatrixRenderer::Pixel* textBottom = renderer.pixel(7, 39);
  assert(textBottom != nullptr);
  assert(textBottom->red == 12 && textBottom->green == 34 && textBottom->blue == 56);

  assert(renderer.beginText(
    1, 16, 32, 64, 0, 50, 1,
    90, 80, 70, true, 1, 2, 3, 2000
  ));
  uint8_t glyph16x32[64]{};
  glyph16x32[0] = 0x01;
  glyph16x32[63] = 0x80;
  assert(renderer.setTextGlyph(0, glyph16x32, sizeof(glyph16x32)));
  renderer.renderText(2000);
  const IDotMatrixRenderer::Pixel* text16 = renderer.pixel(0, 16);
  assert(text16 != nullptr);
  assert(text16->red == 90 && text16->green == 80 && text16->blue == 70);
  const IDotMatrixRenderer::Pixel* textBackground = renderer.pixel(20, 20);
  assert(textBackground != nullptr);
  assert(textBackground->red == 1 && textBackground->green == 2 &&
    textBackground->blue == 3);

  // Invalid profiles retain the reference implementation's 16x16 fallback.
  assert(renderer.begin(0xFF));
  assert(renderer.width() == 16 && renderer.height() == 16);

  // Text speed spans a deliberately wide 500..15 ms interval.  A left-moving
  // glyph starts just outside the right edge and enters only when its interval
  // has elapsed.
  assert(renderer.beginText(
    1, 8, 16, 16, 1, 0, 1,
    1, 2, 3, false, 0, 0, 0, 0
  ));
  assert(renderer.setTextGlyph(0, glyph8x16, sizeof(glyph8x16)));
  renderer.renderText(0);
  renderer.renderText(499);
  expectBlack(renderer.pixel(15, 0));
  renderer.renderText(500);
  pixel = renderer.pixel(15, 0);
  assert(pixel != nullptr && pixel->red == 1 && pixel->green == 2 && pixel->blue == 3);

  assert(renderer.beginText(
    1, 8, 16, 16, 1, 100, 1,
    4, 5, 6, false, 0, 0, 0, 0
  ));
  assert(renderer.setTextGlyph(0, glyph8x16, sizeof(glyph8x16)));
  renderer.renderText(0);
  renderer.renderText(14);
  expectBlack(renderer.pixel(15, 0));
  renderer.renderText(15);
  pixel = renderer.pixel(15, 0);
  assert(pixel != nullptr && pixel->red == 4 && pixel->green == 5 && pixel->blue == 6);
}
