#include "atomic_counter.hpp"

using namespace base;

Atomic_Counter::Atomic_Counter():counter(0){}

int Atomic_Counter::getAndIncrement(){
	return __sync_fetch_and_add(&counter,1);
}

int Atomic_Counter::getResult(){
	return counter;
}
