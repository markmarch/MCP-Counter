#ifndef MCP_BASE_ATOMIC_COUNTER_HEADER
#define MCP_BASE_ATOMIC_COUNTER_HEADER
namespace base{

class Atomic_Counter{
public:
	Atomic_Counter();
	int getAndIncrement();
	int getResult();
private:
	int counter;
	Atomic_Counter(Atomic_Counter&);
	Atomic_Counter& operator=(Atomic_Counter&);
};

}// end of base
#endif