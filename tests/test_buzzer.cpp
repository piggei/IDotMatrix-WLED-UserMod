#include "../IDotMatrixBuzzer.h"

#include <cassert>
#include <cstdint>

struct Sink {
  bool on = false;
  uint32_t transitions = 0;
};

static void output(void* context, bool on) {
  Sink* sink = static_cast<Sink*>(context);
  sink->on = on;
  ++sink->transitions;
}

int main() {
  Sink sink;
  IDotMatrixBuzzer buzzer;
  buzzer.attach(&output, &sink);

  assert(!buzzer.isPlaying());
  assert(!buzzer.outputOn());

  buzzer.startTrill(1000);
  assert(buzzer.isPlaying());
  assert(buzzer.outputOn());
  assert(sink.on);
  assert(sink.transitions == 1);

  buzzer.loop(1089);
  assert(sink.on && sink.transitions == 1);
  buzzer.loop(1090); // first pulse ends
  assert(!sink.on && sink.transitions == 2);
  buzzer.loop(1159);
  assert(!sink.on && sink.transitions == 2);
  buzzer.loop(1160); // second pulse begins
  assert(sink.on && sink.transitions == 3);
  buzzer.loop(1250); // second pulse ends
  assert(!sink.on && sink.transitions == 4);
  buzzer.loop(1320); // third pulse begins
  assert(sink.on && sink.transitions == 5);
  buzzer.loop(1410); // third pulse ends, long pause begins
  assert(!sink.on && sink.transitions == 6);
  buzzer.loop(1959);
  assert(!sink.on && sink.transitions == 6);
  buzzer.loop(1960); // next trill begins
  assert(sink.on && sink.transitions == 7);

  buzzer.stop();
  assert(!buzzer.isPlaying());
  assert(!buzzer.outputOn());
  assert(!sink.on && sink.transitions == 8);

  // stop() is idempotent and does not emit redundant GPIO transitions.
  buzzer.stop();
  assert(sink.transitions == 8);

  // The configuration-page test is a single trill only. It must stop after
  // three pulses instead of repeating indefinitely.
  buzzer.startTest(3000);
  assert(buzzer.isPlaying());
  assert(sink.on && sink.transitions == 9);
  buzzer.loop(3090);
  assert(!sink.on && sink.transitions == 10);
  buzzer.loop(3160);
  assert(sink.on && sink.transitions == 11);
  buzzer.loop(3250);
  assert(!sink.on && sink.transitions == 12);
  buzzer.loop(3320);
  assert(sink.on && sink.transitions == 13);
  buzzer.loop(3410);
  assert(!buzzer.isPlaying());
  assert(!sink.on && sink.transitions == 14);
  buzzer.loop(5000);
  assert(!sink.on && sink.transitions == 14);

  buzzer.stop();
  assert(sink.transitions == 14);

  // A schedule notification is exactly three groups of three pulses.  It must
  // not keep sounding for the full duration of the schedule activity.
  buzzer.startScheduleAlert(6000);
  assert(buzzer.isPlaying());
  assert(sink.on && sink.transitions == 15);
  uint32_t t = 6000;
  for (uint8_t group = 0; group < 3; ++group) {
    for (uint8_t pulse = 0; pulse < 3; ++pulse) {
      t += 90;
      buzzer.loop(t); // pulse ends
      assert(!sink.on);
      if (group == 2 && pulse == 2) break;
      if (pulse < 2) t += 70;
      else t += 550;
      buzzer.loop(t); // next pulse/group begins
      assert(sink.on);
    }
  }
  assert(!buzzer.isPlaying());
  assert(!sink.on);
  assert(sink.transitions == 32); // 9 ON + 9 OFF after the previous 14 transitions
  buzzer.loop(t + 5000);
  assert(sink.transitions == 32);

  return 0;
}
