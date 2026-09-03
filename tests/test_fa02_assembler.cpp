#include "../IDotMatrixFA02Assembler.h"

#include <cassert>
#include <cstring>

int main() {
  IDotMatrixFA02Assembler assembler;

  const uint8_t complete[] = {0x05, 0x00, 0x07, 0x01, 0x01};
  assert(assembler.append(complete, sizeof(complete)) ==
    IDotMatrixFA02Assembler::Result::Complete);
  assert(assembler.complete());
  assert(assembler.expected() == sizeof(complete));
  assert(memcmp(assembler.data(), complete, sizeof(complete)) == 0);
  assert(assembler.append(complete, sizeof(complete)) ==
    IDotMatrixFA02Assembler::Result::Busy);

  assembler.reset();
  assert(!assembler.complete() && assembler.expected() == 0);
  assert(assembler.append(complete, 2) ==
    IDotMatrixFA02Assembler::Result::Accumulating);
  assert(assembler.expected() == 5 && assembler.received() == 2);
  assert(assembler.append(complete + 2, 1) ==
    IDotMatrixFA02Assembler::Result::Accumulating);
  assert(assembler.append(complete + 3, 2) ==
    IDotMatrixFA02Assembler::Result::Complete);
  assert(memcmp(assembler.data(), complete, sizeof(complete)) == 0);

  assembler.reset();
  const uint8_t tooLarge[] = {0xFF, 0x7F};
  assert(assembler.append(tooLarge, sizeof(tooLarge)) ==
    IDotMatrixFA02Assembler::Result::Invalid);
  assert(assembler.expected() == 0);

  const uint8_t tooShort[] = {0x05};
  assert(assembler.append(tooShort, sizeof(tooShort)) ==
    IDotMatrixFA02Assembler::Result::Invalid);
}
