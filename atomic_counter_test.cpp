#include <tr1/functional>
#include "unit_test.hpp"
#include "thread.hpp"
#include "atomic_counter.hpp"
#include "ticks_clock.hpp"

namespace{
  using std::tr1::bind;
  using base::makeThread;
  using base::Atomic_Counter;
  using base::TicksClock;

//  const int REPEAT_TIME      = 1000;
//  1 million
    const int REPEAT_TIME    = 1000000;
//  const int MEGA_REPEAT_TIME = 10000000;

  double getTotal(TicksClock::Ticks start,
      TicksClock::Ticks end) {
    double duration = end - start;
    double time = duration / TicksClock::ticksPerSecond();
    return time;
  }

  struct TestCombo{
    int repeat_time;
    double update_total;
    double read_total;
    TestCombo(int repeat):repeat_time(repeat), update_total(0),
    read_total(0){}
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

  AtomicTester::AtomicTester(Atomic_Counter * atomic_counter):
    atomic_counter_(atomic_counter){}
  void AtomicTester::start(TestCombo * tc_p){
    tid_ = makeThread(std::tr1::bind(&AtomicTester::test, this, tc_p));
  }
  void AtomicTester::join(){
    pthread_join(tid_,NULL);
  }

  // a test class for simple test
  class AtomicTester_1 : public AtomicTester{
    public:
      AtomicTester_1(Atomic_Counter * atomic_counter) : AtomicTester(atomic_counter){}
      void test(TestCombo * tc_p);  // overwrite the pure vitural function
  };
  void AtomicTester_1::test(TestCombo * tc_p){
    TicksClock::Ticks start = TicksClock::getTicks();
    int counter = 0;
    for(int i = 0 ; i < tc_p->repeat_time; i++){
      counter = atomic_counter_->getAndIncrement();
      // std::cout<<counter<<std::endl;
    }
    TicksClock::Ticks end = TicksClock::getTicks();
    tc_p->update_total = getTotal(start, end);
  }

  class AtomicTester_2 : public AtomicTester{
    public:
      AtomicTester_2(Atomic_Counter * atomic_counter) : AtomicTester(atomic_counter){}
      void test(TestCombo * tc_p);  // overwrite the pure vitural function
  };
  void AtomicTester_2::test(TestCombo * tc_p){
    TicksClock::Ticks start = TicksClock::getTicks();
    for(int i = 0 ; i < tc_p->repeat_time; i++){
      atomic_counter_->getResult();
    }
    TicksClock::Ticks end = TicksClock::getTicks();
    tc_p->read_total = getTotal(start, end);
//    std::cout << "read time inside is: " << tc_p->read_total << std::endl;
  }

  // helper class
  class AtomicTestHelper{
    public:
      void runner(Atomic_Counter * atomic_counter,
          int              thread_num,
          int              repeat_time);
      void runner2(Atomic_Counter * atomic_counter,
          int              thread_num,
          int              repeat_time);
       void runner3(Atomic_Counter * atomic_counter,
          int              thread_num,
          int              repeat_time);
      static AtomicTestHelper& getInstance(){
        static AtomicTestHelper instance;
        return instance;
      }   
    private:
      AtomicTestHelper(){}
      AtomicTestHelper(AtomicTestHelper&);
      AtomicTestHelper& operator=(AtomicTestHelper&);
  };
  void AtomicTestHelper::runner(Atomic_Counter * atomic_counter,
      int              thread_num,
      int              repeat_time)
  {
    double update_sum = 0;
    AtomicTester_1 ** atomicTester = new AtomicTester_1 * [thread_num];
    TestCombo **testCombo = new TestCombo *[thread_num];
    for(int i = 0 ; i < thread_num; i++){
      atomicTester[i] = new AtomicTester_1(atomic_counter);
      testCombo[i] = new TestCombo(repeat_time);
    }
    for(int i = 0 ; i < thread_num; i++){
      atomicTester[i]->start(testCombo[i]);
    }
    for(int i = 0 ; i < thread_num; i++){
      atomicTester[i]->join();
    }
    for(int i = 0; i < thread_num; i++){
      update_sum += testCombo[i]->update_total;
    }
    double update_avg = (update_sum / (thread_num*repeat_time)) * 1e9;
    std::cout << "average update is: " << update_avg << std::endl;
    for(int i = 0; i < thread_num; i++){ 
      delete atomicTester[i];	
      delete testCombo[i];
    }
    delete [] atomicTester;
    delete [] testCombo;
  }

  // single read multiple updates
  void AtomicTestHelper::runner2(Atomic_Counter * atomic_counter,
      int              thread_num,
      int              repeat_time)
  {
    double update_sum = 0;
    double read_sum = 0;
    AtomicTester_1 ** at_update = new AtomicTester_1 * [thread_num];
    AtomicTester_2 * at_read = new AtomicTester_2(atomic_counter);
    TestCombo **testCombo = new TestCombo *[thread_num];
    for(int i = 0 ; i < thread_num-1; i++){
      at_update[i] = new AtomicTester_1(atomic_counter);
    }
    for(int i = 0; i < thread_num; i++){
      testCombo[i] = new TestCombo(repeat_time);
    }
    for(int i = 0 ; i < thread_num-1; i++){
      at_update[i]->start(testCombo[i]);
    }
    at_read->start(testCombo[thread_num-1]);
    for(int i = 0 ; i < thread_num-1; i++){
      at_update[i]->join();
    }
    at_read->join();
    for(int i = 0; i < thread_num-1; i++){
      update_sum += testCombo[i]->update_total;
    }
    read_sum += testCombo[thread_num-1]->read_total;
    double update_avg = (update_sum / (repeat_time*(thread_num-1))) * 1e9;
    double read_avg = (read_sum / repeat_time) * 1e9;
    std::cout << "average update is: " << update_avg << std::endl;
    std::cout << "average read is: " << read_avg << std::endl;
    for(int i = 0; i < thread_num-1; i++){ 
      delete at_update[i];	
    }
    for(int i = 0; i < thread_num; i++){
      delete testCombo[i];
    }
    delete at_read;
    delete [] at_update;
    delete [] testCombo;
  }

  // single update multiple read
  void AtomicTestHelper::runner3(Atomic_Counter * atomic_counter,
      int              thread_num,
      int              repeat_time)
  {
    double update_sum = 0;
    double read_sum = 0;
    AtomicTester_1 * at_update = new AtomicTester_1(atomic_counter);
    AtomicTester_2 ** at_read = new AtomicTester_2 *[thread_num-1];
    TestCombo **testCombo = new TestCombo *[thread_num];
    for(int i = 0 ; i < thread_num-1; i++){
      at_read[i] = new AtomicTester_2(atomic_counter);
    }
    for(int i = 0; i < thread_num; i++){
      testCombo[i] = new TestCombo(repeat_time);
    }
    at_update->start(testCombo[0]);
    for(int i = 0 ; i < thread_num-1; i++){
      at_read[i]->start(testCombo[i+1]);
    }
    at_update->join();
    for(int i = 0 ; i < thread_num-1; i++){
      at_read[i]->join();
    }
    for(int i = 0; i < thread_num-1; i++){
      read_sum += testCombo[i+1]->read_total;
    }
    update_sum += testCombo[0]->update_total;
    double read_avg = (read_sum / (repeat_time*(thread_num-1))) * 1e9;
    double update_avg = (update_sum / repeat_time) * 1e9;
    std::cout << "average update is: " << update_avg << "ns" << std::endl;
    std::cout << "average read is: " << read_avg << "ns" << std::endl;
    for(int i = 0; i < thread_num-1; i++){ 
      delete at_read[i];	
    }
    for(int i = 0; i < thread_num; i++){
      delete testCombo[i];
    }
    delete at_update;
    delete [] at_read;
    delete [] testCombo;
  }
  // update test
  
  TEST(Basics, Sequential){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner(&atomic_counter,1,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME);
  }
  TEST(Basics, Concurrency2){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner(&atomic_counter,2,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME*2);
  }
  TEST(Basics, Concurrency4){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner(&atomic_counter,4,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME*4);
  }
 TEST(Basics, Concurrency8){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner(&atomic_counter,8,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME*8);
  }
 TEST(Basics, Concurrency16){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner(&atomic_counter,16,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME*16);
  }
 TEST(Basics, Concurrency32){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner(&atomic_counter,32,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME*32);
  }
 TEST(Basics, Concurrency64){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner(&atomic_counter,64,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME*64);
  }
 TEST(Basics, Concurrency128){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner(&atomic_counter,128,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME*128);
  }
  
  // single read multiple updates
  
  TEST(SingleRead, Sequential){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner2(&atomic_counter,2,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME);
  }
  TEST(SingleRead, Concurrency2){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner2(&atomic_counter,3,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME*2);
  }
  TEST(SingleRead, Concurrency4){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner2(&atomic_counter,5,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME*4);
  }
 TEST(SingleRead, Concurrency8){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner2(&atomic_counter,9,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME*8);
  }
 TEST(SingleRead, Concurrency16){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner2(&atomic_counter,17,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME*16);
  }
 TEST(SingleRead, Concurrency32){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner2(&atomic_counter,33,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME*32);
  }
 TEST(SingleRead, Concurrency64){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner2(&atomic_counter,65,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME*64);
  }
 TEST(SingleRead, Concurrency128){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner2(&atomic_counter,129,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME*128);
  }
  
// single update multiple reads

  TEST(SingleUpdate, Sequential){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner3(&atomic_counter,2,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME);
  }
  TEST(SingleUpdate, Concurrency2){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner3(&atomic_counter,3,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME);
  }
  TEST(SingleUpdate, Concurrency4){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner3(&atomic_counter,5,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME);
  }
 TEST(SingleUpdate, Concurrency8){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner3(&atomic_counter,9,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME);
  }
 TEST(SingleUpdate, Concurrency16){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner3(&atomic_counter,17,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME);
  }
 TEST(SingleUpdate, Concurrency32){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner3(&atomic_counter,33,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME);
  }
 TEST(SingleUpdate, Concurrency64){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner3(&atomic_counter,65,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME);
  }
 TEST(SingleUpdate, Concurrency128){
    Atomic_Counter atomic_counter;
    AtomicTestHelper::getInstance().runner3(&atomic_counter,129,REPEAT_TIME);
    EXPECT_EQ(atomic_counter.getResult(),REPEAT_TIME);
  }
  
}// end of non-name namespace

int main(int argc, char *argv[]) {
  return RUN_TESTS();
}
