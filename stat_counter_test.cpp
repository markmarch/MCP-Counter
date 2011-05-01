#include <tr1/functional>

#include "unit_test.hpp"
#include "thread.hpp"
#include "stat_counter.hpp"
#include "ticks_clock.hpp"

namespace{
  using std::tr1::bind;
  using base::makeThread;
  using base::Stat_Counter;
  using base::TicksClock;

//  const int REPEAT_TIME = 1000; 
//  1 million
  const int REPEAT_TIME = 100000;

  const int MAX_THREAD  = 128;

  static double getTotal(TicksClock::Ticks start,
      TicksClock::Ticks end) {
    double duration = end - start;
    double time = duration / TicksClock::ticksPerSecond();
    return time;
  }
  // message structure
  struct TestCombo{
    int thread_id;
    int repeat_time;
    double update_total; // update time for each thread
    double read_total;   // read time for each thread
    TestCombo(int thread_id, int repeat_time):
      thread_id(thread_id),
      repeat_time(repeat_time),
      update_total(0), read_total(0){}
  };

  // base class
  class StatTester{
    public:
      StatTester(Stat_Counter * stat_counter);
      void start(TestCombo *tc);
      void join();
      virtual void test(TestCombo *tc)  = 0;
    protected:
      Stat_Counter    * stat_counter;
      pthread_t         tid_;
  };
  StatTester::StatTester(Stat_Counter * stat_counter):
    stat_counter(stat_counter){}
  void StatTester::start(TestCombo *tc){
    tid_ = makeThread(std::tr1::bind(&StatTester::test,this, tc));
  }
  void StatTester::join(){
    pthread_join(tid_, NULL);
  }

  // update testcase
  class StatTester_1 : public StatTester{
    public: 
      StatTester_1(Stat_Counter * stat_counter) : StatTester(stat_counter){ }
      void test(TestCombo *tc);           // overwrite the pure vitural function in Stat_Test
  };
  void StatTester_1::test(TestCombo *tc){
    stat_counter->count_register_thread();
    TicksClock::Ticks start = TicksClock::getTicks();
    for(int i = 0 ; i < tc->repeat_time; i++){
      stat_counter->inc_count();
    }
    TicksClock::Ticks end = TicksClock::getTicks();
    tc->update_total = getTotal(start, end);
    stat_counter->count_unregister_thread();
  }

  // read testcase
  class StatTester_2 : public StatTester {
    public:
      StatTester_2(Stat_Counter * stat_counter) : StatTester(stat_counter) { }
      void test(TestCombo *tc);
  };
  void StatTester_2::test(TestCombo *tc) {
    stat_counter->count_register_thread();
    TicksClock::Ticks start = TicksClock::getTicks();
    for(int i = 0; i < tc->repeat_time; i++) {
      stat_counter->read_count();
    }
    TicksClock::Ticks end = TicksClock::getTicks();
    tc->read_total = getTotal(start, end);
    stat_counter->count_unregister_thread();
  }

  // helper class
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
      void runner2(Stat_Counter * stat_counter,
          int            max_thread_num,
          int            repeat_time
          );
      void runner3(Stat_Counter * stat_counter,
          int            max_thread_num,
          int            repeat_time
          );

    private:
      StatTestHelper(){}
      StatTestHelper(StatTestHelper&);
      StatTestHelper& operator=(StatTestHelper&);
  };
  // basic test
  void StatTestHelper::runner(Stat_Counter * stat_counter,
      int            thread_num,
      int            repeat_time)
  {
    StatTester_1  ** statTester  = new StatTester_1* [thread_num];
    TestCombo **testCombo = new TestCombo *[thread_num];
    double update_sum = 0;  // record total update time

    for(int i = 0; i < thread_num; i++){
      statTester[i] = new StatTester_1(stat_counter);
      testCombo[i] = new TestCombo(i, repeat_time);
    }
    for(int i = 0; i < thread_num; i++){
      statTester[i]->start(testCombo[i]);
    }
    for(int i = 0; i < thread_num; i++){
      statTester[i]->join();
    }
    for(int i = 0; i < thread_num; i++) {
      update_sum += testCombo[i]->update_total;
    }
    // calculate average
    double update_avg = (update_sum / (thread_num*repeat_time)) * 1e9;
    std::cout << "average update time: " << update_avg << std::endl;
    for(int i = 0; i < thread_num; i++) {
      delete statTester[i];
      delete testCombo[i];
    }
    delete []statTester;
    delete []testCombo;
  }

  // single read multiple updates
  void StatTestHelper::runner2(Stat_Counter * stat_counter,
      int             thread_num,
      int             repeat_time)
  {
    StatTester_1 **st_update = new StatTester_1 *[thread_num-1];
    TestCombo **testCombo = new TestCombo *[thread_num];
    // reader object and read combo
    StatTester_2 *st_read = new StatTester_2(stat_counter);
    double update_sum = 0;
    double read_sum = 0;
    for(int i = 0; i < thread_num-1; i++) {
      st_update[i] = new StatTester_1(stat_counter);
    }
    for(int i = 0; i < thread_num; i++) { 
      testCombo[i] = new TestCombo(i, repeat_time);
    }
    // start update
    for(int i = 0; i < thread_num-1; i++) {
      st_update[i]->start(testCombo[i]);
    }
    // start reader
    st_read->start(testCombo[thread_num-1]);
    for(int i = 0; i < thread_num-1; i++) {
      st_update[i]->join();
    }
    st_read->join();
    for(int i = 0; i < thread_num-1; i++) {
      update_sum += testCombo[i]->update_total;
    }
    read_sum = testCombo[thread_num-1]->read_total;
    double update_avg = (update_sum / ((thread_num-1)*repeat_time))*1e9;
    std::cout << "average update is: " << update_avg << "ns" << std::endl;
    double read_avg = (read_sum / (repeat_time))*1e9;
    std::cout << "average read is: " << read_avg << "ns" << std::endl;
    for(int i = 0; i < thread_num-1; i++) {
      delete st_update[i];
    }
    for(int i = 0; i < thread_num; i++) {
      delete testCombo[i];
    }
    delete st_read;
    delete []st_update;
    delete []testCombo;
  }

  // multiple read single update
  void StatTestHelper::runner3(Stat_Counter *stat_counter,
      int              thread_num,
      int              repeat_time)
  {
    StatTester_1 *st_update = new StatTester_1(stat_counter);
    StatTester_2 **st_read = new StatTester_2 *[thread_num-1];
    TestCombo **testCombo = new TestCombo *[thread_num];
    double update_sum = 0;
    double read_sum = 0;
    for(int i = 0; i < thread_num-1; i++) {
      st_read[i] = new StatTester_2(stat_counter);
    }
    for(int i = 0; i < thread_num; i++) {
      testCombo[i] = new TestCombo(thread_num, repeat_time);
    } 
    // start one update
    st_update->start(testCombo[0]);
    // start multiple read
    for(int i = 0; i < thread_num-1; i++) {
      st_read[i]->start(testCombo[i+1]);
    }
    st_update->join();
    for(int i = 0; i < thread_num-1; i++) {
      st_read[i]->join();
    }
    update_sum += testCombo[0]->update_total;
    for(int i = 0; i < thread_num-1; i++) {
      read_sum += testCombo[i+1]->read_total;
    }
    double update_avg = (update_sum / repeat_time) * 1e9;
    std::cout << "average update: " << update_avg << "ns" << std::endl;
    double read_avg = (read_sum / (repeat_time * (thread_num-1))) * 1e9;
    std::cout << "average read: " << read_avg << "ns" << std::endl;
    for(int i = 0; i < thread_num-1; i++) {
      delete st_read[i];
    }
    for(int i = 0; i < thread_num; i++) {
      delete testCombo[i];
    }
    delete st_update;
    delete []st_read;
    delete []testCombo;
  }

  /******************** basic test *************************/

  TEST(Basics, Sequential){
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner(&stat_counter,1,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME);
  }
  TEST(Basics, Concurency2){
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner(&stat_counter,2,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME*2);
  }
  TEST(Basics, Concurency4){
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner(&stat_counter,4,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME*4);
  }
  TEST(Basics, Concurency8){
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner(&stat_counter,8,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME*8);
  }
  TEST(Basics, Concurency16){
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner(&stat_counter,16,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME*16);
  }
  TEST(Basics, Concurency32){
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner(&stat_counter,32,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME*32);
  }
  TEST(Basics, Concurency64){
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner(&stat_counter,64,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME*64);
  }
  TEST(Basics, Concurency128){
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner(&stat_counter,128,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME*128);
  }

  /************** single read test ******************/  
  
  TEST(SingleRead, Sequential) {
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner2(&stat_counter,2,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME);
  }
  TEST(SingleRead, concurrency2) {
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner2(&stat_counter,3,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME*2);
  }
  TEST(SingleRead, concurrency4) {
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner2(&stat_counter,5,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME*4);
  }
  TEST(SingleRead, concurrency8) {
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner2(&stat_counter,9,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME*8);
  }
  TEST(SingleRead, concurrency16) {
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner2(&stat_counter,17,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME*16);
  }
  TEST(SingleRead, concurrency32) {
  Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner2(&stat_counter,33,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME*32);
  }
  TEST(SingleRead, concurrency64) {
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner2(&stat_counter,65,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME*64);
  }
  TEST(SingleRead, concurrency128) {
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner2(&stat_counter,129,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME*128);
  }

  /******************* single update test **********************/

 TEST(SingleUpdate, Sequential) {
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner3(&stat_counter,2,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME);
  }
  TEST(SingleUpdate, concurrency2) {
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner3(&stat_counter,3,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME);
  }
  TEST(SingleUpdate, concurrency4) {
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner3(&stat_counter,5,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME);
  }
  TEST(SingleUpdate, concurrency8) {
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner3(&stat_counter,9,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME);
  }
  TEST(SingleUpdate, concurrency16) {
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner3(&stat_counter,17,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME);
  }
  TEST(SingleUpdate, concurrency32) {Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner3(&stat_counter,33,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME);
  }
  TEST(SingleUpdate, concurrency64) {
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner3(&stat_counter,65,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME);
  }
  TEST(SingleUpdate, concurrency128) {
    Stat_Counter stat_counter(MAX_THREAD);
    StatTestHelper::getInstance().runner3(&stat_counter,129,REPEAT_TIME);
    EXPECT_EQ(stat_counter.read_count(),REPEAT_TIME);
  }


} // end of non named namespace


int main(int argc, char *argv[]) {
  return RUN_TESTS();
}

