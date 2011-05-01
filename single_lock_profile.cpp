#include "single_lock_counter.hpp"
#include "ticks_clock.hpp"

#include <tr1/functional>
#include <pthread.h>
#include <stdlib.h>

namespace{

using std::tr1::bind;
using base::Single_Lock_Counter;
using base::TicksClock;

const int REPEAT_TIME      = 1000;
const int MEGA_REPEAT_TIME = 1000000;

static double getTotal(TicksClock::Ticks start, 
    TicksClock::Ticks end) {
  double duration = end - start;
  double time = duration / TicksClock::ticksPerSecond();
  return time;
}

Single_Lock_Counter *counter;

int thread_num;

pthread_t *threads;


void *test(void *thread_id){
  //long tid = (long) thread_id;
  TicksClock::Ticks start = TicksClock::getTicks();
  for(int i = 0 ; i < MEGA_REPEAT_TIME; i++){
    counter->getAndIncrement();
  }
  TicksClock::Ticks end = TicksClock::getTicks();
  double duration = getTotal(start,end)*1e9/MEGA_REPEAT_TIME;
  printf("average:%f\n",duration);

  pthread_exit((void*) thread_id);
}


} // end of namespace


int main(int argc, char *argv[]){
  if(argc < 2){
    thread_num = 8;
  }
  else thread_num = atoi(argv[1]);
  counter    = new Single_Lock_Counter();
  threads = new pthread_t[thread_num];
  int rc;
  for(int i = 0 ; i < thread_num; i++){
    rc = pthread_create(&threads[i],NULL, test, (void *)i);
    if (rc) {
      printf("ERROR; ");
      exit(-1);
    }
  }
  for(int i = 0 ; i < thread_num; i++){
    pthread_join(threads[i], NULL);
  }
  printf("%d\n", counter->getResult());
  delete []threads;
  delete counter;
}

