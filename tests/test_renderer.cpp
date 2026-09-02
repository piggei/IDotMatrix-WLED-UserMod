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
  assert(renderer.acceptedPixelUpdates() == 1);

  renderer.setVisible(true);
  assert(renderer.isVisible());
  renderer.clear();
  expectBlack(renderer.pixel(15, 15));

  assert(renderer.begin(0x03));
  assert(renderer.width() == 32 && renderer.height() == 32);
  assert(renderer.pixelCount() == 1024);
  assert(!renderer.isVisible());

  assert(renderer.begin(0x04));
  assert(renderer.width() == 64 && renderer.height() == 64);
  assert(renderer.pixelCount() == 4096);
  assert(renderer.setPixel(63, 63, 255, 1, 2));
  assert(!renderer.setPixel(64, 63, 255, 1, 2));

  // Invalid profiles retain the reference implementation's 16x16 fallback.
  assert(renderer.begin(0xFF));
  assert(renderer.width() == 16 && renderer.height() == 16);
}
