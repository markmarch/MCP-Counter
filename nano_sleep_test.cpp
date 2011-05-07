#include <time.h>
#include <stdlib.h>
#include "ticks_clock.hpp"
static const struct timespec SLEEP_TIME={0,1000};
using base::TicksClock; 
int main(){
  TicksClock::Ticks start = TicksClock::getTicks(); 
  for(int i = 0 ; i < 100; i++){
    nanosleep(&SLEEP_TIME, NULL);
  }
  TicksClock::Ticks end   = TicksClock::getTicks(); 
  exit(0);
}

