#include <time.h>
#include <stdio.h>
#include "spinlock.hpp"

namespace base {

Spinlock::Spinlock() 
  : locked_(false) {
}

Spinlock::~Spinlock() {
}

void Spinlock::lock() { 
  struct timespec tim;
  struct timespec tim2;
  tim.tv_sec = 0;
  tim.tv_nsec = 1000;
  while (__sync_lock_test_and_set(&locked_, true)) {
    // try to read the position 10 times
    for(int i = 0;i < 10 ; i++){
      if(locked_==false) {
        if(!__sync_lock_test_and_set(&locked_,true)){
          return;
        }
        else break;
      }
      nanosleep(&tim, &tim2);
    }
    //
  }
}

void Spinlock::unlock() {
  __sync_lock_release(&locked_);
}

}  // namespace base
