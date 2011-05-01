#include <tr1/functional>

#include "unit_test.hpp"
#include "thread.hpp"
#include "single_lock_counter.hpp"
#include "ticks_clock.hpp"

namespace{

  using std::tr1::bind;
  using base::makeThread;
  using base::Single_Lock_Counter;
  using base::TicksClock;

//  const int REPEAT_TIME      = 1000;
//  1 million
    const int REPEAT_TIME    = 100000;
//  const int MEGA_REPEAT_TIME = 10000000;

  struct TestCombo{
    int repeat_time;
    double update_total;  // total update per thread
    double read_total;    // total read per thread
    TestCombo(int repeat_time): repeat_time(repeat_time),
    update_total(0), read_total(0){ }
  };

  static double getTotal(TicksClock::Ticks start,
      TicksClock::Ticks end) {
    double duration = end - start;
    double time = duration / TicksClock::ticksPerSecond();
    return time;
  }

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
  SingleLockTester::SingleLockTester(Single_Lock_Counter * counter):
    counter_(counter){}
  void SingleLockTester::start(TestCombo * tc_p){
    tid_ = makeThread(std::tr1::bind(&SingleLockTester::test, this, tc_p));
  }

  void SingleLockTester::join(){
    pthread_join(tid_, NULL);
  }

  // derived class for update testcase
  class SingleLockTester_1 : public SingleLockTester{
    public:
      SingleLockTester_1(Single_Lock_Counter * counter) : SingleLockTester(counter){}
      void test(TestCombo * tc_p);  // override the virtual function.
  };
  void SingleLockTester_1::test(TestCombo * tc_p){
    TicksClock::Ticks start = TicksClock::getTicks();
    for(int i = 0; i < tc_p->repeat_time; i++){
      counter_->getAndIncrement();
    }
    TicksClock::Ticks end = TicksClock::getTicks();
    tc_p->update_total = getTotal(start, end);
//    std::cout << "update time: " << tc_p->update_total << std::endl;
  }
  // derived class for read testcase
  class SingleLockTester_2 : public SingleLockTester {
    public:
      SingleLockTester_2(Single_Lock_Counter *counter) : SingleLockTester(counter) {}
      void test(TestCombo *tc_p);
  };
  void SingleLockTester_2::test(TestCombo *tc_p){
    TicksClock::Ticks start = TicksClock::getTicks();
    for(int i = 0; i < tc_p->repeat_time; i++){
      counter_->getResult();
    }
    TicksClock::Ticks end = TicksClock::getTicks();
    tc_p->read_total = getTotal(start, end);
//    std::cout << "read time: " << tc_p->read_total;
  }

  // helper class
  class SingleLockTestHelper{
    public:
      void runner(Single_Lock_Counter * single_lock_counter,
          int                   thread_num,
          int                   repeat_time);
      void runner2(Single_Lock_Counter * single_lock_counter,
          int                   thread_num,
          int                   repeat_time);
      void runner3(Single_Lock_Counter * single_lock_counter,
          int                   thread_num,
          int                   repeat_time);

      static SingleLockTestHelper& getInstance(){
        static SingleLockTestHelper instance;
        return instance;
      }

    private:
      SingleLockTestHelper(){}
      SingleLockTestHelper(SingleLockTestHelper &);
      SingleLockTestHelper& operator=(SingleLockTestHelper&);
  };

  // update only testcases
  void SingleLockTestHelper::runner(Single_Lock_Counter * single_lock_counter,
      int                   thread_num,
      int                   repeat_time
      )
  {
    double update_sum = 0;
    SingleLockTester_1 ** singleLockTester = new SingleLockTester_1 * [thread_num];
    TestCombo **testCombo = new TestCombo *[thread_num];
    for(int i = 0 ; i < thread_num; i++){
      singleLockTester[i] = new SingleLockTester_1(single_lock_counter);
      testCombo[i] = new TestCombo(repeat_time);
    }
    for(int i = 0 ; i < thread_num; i++){
      singleLockTester[i]->start(testCombo[i]);	
    }
    for(int i = 0 ; i < thread_num; i++){
      singleLockTester[i]->join();
    }
    for(int i = 0; i < thread_num; i++){
      update_sum += testCombo[i]->update_total;
    }
    double update_avg = (update_sum / (repeat_time*thread_num)) * 1e9;
    std::cout << "average update: " << update_avg << "ns" << std::endl;
    for(int i = 0; i < thread_num; i++){ 
      delete singleLockTester[i];	
      delete testCombo[i];
    }
    delete []singleLockTester;
    delete []testCombo;
  }

  // single read multiple update testcase
  void SingleLockTestHelper::runner2(Single_Lock_Counter * single_lock_counter,
      int                    thread_num,
      int                    repeat_time)
  {
    double update_sum = 0;
    double read_sum = 0;
    SingleLockTester_1 **slt_update = new SingleLockTester_1 *[thread_num-1];
    SingleLockTester_2 *slt_read = new SingleLockTester_2(single_lock_counter);
    TestCombo **testCombo = new TestCombo *[thread_num];
    for(int i = 0; i < thread_num-1; i++){
      slt_update[i] = new SingleLockTester_1(single_lock_counter);
    }
    for(int i = 0; i < thread_num; i++){
      testCombo[i] = new TestCombo(repeat_time);
    }
    // do start
    for(int i = 0; i < thread_num-1; i++) {
      slt_update[i]->start(testCombo[i]);
    }
    // do read
    slt_read->start(testCombo[thread_num-1]);
    for(int i = 0; i < thread_num-1; i++){
      slt_update[i]->join();
    }
    slt_read->join();
    for(int i = 0; i < thread_num-1; i++){
      update_sum += testCombo[i]->update_total;
    }
    read_sum += testCombo[thread_num-1]->read_total;
    double update_avg = (update_sum / (repeat_time * (thread_num-1))) * 1e9;
    double read_avg = (read_sum / repeat_time) * 1e9;
    std::cout << "average update is: " << update_avg << "ns" << std::endl;
    std::cout << "average read is: " << read_avg << "ns" << std::endl;
    for(int i = 0; i < thread_num-1; i++) {
      delete slt_update[i];
    }
    for(int i = 0; i < thread_num; i++) {
      delete testCombo[i];
    }
    delete slt_read;
    delete [] slt_update;
    delete [] testCombo;
  }

  // single update multiple read testcase
  void SingleLockTestHelper::runner3(Single_Lock_Counter * single_lock_counter,
      int                    thread_num,
      int                    repeat_time)
  {
    double update_sum = 0;
    double read_sum = 0;
    SingleLockTester_1 *slt_update = new SingleLockTester_1(single_lock_counter);
    SingleLockTester_2 **slt_read = new SingleLockTester_2 *[thread_num-1];
    TestCombo **testCombo = new TestCombo *[thread_num];
    for(int i = 0; i < thread_num-1; i++){
      slt_read[i] = new SingleLockTester_2(single_lock_counter);
    }
    for(int i = 0; i < thread_num; i++){
      testCombo[i] = new TestCombo(repeat_time);
    }
    // do update
    slt_update->start(testCombo[0]);
    // do read
    for(int i = 0; i < thread_num-1; i++) {
      slt_read[i]->start(testCombo[i+1]);
    }
    slt_update->join();
    for(int i = 0; i < thread_num-1; i++){
      slt_read[i]->join();
    }
    for(int i = 0; i < thread_num-1; i++){
      read_sum += testCombo[i+1]->read_total;
    }
    update_sum += testCombo[0]->update_total;
    double read_avg = (read_sum / (repeat_time * (thread_num-1))) * 1e9;
    double update_avg = (update_sum / repeat_time) * 1e9;
    std::cout << "average update is: " << update_avg << "ns" << std::endl;
    std::cout << "average read is: " << read_avg << "ns" << std::endl;
    for(int i = 0; i < thread_num-1; i++) {
      delete slt_read[i];
    }
    for(int i = 0; i < thread_num; i++) {
      delete testCombo[i];
    }
    delete slt_update;
    delete [] slt_read;
    delete [] testCombo;
  }

  // updates test 
  TEST(Basics, Sequential){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner(&single_lock_counter,1,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME);
  }
  TEST(Basics, Concurrency2){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner(&single_lock_counter,2,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME*2);
  }
  TEST(Basics, Concurrency4){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner(&single_lock_counter,4,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME*4);
  }
  TEST(Basics, Concurrency8){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner(&single_lock_counter,8,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME*8);
  }
  TEST(Basics, Concurrency16){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner(&single_lock_counter,16,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME*16);
  }
  TEST(Basics, Concurrency32){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner(&single_lock_counter,32,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME*32);
  }
  TEST(Basics, Concurrency64){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner(&single_lock_counter,64,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME*64);
  }
  TEST(Basics, Concurrency128){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner(&single_lock_counter,128,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME*128);
  }

  // single read multiple update test
  
  TEST(SingleRead, Sequential){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner2(&single_lock_counter,2,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME);
  }
  TEST(SingleRead, Concurrency2){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner2(&single_lock_counter,3,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME*2);
  }
  TEST(SingleRead, Concurrency4){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner2(&single_lock_counter,5,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME*4);
  }
  TEST(SingleRead, Concurrency8){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner2(&single_lock_counter,9,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME*8);
  }
  TEST(SingleRead, Concurrency16){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner2(&single_lock_counter,17,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME*16);
  }
  TEST(SingleRead, Concurrency32){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner2(&single_lock_counter,33,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME*32);
  }
  TEST(SingleRead, Concurrency64){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner2(&single_lock_counter,65,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME*64);
  }
  TEST(SingleRead, Concurrency128){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner2(&single_lock_counter,129,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME*128);
  }
  
  // single update multiple read test
  
  TEST(SingleUpdate, Sequential){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner3(&single_lock_counter,2,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME);
  }
  TEST(SingleUpdate, Concurrency2){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner3(&single_lock_counter,3,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME);
  }
  
  TEST(SingleUpdate, Concurrency4){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner3(&single_lock_counter,5,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME);
  }
  TEST(SingleUpdate, Concurrency8){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner3(&single_lock_counter,9,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME);
  }
  TEST(SingleUpdate, Concurrency16){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner3(&single_lock_counter,17,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME);
  }
  TEST(SingleUpdate, Concurrency32){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner3(&single_lock_counter,33,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME);
  }
  TEST(SingleUpdate, Concurrency64){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner3(&single_lock_counter,65,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME);
  }
  TEST(SingleUpdate, Concurrency128){
    Single_Lock_Counter single_lock_counter;
    SingleLockTestHelper::getInstance().runner3(&single_lock_counter,129,REPEAT_TIME);
    EXPECT_EQ(single_lock_counter.getResult(),REPEAT_TIME);
  }

}// end of non-name namespace

int main(int argc, char *argv[]) {
  return RUN_TESTS();
}
