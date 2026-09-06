#include "../IDotMatrixCompactGif.h"
#include "../IDotMatrixRenderer.h"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <vector>

struct MemoryInput {
  std::vector<uint8_t> data;
  size_t pos = 0;
};

static int32_t readMemory(void* context, uint8_t* buffer, size_t length) {
  MemoryInput* in = static_cast<MemoryInput*>(context);
  if (!in || !buffer || in->pos >= in->data.size()) return 0;
  size_t remaining = in->data.size() - in->pos;
  if (length > remaining) length = remaining;
  for (size_t i = 0; i < length; ++i) buffer[i] = in->data[in->pos + i];
  in->pos += length;
  return int32_t(length);
}

static bool seekMemory(void* context, uint32_t position) {
  MemoryInput* in = static_cast<MemoryInput*>(context);
  if (!in || position > in->data.size()) return false;
  in->pos = position;
  return true;
}

int main() {
  std::ifstream file("tests/compact_test.gif", std::ios::binary);
  assert(file.good());
  MemoryInput in;
  in.data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

  IDotMatrixRenderer renderer;
  assert(renderer.begin(0x04, 16, 16));
  assert(renderer.beginAnimation());

  IDotMatrixCompactGif decoder;
  assert(decoder.open(
    &in, readMemory, seekMemory, 64, 64, 16, 16, renderer.animationFrameBytes()
  ));
  assert(decoder.workspaceBytes() == IDotMatrixCompactGif::requiredWorkspaceBytes(768));

  uint16_t delay = 0;
  assert(decoder.decodeNextFrame(renderer, delay) == IDotMatrixCompactGif::Result::Frame);
  assert(delay == 50);
  const auto* first = renderer.pixel(0, 0);
  assert(first && first->red > 240 && first->green < 10 && first->blue < 10);

  assert(decoder.decodeNextFrame(renderer, delay) == IDotMatrixCompactGif::Result::Frame);
  assert(delay == 70);
  const auto* second = renderer.pixel(0, 0);
  assert(second && second->green > 240 && second->red < 10 && second->blue < 10);

  assert(decoder.decodeNextFrame(renderer, delay) == IDotMatrixCompactGif::Result::End);
  decoder.close();

  std::ifstream stressFile("tests/compact_stress.gif", std::ios::binary);
  assert(stressFile.good());
  MemoryInput stress;
  stress.data.assign(std::istreambuf_iterator<char>(stressFile), std::istreambuf_iterator<char>());
  assert(decoder.open(
    &stress, readMemory, seekMemory, 64, 64, 16, 16, renderer.animationFrameBytes()
  ));
  assert(decoder.decodeNextFrame(renderer, delay) == IDotMatrixCompactGif::Result::Frame);
  assert(delay == 40);
  const auto* stress0 = renderer.pixel(3, 5);
  assert(stress0 != nullptr);
  const uint8_t sx0 = uint8_t(3u * 64u / 16u);
  const uint8_t sy0 = uint8_t(5u * 64u / 16u);
  const uint8_t expected0 = uint8_t((uint16_t(sx0) * 37u + uint16_t(sy0) * 73u + uint16_t(sx0) * sy0 * 13u) & 255u);
  assert(stress0->red >= expected0 - (expected0 > 7 ? 7 : 0));
  assert(stress0->red <= uint8_t(expected0 + (expected0 < 248 ? 7 : 0)));
  assert(decoder.decodeNextFrame(renderer, delay) == IDotMatrixCompactGif::Result::Frame);
  assert(delay == 60);
  const auto* stress1 = renderer.pixel(11, 9);
  assert(stress1 != nullptr);
  const uint8_t sx1 = uint8_t(11u * 64u / 16u);
  const uint8_t sy1 = uint8_t(9u * 64u / 16u);
  const uint8_t expected1 = uint8_t((uint16_t(sx1) * 37u + uint16_t(sy1) * 73u + 91u + uint16_t(sx1) * sy1 * 13u) & 255u);
  assert(stress1->green >= expected1 - (expected1 > 7 ? 7 : 0));
  assert(stress1->green <= uint8_t(expected1 + (expected1 < 248 ? 7 : 0)));
  assert(decoder.decodeNextFrame(renderer, delay) == IDotMatrixCompactGif::Result::End);
  decoder.close();
}

