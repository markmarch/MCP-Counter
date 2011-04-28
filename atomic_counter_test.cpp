#include <tr1/functional>
#include "unit_test.hpp"
#include "thread.hpp"
#include "atomic_counter.hpp"

namespace{
using std::tr1::bind;
using base::makeThread;
using base::Atomic_Counter;

const int REPEAT_TIME      = 1000;
const int MEGA_REPEAT_TIME = 10000000;
struct TestCombo{
	int repeat_time;
};



// the abstract class for test 
class AtomicTester{
public:
	AtomicTester(Atomic_Counter * atomic_counter);
	void start(TestCombo * tc_p);
	void join();
	virtual void test(TestCombo * tc_p) = 0;
protected:
	Atomic_Counter     * atomic_counter_;
	pthread_t            tid_;
};

class AtomicTestHelper{
public:
	void runner(Atomic_Counter * atomic_counter,
	            TestCombo      * tc_p,
	            int              thread_num)
	{
		AtomicTester_1 ** atomicTester = new AtomicTester_1 * [thread_num];
		for(int i = 0 ; i < thread_num; i++){
			atomicTester[i] = new AtomicTester_1(atomic_counter);
		}
		
		for(int i = 0 ; i < thread_num; i++){
			atomicTester[i]->start(tc_p);	
		}
		
		for(int i = 0 ; i < thread_num; i++){
			atomicTester[i]->join();
			delete atomicTester[i];	
		}
		delete []atomicTester;
	}
	static AtomicTestHelper& getInstance(){
		static AtomicTestHelper instance;
		return instance;
	}
	
private:
	AtomicTestHelper(){}
	AtomicTestHelper(AtomicTestHelper&);
	AtomicTestHelper& operator=(AtomicTestHelper&);
};

// a test class for simple test
class AtomicTester_1 : public AtomicTester{
	AtomicTester_1(Atomic_Counter * atomic_counter) : AtomicTester(atomic_counter){}
	void test(TestCombo * tc_p);  // overwrite the pure vitural function
};

AtomicTester::AtomicTester(Atomic_Counter * atomic_counter):
  atomic_counter_(atomic_counter){}

void AtomicTester::start(TestCombo * tc_p){
	tid_ = makeThread(std::tr1::bind(&AtomicTester::test, this, tc_p));
}

void AtomicTester_1::test(TestCombo * tc_p){
	for(int i = 0 ; i < tc_p->repeat_time; i++){
		atomic_counter->getAndIncrement();
	}
}

void AtomicTester::join(){
	pthread_join(tid_,NULL);
}


// all the tests 
TEST(Basics, Sequential){
	Atomic_Counter atomic_counter;
	TestCombo tc(REPEAT_TIME);
	AtomicTestHelper::getInstance().runner(&atomic_counter,1,&tc);
	EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME);
}


TEST(Basics, Concurrency2Thread){
	Atomic_Counter atomic_counter;
	TestCombo tc(REPEAT_TIME);
	AtomicTestHelper::getInstance().runner(&atomic_counter,2,&tc);
	EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME*2);
}


TEST(Basics, Concurrency4Thread){
	Atomic_Counter atomic_counter;
	TestCombo tc(REPEAT_TIME);
	AtomicTestHelper::getInstance().runner(&atomic_counter,4,&tc);
	EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME*4);
}
}// end of non-name namespace

int main(int argc, char *argv[]) {
  return RUN_TESTS();
}