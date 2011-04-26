#include "single_lock_counter.hpp"


namespace base{

Single_Lock_Counter():counter_(0){}

int Single_Lock_Counter::getAndIncrement(){
	ScopedLock l(&lock_);
	counter_++;
	return counter_;
}

int Single_Lock_Counter::getResult(){
	return counter_;
}

}// end of namespace base