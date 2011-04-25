#include "ticks_clock.hpp"

namespace base {

static double ticks_per_micro_second = 0;

void moduleInit() {
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 500; // 500 ns
  TicksClock::Ticks before = TicksClock::getTicks();
  nanosleep(&ts, NULL);
  TicksClock::Ticks after = TicksClock::getTicks();
  ticks_per_micro_second = (after - before) * 2;
}

pthread_once_t TicksClock::once_control = PTHREAD_ONCE_INIT;
long ticks_per_micro_second_ = 0;

double TicksClock::ticksPerMicroSecond() {
  pthread_once(&once_control, moduleInit);
  return ticks_per_micro_second;
}

}  // namespace base
