#include "tree_node.hpp"
#include <stdlib.h>
#include <iostream>
using namespace base;
Node::Node():
  lock_(Node::UNOCCUPIED_AND_UNLOCKED),
  cStatus_(ROOT),
  firstValue_(0),
  secondValue_(0),
  result_(0),
  parent_(NULL){}

Node::Node(Node * myParent):
  lock_(Node::UNOCCUPIED_AND_UNLOCKED),
  cStatus_(IDLE),
  firstValue_(0),
  secondValue_(0),
  result_(0),
  parent_(myParent){}

Node * Node::getParent(){
  return this->parent_;
}

bool Node::pre_combine(){
  // int k=0;
  while(!(__sync_bool_compare_and_swap(&lock_,UNOCCUPIED_AND_UNLOCKED,
    OCCUPIED_AND_UNLOCKED))){
    // spin on local cache before try again
    for(int i=0;i<LOOP_TIME&&lock_!=UNOCCUPIED_AND_UNLOCKED;i++);
    // nanosleep(&SLEEP_TIME,NULL);
  }
  // printf("%d ",k);
  
  switch(this->cStatus_){
    case IDLE:
      cStatus_ = FIRST;
      __sync_fetch_and_sub(&lock_,2); // set lock_ to unoccupied&unlocked
      return true;
    case FIRST:
      cStatus_ = SECOND;
      __sync_fetch_and_sub(&lock_,1); // set lock_ to unoccupied&locked
      return false;
    case ROOT:
      __sync_fetch_and_sub(&lock_,1); // set lock_ to unoccupied&locked
      return false;
    default:
      std::cerr << "error! unexpected Node state i pre-combine" << std::endl;
      ::exit(1);
  }
}

int Node::combine(int combined){
  // set lock_ to occupied&locked
  while(!(__sync_bool_compare_and_swap(&lock_,UNOCCUPIED_AND_UNLOCKED,
    OCCUPIED_AND_LOCKED))){
    // spin on local cache before try again
    for(int i=0;i<LOOP_TIME&&lock_>UNOCCUPIED_AND_UNLOCKED;i++);
    // nanosleep(&SLEEP_TIME,NULL);
  }
  
  this->firstValue_  = combined;
  switch(this->cStatus_){
    case FIRST:
      __sync_fetch_and_sub(&lock_,2); // set lock_ to unoccupied&locked
      return  this->firstValue_;
    case SECOND:
      __sync_fetch_and_sub(&lock_,2); // set lock_ to unoccupied&locked
      return  this->firstValue_ + this->secondValue_;
    default:
      std::cerr << "error! unexpected Node stat in combine " << std::endl;
      exit(1);
  }
}

int Node::op(int combined){
  // set lock_ to occupied&locked
  while(!(__sync_bool_compare_and_swap(&lock_,UNOCCUPIED_AND_LOCKED,
    OCCUPIED_AND_LOCKED))){
    // spin on local cache before try again
    for(int i=0;i<LOOP_TIME&&lock_!=UNOCCUPIED_AND_LOCKED;i++);
    // nanosleep(&SLEEP_TIME,NULL);
  }

  int oldValue;
  int result;
  switch (this->cStatus_) {
    case ROOT:
      oldValue              = this->result_;
      this->result_        += combined;
      __sync_fetch_and_sub(&lock_,3); // set lock_ to unoccupied&unlocked
      return oldValue;
    case SECOND:
      this->secondValue_    = combined;
      __sync_fetch_and_sub(&lock_,3); // set lock_ to unoccupied&unlocked
      
      // spin until cStatus changes to RESULT
      
      while(!__sync_bool_compare_and_swap(&lock_,RESULT_READY,OCCUPIED_AND_UNLOCKED)){
         for(int i=0;i<LOOP_TIME&&lock_!=RESULT_READY;i++);
         // nanosleep(&SLEEP_TIME,NULL);
      }
      this->cStatus_        = IDLE;
      result  = result_;
      __sync_fetch_and_sub(&lock_,2); // set lock_ to unoccupied&unlocked
      return result;
    default:
      std::cerr << "error in op " << std::endl;
      ::exit(1);
  }
}

void Node::distribute(int prior){
  while(!__sync_bool_compare_and_swap(&lock_,UNOCCUPIED_AND_LOCKED,OCCUPIED_AND_LOCKED)){
    for(int i=0;i<LOOP_TIME&&lock_==OCCUPIED_AND_LOCKED;i++);
    // nanosleep(&SLEEP_TIME,NULL);
  }
  switch (this->cStatus_) {
    case FIRST:
      this->cStatus_        = IDLE;
      __sync_fetch_and_sub(&lock_,3); // set lock_ to unoccupied and unlocked
      break;
    case SECOND:
      this->result_         = prior + this->firstValue_;
      this->cStatus_        = RESULT;
      __sync_fetch_and_add(&lock_,1); // set lock_ to result ready
      break;
    default:
      std::cerr << "error in distribute" << std::endl;
      ::exit(1);
  }
}



