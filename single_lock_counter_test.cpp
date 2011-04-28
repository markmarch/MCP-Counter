#include <tr1/functional>
#include "unit_test.hpp"
#include "thread.hpp"
#include "single_lock_counter.hpp"
namespace{
using std::tr1::bind;
using base::makeThread;
using base::Single_Lock_Counter;

const int REPEAT_TIME      = 1000;
const int MEGA_REPEAT_TIME = 10000000;

struct TestCombo{
	int repeat_time;
  TestCombo(int repeat_time): repeat_time(repeat_time){}
};

// the abstract class for test
class SingleLockTester{
public:
	SingleLockTester(Single_Lock_Counter * counter);
	void start(TestCombo * tc_p);
	void join();
	virtual void test(TestCombo * tc_p) = 0;
protected:
	Single_Lock_Counter     * counter_;
	pthread_t                 tid_;
};

class SingleLockTester_1 : public SingleLockTester{
public:
  SingleLockTester_1(Single_Lock_Counter * counter) : SingleLockTester(counter){}
  void test(TestCombo * tc_p);  // override the virtual function.
};

SingleLockTester::SingleLockTester(Single_Lock_Counter * counter):
  counter_(counter){}
void SingleLockTester::start(TestCombo * tc_p){
	tid_ = makeThread(std::tr1::bind(&SingleLockTester::test, this, tc_p));
}

void SingleLockTester::join(){
	pthread_join(tid_, NULL);
}

void SingleLockTester_1::test(TestCombo * tc_p){
	for(int i = 0; i < tc_p->repeat_time; i++){
		counter_->getAndIncrement();
	}
}

class SingleLockTestHelper{
public:
	void runner(Single_Lock_Counter * single_lock_counter,
	            int                   thread_num,
              TestCombo           * tc_p
	            )
	{
		SingleLockTester_1 ** singleLockTester = new SingleLockTester_1 * [thread_num];
		for(int i = 0 ; i < thread_num; i++){
			singleLockTester[i] = new SingleLockTester_1(single_lock_counter);
		}
		
		for(int i = 0 ; i < thread_num; i++){
			singleLockTester[i]->start(tc_p);	
		}
		
		for(int i = 0 ; i < thread_num; i++){
			singleLockTester[i]->join();
			delete singleLockTester[i];	
		}
		delete []singleLockTester;
	}
	static SingleLockTestHelper& getInstance(){
		static SingleLockTestHelper instance;
		return instance;
	}
private:
	SingleLockTestHelper(){}
	SingleLockTestHelper(SingleLockTestHelper &);
	SingleLockTestHelper& operator=(SingleLockTestHelper&);
};

// all the test
TEST(Basics, Sequential){
	Single_Lock_Counter single_lock_counter;
	TestCombo tc(REPEAT_TIME);
  SingleLockTestHelper::getInstance().runner(&single_lock_counter,1,&tc);
	EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME);
}


TEST(Basics, Concurrency2Thread){
	Single_Lock_Counter single_lock_counter;
	TestCombo tc(REPEAT_TIME);
	SingleLockTestHelper::getInstance().runner(&single_lock_counter,2,&tc);
	EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME*2);
}


TEST(Basics, Concurrency4Thread){
	Single_Lock_Counter single_lock_counter;
	TestCombo tc(REPEAT_TIME);
	SingleLockTestHelper::getInstance().runner(&single_lock_counter,4,&tc);
	EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME*4);
}

}// end of non-name namespace

int main(int argc, char *argv[]) {
  return RUN_TESTS();
}
