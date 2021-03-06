#include "tree_node.hpp"
#include <stdlib.h>
#include <iostream>
using namespace base;
Node::Node():
  locked_(false),
  cStatus_(ROOT),
  firstValue_(0),
  secondValue_(0),
  result_(0),
  parent_(NULL)
  {
    //padding_ = new char [pad_size];
  }
Node::Node(Node * myParent):
  locked_(false),
  cStatus_(IDLE),
  firstValue_(0),
  secondValue_(0),
  result_(0),
  parent_(myParent)
  {
    //padding_ = new char [pad_size];
  }
Node * Node::getParent(){
  return this->parent_;
}
bool Node::pre_combine(){
  ScopedLock l(&node_lock_);
  //std::cout << "pre-combine" << std::endl;
  //int count  = 0;
  while(this->locked_) {
    cond_var_.wait(&node_lock_);
  }
  switch(this->cStatus_){
    case IDLE:
      cStatus_ = FIRST;
      return true;
    case FIRST:
      //__sync_fetch_and_or(&this->locked_, true);
      locked_  = true;
      cStatus_ = SECOND;
      return false;
    case ROOT:
      return false;
    default:
      std::cerr << "error! unexpected Node state i pre-combine" << std::endl;
      ::exit(1);
  }
}

int Node::combine(int combined){
  ScopedLock l(&node_lock_);
  //std::cout << "combine" << std::endl;
  while(this->locked_){
    cond_var_.wait(&node_lock_);
  }
  
  //__sync_fetch_and_or(&this->locked_, true);
  this->firstValue_  = combined;
  this->locked_  = true;
  switch(this->cStatus_){
    case FIRST:
      return  this->firstValue_;
    case SECOND:
      return  this->firstValue_ + this->secondValue_;
    default:
      std::cerr << "error! unexpected Node stat in combine " << std::endl;
      exit(1);
  }
}

int Node::op(int combined){
  ScopedLock l(&node_lock_);
  
  int oldValue;
  switch (this->cStatus_) {
    case ROOT:
      oldValue              = this->result_;
      this->result_        += combined;
      return oldValue;
    case SECOND:
      this->secondValue_    = combined;
      // is there really need to be atomic operation?
      //__sync_fetch_and_and(&this->locked_, false);
      this->locked_         = false;
      cond_var_.signalAll();
      while (this->cStatus_ != RESULT){
        //std::cout << "wait on op" << std::endl;
        cond_var_.wait(&node_lock_);
        //std::cout << "wake on op" << std::endl;
      }
      locked_               = false;
      // same operation like before
      //__sync_fetch_and_and(&this->locked_, false);
     
      cond_var_.signalAll();
      this->cStatus_        = IDLE;
      return this->result_;
    default:
      std::cerr << "error in op " << std::endl;
      ::exit(1);
  }
}

void Node::distribute(int prior){
  ScopedLock l(&node_lock_);
  
  switch (this->cStatus_) {
    case FIRST:
      this->cStatus_        = IDLE;
      // 
      //__sync_fetch_and_and(&this->locked_, false);
      this->locked_         = false;
      break;
    case SECOND:
      this->result_         = prior + this->firstValue_;
      this->cStatus_        = RESULT;
      break;
    default:
      std::cerr << "error in distribute" << std::endl;
      ::exit(1);
  }
  this->cond_var_.signalAll();
}



