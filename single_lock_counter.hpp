#ifndef MCP_BASE_SINGLE_LOCK_COUNTER_HEADER
#define MCP_BASE_SINGLE_LOCK_COUNTER_HEADER
#include "lock.hpp"
namespace base{
class Single_Lock_Counter{
public:
	Single_Lock_Counter();
	int getAndIncrement();
	int getResult();
private:
	Mutex     lock_;
	int       counter_;
	Single_Lock_Counter(Single_Lock_Counter&);
	Single_Lock_Counter& operator=(Single_Lock_Counter&);
};
}

#endif