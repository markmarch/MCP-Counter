#include <tr1/functional>
#include <pthread.h>
#include "combining_tree.hpp"
#include <stdlib.h>
namespace {
using std::tr1::bind;
using base::Combining_Tree;
const int REPEAT_TIME      = 1000;
const int MEGA_REPEAT_TIME = 1000000;

struct TestCombo{
  int thread_id;
};

Combining_Tree *tree;

int thread_num;
pthread_t *threads;


void *test(void *thread_id){
  long tid = (long) thread_id;
  for(int i = 0 ; i < MEGA_REPEAT_TIME; i++){
    tree->getAndIncrement(tid);
  }
  pthread_exit((void*) thread_id);
}
}

int main(int argc, char *argv[]){
  if(argc < 2){
    thread_num = 8;
  }
  else thread_num = atoi(argv[1]);
  tree    = new Combining_Tree(thread_num-1);
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
  printf("%d\n", tree->getResult());
  delete []threads;
  delete tree;
}

