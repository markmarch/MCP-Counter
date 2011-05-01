#include "stat_counter.hpp"
#include "ticks_clock.hpp"

#include <tr1/functional>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
namespace{

using std::tr1::bind;
using base::Stat_Counter;
using base::TicksClock;
const int MEGA_REPEAT_TIME = 1000000;

static double getTotal(TicksClock::Ticks start, 
    TicksClock::Ticks end) {
  double duration = end - start;
  double time = duration / TicksClock::ticksPerSecond();
  return time;
}

Stat_Counter *counter;

int thread_num;

pthread_t *threads;


void *test(void * thread_id){
  //long tid = (long) thread_id;
  counter->count_register_thread();
  TicksClock::Ticks start = TicksClock::getTicks();
  for(int i = 0 ; i < MEGA_REPEAT_TIME; i++){
    counter->inc_count();
  }
  TicksClock::Ticks end = TicksClock::getTicks();
  double duration = getTotal(start,end)*1e9/MEGA_REPEAT_TIME;
  
  counter->count_unregister_thread();
  printf("average:%f\n",duration);

  pthread_exit((void*) thread_id);
}


} // end of namespace


int main(int argc, char *argv[]){
  if(argc < 2){
    thread_num = 8;
  }
  else thread_num = atoi(argv[1]);
  counter    = new Stat_Counter(thread_num);
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
  printf("%d\n", counter->read_count());
  delete []threads;
  delete counter;
}

