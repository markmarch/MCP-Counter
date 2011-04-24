#ifndef MCP_BASE_STAT_COUNTER_HEADER
#define MCP_BASE_STAT_COUNTER_HEADER
#include <map>
#include <vector>
#include <pthread.h>
#include "spinlock.hpp"
/*
** Taolun Chai
*/
namespace base{
using std::map;
using std::vector;
class Stat_Counter{
public:
	int          read_count();              // read total count;
	void         inc_count();
	bool         count_register_thread();   // register a thread
	bool         count_unregister_thread(); // unregister thread
	Stat_Counter(int max_thread_num);       // use defined max thread_num
	Stat_Counter();                         // give the default max thread_num is 16
private:
	__thread int counter_;                  // count for each thread
	int 				 max_thread_num_;           // max connected thread
	int **       counter_p_;                // pointer point to all thread counter
	int          final_count_;              // the sum for this counter
	Spinlock     spinlock_;                 // lock the Stat_counter for register or unregister 
	map<int,int> pid_map_index_;	          // map for pid and it coresponding index on counter_p_
	vector<int>  index_free_;               // vector stores all the free counter_p_ index
	
	//vector<int>  index_busy_;
	Stat_Counter(Stat_Counter&);            // non copy constructable
	Stat_Counter& operator=(Stat_Counter&); // non copyable
};

}
#endif
