#include <iostream>
#include <stdlib.h>
#include "stat_counter.hpp"
/*
** Taolun Chai
*/
namespace base{
Stat_Counter::Stat_Counter():
  max_thread_num_(16),
  final_count_(0)
{
	counter_p_ = new int* [max_thread_num_]; 
	for(int i = 0; i < max_thread_num_; i++){
		index_free_.push_back(i);
	}
}
Stat_Counter::Stat_Counter(int max_thread_num):
  max_thread_num_(max_thread_num),
  final_count_(0)
{
	counter_p_ = new int* [max_thread_num_];
	for(int i = 0; i < max_thread_num_; i++){
		index_free_.push_back(i);
	}
}
void Stat_Counter::inc_count(){
	counter_++;
}

__thread int Stat_Counter::counter_;

int Stat_Counter::read_count(){
	int sum;
	this->spinlock_.lock();
	sum = final_count_;
	for(int i = 0 ; i < max_thread_num_; i++){
		if(counter_p_[i]) sum += *counter_p_[i];
	}
	this->spinlock_.unlock();
	return sum;
}

bool Stat_Counter::count_register_thread(){
	this->spinlock_.lock();
	if(index_free_.empty()){
		this->spinlock_.unlock();
		return false;     // no free space available
	} 
	// check whether the thread been register
	if(pid_map_index_.find(pthread_self())!=pid_map_index_.end()){
		std::cerr << "error! duplicate register" << std::endl;
		exit(1);
	}
	// do the register
	pid_map_index_[pthread_self()] = index_free_.back();
	counter_p_[index_free_.back()] = &counter_;
	counter_ = 0;
  index_free_.pop_back();
	this->spinlock_.unlock();
	return true;
}
bool Stat_Counter::count_unregister_thread(){
	this->spinlock_.lock();
	map<int,int>::iterator it = pid_map_index_.find(pthread_self());
	// not found this thread!!
	if(it==pid_map_index_.end()){
		std::cerr << "error! thread not registered!" << std::endl;
		exit(1);
	}
	final_count_ += counter_;
	counter_p_[it->second] = NULL;
	index_free_.push_back(it->second);
	pid_map_index_.erase(it);
	this->spinlock_.unlock();
	return true;
}
}
