#include <tr1/functional>
#include "unit_test.hpp"
#include "thread.hpp"
#include "stat_counter.hpp"

namespace{
using std::tr1::bind;
using base::makeThread;
using base::Stat_Counter;

const int REPEAT_TIME      = 1000;
const int MEGA_REPEAT_TIME = 10000000;

struct TestCombo{
	int thread_id;
	int repeat_time;
	TestCombo(int thread_id, int repeat_time):
	  thread_id(thread_id),
	  repeat_time(repeat_time){}
};

class StatTester{
public:
	StatTester(Stat_Counter * stat_counter);
	void start(int repeat_time);
	void join();
	virtual void test(int repeat_time)  = 0;
protected:
	Stat_Counter    * stat_counter;
	pthread_t         tid_;
};

class StatTester_1 : public StatTester{
public: 
	void test(int repeat_time);           // overwrite the pure vitural function in Stat_Test
};

class StatTestHelper{
public:
	void runner(Stat_Counter * stat_counter,
				      int            max_thread_num,
				      int            repeat_time
				     );
	static StatTestHelper& getInstance(){
		static StatTestHelper instance;
		return instance;
	}
private:
	StatTestHelper(){}
	StatTestHelper(StatTestHelper&);
	StatTestHelper& operator=(StatTestHelper&);
};

void StatTestHelper::runner(Stat_Counter * stat_counter,
	                          int            thread_num,
											      int            repeat_time)
{
	StatTester_1  ** statTester  = new StatTester_1* [thread_num];
	for(int i = 0; i < thread_num; i++){
		statTester[i] = new statTester(stat_counter);
	}
	for(int i = 0; i < thread_num; i++){
		statTester[i]->start();
	}
	for(int i = 0; i < thread_num; i++){
		statTester[i]->join();
		delete statTester[i];
	}
	delete []statTester;
}

StatTester::StatTester(Stat_Counter * stat_counter):
  stat_counter(stat_counter){}

void StatTester::start(int repeat_time){
	tid_ = makeThread(std::tr1::bind(&StatTester::test,this, repeat_time));
}

void StatTester::join(){
	pthread_join(tid_, NULL);
}


void StatTester_1::test(int repeat_time){
	stat_counter.count_register_thread();
	for(int i = 0 ; i < repeat_time; i++){
		stat_counter.inc_count();
	}
	stat_counter.unregister_thread();
} 

// all the tests 
TEST(Basics, Sequential){
	Stat_Counter stat_counter;
	StatTestHelper::getInstance().runner(&stat_counter,1,REPEAT_TIME);
	EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME);
}

} // end of non named namespace


int main(int argc, char *argv[]) {
  return RUN_TESTS();
}

