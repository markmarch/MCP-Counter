#include <tr1/functional>
#include <iostream>

#include "unit_test.hpp"
#include "thread.hpp"
#include "combining_tree.hpp"
#include "ticks_clock.hpp"

namespace {

using std::tr1::bind;
using base::makeThread;
using base::Combining_Tree;
using base::TicksClock;

//const int REPEAT_TIME = 1000;
// 1 million
const int REPEAT_TIME = 1000000;
//const int MEGA_REPEAT_TIME = 10000000;

// calculate total update/read time per-thread
static double getTotal(TicksClock::Ticks start, 
    TicksClock::Ticks end) {
  double duration = end - start;
  double time = duration / TicksClock::ticksPerSecond();
  return time;
}

struct TestCombo{
  int thread_id;
  int repeat_time;
  double update_total;  // update operation time
  double read_total;    // read operation time
  // Constructor
  TestCombo(int thread_id, int repeat_time):
    thread_id(thread_id),
    repeat_time(repeat_time), update_total(0),
    read_total(0) { }
};


class TreeTester_1{
public:
  TreeTester_1(Combining_Tree * tree);
  ~TreeTester_1(){};
  void start1(TestCombo * tc_p);
  void start2(TestCombo * tc_p);
  void start3(TestCombo * tc_p);
  void test1(TestCombo * tc_p);
  void test2(TestCombo * tc_p);
  void test3(TestCombo * tc_p);
  void join();

private:
  Combining_Tree * tree_;
  pthread_t        tid_;
  struct timespec interval_;  //time interval
};

TreeTester_1::TreeTester_1(Combining_Tree * tree)
  : tree_(tree){
    this->interval_ = {0, 1000};  // 1 microsecond interval
  }
// Normal test
void TreeTester_1::start1(TestCombo * tc_p){
  tid_ = makeThread(std::tr1::bind(&TreeTester_1::test1, this, tc_p));
}
// Test with time interval
void TreeTester_1::start2(TestCombo * tc_p){
  tid_ = makeThread(std::tr1::bind(&TreeTester_1::test2, this, tc_p));
}
// Test with read operation
void TreeTester_1::start3(TestCombo * tc_p){
  tid_ = makeThread(std::tr1::bind(&TreeTester_1::test3, this, tc_p));
}

void TreeTester_1::join(){
  pthread_join(tid_, NULL);
}
// normal update test
void TreeTester_1::test1(TestCombo * tc_p){
  TicksClock::Ticks start = TicksClock::getTicks();
  for(int i = 0 ; i < tc_p->repeat_time; i++){
    tree_->getAndIncrement(tc_p->thread_id);
  }
  TicksClock::Ticks end = TicksClock::getTicks();
  tc_p->update_total = getTotal(start, end);
//  std::cout << "update operation duration: " << tc_p->update_total << std::endl;
}

// update with time interval
void TreeTester_1::test2(TestCombo * tc_p){
  TicksClock::Ticks start = TicksClock::getTicks();
  for(int i = 0 ; i < tc_p->repeat_time; i++){
    tree_->getAndIncrement(tc_p->thread_id);
    nanosleep(&interval_, NULL);
  }
  TicksClock::Ticks end = TicksClock::getTicks();
  tc_p->update_total = getTotal(start, end);
}

// read operation
void TreeTester_1::test3(TestCombo * tc_p){
  TicksClock::Ticks start = TicksClock::getTicks();
  for(int i = 0 ; i < tc_p->repeat_time; i++){
    tree_->getResult();
  }
  TicksClock::Ticks end = TicksClock::getTicks();
  tc_p->read_total = getTotal(start, end);
//  std::cout << "read operation duration: " << tc_p->read_total << std::endl;
}

class TreeTestHelper{
public:
  void runner(Combining_Tree * tree,
              int              tree_size, 
              int              thread_num,
              int              repeat_time
             );
  void runner2(Combining_Tree * tree,
              int               tree_size, 
              int               thread_num,
              int               repeat_time
             );
  void runner3(Combining_Tree * tree,
              int               tree_size,
              int               thread_num,
              int               repeat_time
              );
  void runner4(Combining_Tree * tree,
              int               tree_size,
              int               thread_num,
              int               repeat_time
              );

  static TreeTestHelper& getInstance(){
    static TreeTestHelper instance;
    return instance;
  }
private:
  TreeTestHelper(){}
  TreeTestHelper(TreeTestHelper &);
  TreeTestHelper& operator=(TreeTestHelper&);
};

/***************** basic running case ***********************/
void TreeTestHelper::runner(Combining_Tree * tree, 
                            int              tree_size, 
                            int              thread_num, 
                            int              repeat_time)
{
  TreeTester_1 ** treeTester = new TreeTester_1* [thread_num];
  TestCombo    ** testCombo  = new TestCombo* [thread_num]; 
  double update_sum = 0;  // total update time
  for(int i = 0 ; i < thread_num ; i++){
    treeTester[i] = new TreeTester_1(tree);
    testCombo[i]  = new TestCombo(i, repeat_time);
  }
  for(int i = 0 ; i < thread_num ; i++){
    treeTester[i]->start1(testCombo[i]);
  } 
  for(int i = 0 ; i < thread_num ; i++){
    treeTester[i]->join();
  }
  // Collect time
  for(int i = 0; i < thread_num; i++) {
//    get average update time for each thread
//    std::cout << "time per thread: " << testCombo[i]->update_total << std::endl;
//    double temp_avg = testCombo[i]->update_total / repeat_time;
//    std::cout << "average for this thread is: " << temp_avg << std::endl;
//    update_sum += temp_avg;
    update_sum += testCombo[i]->update_total;
  }
  // calculate average time
//  std::cout << "update sum is: " << update_sum << std::endl;
//    double update_avg = (update_sum / thread_num) * 1e9;
  double update_avg = (update_sum / (thread_num * repeat_time)) * 1e9;
//  std::cout << "thread count is: " << thread_num << std::endl;
  std::cout << "average update time: " << update_avg << "ns" << std::endl;
  for(int i = 0; i < thread_num; i++) {
    delete treeTester[i];
    delete testCombo[i];
  }
  delete [] testCombo;
  delete [] treeTester;
}

/**************** time interval running case ***************************/
void TreeTestHelper::runner2(Combining_Tree * tree, 
                            int              tree_size, 
                            int              thread_num, 
                            int              repeat_time)
{
  TreeTester_1 ** treeTester = new TreeTester_1* [thread_num];
  TestCombo    ** testCombo  = new TestCombo* [thread_num]; 
  double update_sum = 0;  // total update time
  for(int i = 0 ; i < thread_num ; i++){
    treeTester[i] = new TreeTester_1(tree);
    testCombo[i]  = new TestCombo(i, repeat_time);
  }
  for(int i = 0 ; i < thread_num ; i++){
    treeTester[i]->start2(testCombo[i]);
  }
  for(int i = 0 ; i < thread_num ; i++){
    treeTester[i]->join();
  }
  for(int i = 0; i < thread_num; i++) {
    // will exclude sleep time
//    double temp_avg = (testCombo[i]->update_total - repeat_time * 1e-6) / repeat_time;
//    update_sum += temp_avg;
      update_sum += testCombo[i]->update_total;
  }
//  double update_avg = (update_sum / thread_num) * 1e9;
//    std::cout << "update_sum in time interval is: " << update_sum << std::endl;
    double update_avg = ((update_sum - repeat_time*thread_num*1e-6) / (repeat_time*thread_num))*1e9;
//  std::cout << "thread count is: " << thread_num << std::endl;
  std::cout << "average update time: " << update_avg << "ns" << std::endl;
  for(int i = 0; i < thread_num; i++) {
    delete treeTester[i];
    delete testCombo[i];
  }
  delete [] testCombo;
  delete [] treeTester;
}

/******** single thread read multiple thread update running case *********/
void TreeTestHelper::runner3(Combining_Tree * tree, 
                            int              tree_size, 
                            int              thread_num, 
                            int              repeat_time)
{
  TreeTester_1 ** treeTester = new TreeTester_1* [thread_num];
  TestCombo    ** testCombo  = new TestCombo* [thread_num]; 
  double update_sum = 0;  // total update time
  double read_sum = 0;    // total read time
  for(int i = 0 ; i < thread_num ; i++){
    treeTester[i] = new TreeTester_1(tree);
    testCombo[i]  = new TestCombo(i, repeat_time);
  }
  for(int i = 0 ; i < thread_num ; i++){
//    std::cout << "thread now is: " << i << std::endl;
    if(i == thread_num - 1)  // let last thread do the read
      treeTester[i]->start3(testCombo[i]);  // read
    else
      treeTester[i]->start1(testCombo[i]);  // normal update
  }
  for(int i = 0 ; i < thread_num ; i++){
    treeTester[i]->join();
  }
  for(int i = 0; i < thread_num; i++) {
    if(i == thread_num - 1) {
//      double temp_read_avg = testCombo[i]->read_total / repeat_time;
//      read_sum += temp_read_avg;
        read_sum += testCombo[i]->read_total;
    }
    else {
//      double temp_update_avg = testCombo[i]->update_total / repeat_time;
//      update_sum += temp_update_avg;
      update_sum += testCombo[i]->update_total;
    }
  }
  // Total update
//  double update_avg = (update_sum / (thread_num - 1)) * 1e9;
    double update_avg = (update_sum / (repeat_time*(thread_num-1))) * 1e9;
  // Total read
  double read_avg = (read_sum / repeat_time) * 1e9;
//  std::cout << "thread count is: " << thread_num << std::endl;
  std::cout << "average update time: " << update_avg << "ns" << std::endl;
  std::cout << "average read time: " << read_avg << "ns" << std::endl;
  for(int i = 0; i < thread_num; i++) {
    delete treeTester[i];
    delete testCombo[i];
  }
  delete [] testCombo;
  delete [] treeTester;
}

/****************** multiple read single update case *******************/
void TreeTestHelper::runner4(Combining_Tree * tree, 
                            int              tree_size, 
                            int              thread_num, 
                            int              repeat_time)
{
  TreeTester_1 ** treeTester = new TreeTester_1* [thread_num];
  TestCombo    ** testCombo  = new TestCombo* [thread_num]; 
  double update_sum = 0;  // total update time
  double read_sum = 0;    // total read time
  for(int i = 0 ; i < thread_num ; i++){
    treeTester[i] = new TreeTester_1(tree);
    testCombo[i]  = new TestCombo(i, repeat_time);
  }
  for(int i = 0 ; i < thread_num ; i++){
    if(i == 0) {  // let last n/2 threads do the read
      treeTester[i]->start1(testCombo[i]);  // update
    }
    else {
      treeTester[i]->start3(testCombo[i]);  // read
    }
  }
  for(int i = 0 ; i < thread_num ; i++){
    treeTester[i]->join();
  }
  for(int i = 0; i < thread_num; i++) {
    if(i == 0) {
//      double temp_read_avg = testCombo[i]->read_total / repeat_time;
//      read_sum += temp_read_avg;
      update_sum += testCombo[i]->update_total;
    }
    else {
//      double temp_update_avg = testCombo[i]->update_total / repeat_time;
//      update_sum += temp_update_avg;
      read_sum += testCombo[i]->read_total;
    }
  }
  // Total update
//  double update_avg = (update_sum / (thread_num - 1)) * 1e9;
  double update_avg = (update_sum / repeat_time) * 1e9;
  // Total read
  double read_avg = (read_sum / (repeat_time*(thread_num-1))) * 1e9;
//  std::cout << "thread count is: " << thread_num << std::endl;
  std::cout << "average update time: " << update_avg << "ns" << std::endl;
  std::cout << "average read time: " << read_avg << "ns" << std::endl;
  for(int i = 0; i < thread_num; i++) {
    delete treeTester[i];
    delete testCombo[i];
  }
  delete [] testCombo;
  delete [] treeTester;
}
/************************ basic tests ***********************/

TEST(Basics, Sequential){
  Combining_Tree tree(1);

  TreeTestHelper::getInstance().runner(&tree,1,1,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME);
 }
TEST(Basics, Concurrency2){
  Combining_Tree tree(2);
  TreeTestHelper::getInstance().runner(&tree,2,2,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*2);
}
TEST(Basics, Concurrency4){
  Combining_Tree tree(4);
  TreeTestHelper::getInstance().runner(&tree,4,4,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*4);
}
TEST(Basics, Concurrency8){
  Combining_Tree tree(8);
  TreeTestHelper::getInstance().runner(&tree,8,8,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*8);
}
TEST(Basics, Concurrency16) {
  Combining_Tree tree(16);
  TreeTestHelper::getInstance().runner(&tree,16,16,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*16);
}
TEST(Basics, Concurrency32){
  Combining_Tree tree(32);
  TreeTestHelper::getInstance().runner(&tree,32,32,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*32);
}
TEST(Basics, Concurrency64){
  Combining_Tree tree(64);
  TreeTestHelper::getInstance().runner(&tree,64,64,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*64);
}
TEST(Basics, Concurrency128){
  Combining_Tree tree(128);
  TreeTestHelper::getInstance().runner(&tree,128,128,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*128);
}

/*
TEST(BasicsMega, Sequential){
  Combining_Tree tree(1);
  TreeTestHelper::getInstance().runner(&tree,1,1,MEGA_REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), MEGA_REPEAT_TIME);
}

TEST(BasicsMega, SequentialTwoNode){
  Combining_Tree tree(2);
  TreeTestHelper::getInstance().runner(&tree,2,1,MEGA_REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), MEGA_REPEAT_TIME);
}

TEST(BasicsMega, ConcurrencyThreeNodeTwoThread){
  Combining_Tree tree(3); 
  TreeTestHelper::getInstance().runner(&tree,3,2,MEGA_REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), MEGA_REPEAT_TIME*2);
}

TEST(BasicsMega, ConcurrencyThreeNodeThreeThread){
  Combining_Tree tree(3);
  TreeTestHelper::getInstance().runner(&tree,3,3,MEGA_REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), MEGA_REPEAT_TIME*3);
}

TEST(BasicsMega, ConcurrencyThreeNodeFourThread){
  Combining_Tree tree(3);
  TreeTestHelper::getInstance().runner(&tree,3,4,MEGA_REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), MEGA_REPEAT_TIME*4);
}


TEST(BasicsMega, ConcurrencySevenNodeEightThread){
  Combining_Tree tree(7); 
  TreeTestHelper::getInstance().runner(&tree,7,8,MEGA_REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), MEGA_REPEAT_TIME*8);
}
*/

/************** with time interval ***********************/

TEST(TimeInterval, Sequential){
  Combining_Tree tree(1);

  TreeTestHelper::getInstance().runner2(&tree,1,1,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME);
}
TEST(TimeInterval, Concurrency2){
  Combining_Tree tree(2);
  
  TreeTestHelper::getInstance().runner2(&tree,2,2,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*2);
}
TEST(TimeInterval, Concurrency4){
  Combining_Tree tree(4);
  TreeTestHelper::getInstance().runner2(&tree,4,4,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*4);
}
TEST(TimeInterval, Concurrency8){
  Combining_Tree tree(8);
  TreeTestHelper::getInstance().runner2(&tree,8,8,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*8);
}
TEST(TimeInterval, Concurrency16){
  Combining_Tree tree(16);
  TreeTestHelper::getInstance().runner2(&tree,16,16,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*16);
}
TEST(TimeInterval, Concurrency32){
  Combining_Tree tree(32);
  TreeTestHelper::getInstance().runner2(&tree,32,32,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*32);
}
TEST(TimeInterval, Concurrency64){
  Combining_Tree tree(64);
  TreeTestHelper::getInstance().runner2(&tree,64,64,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*64);
}
TEST(TimeInterval, Concurrency128){
  Combining_Tree tree(128);
  TreeTestHelper::getInstance().runner2(&tree,128,128,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*128);
}

/**************** single read multiple updates ********************/

TEST(SingleRead, Sequential) {
  Combining_Tree tree(1);
  TreeTestHelper::getInstance().runner3(&tree,1,2,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME);
}
TEST(SingleRead, Concurrency2){
  Combining_Tree tree(2);
  TreeTestHelper::getInstance().runner3(&tree,2,3,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*2);
}
TEST(SingleRead, Concurrency4){
  Combining_Tree tree(4);
  TreeTestHelper::getInstance().runner3(&tree,4,5,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*4);
}
TEST(SingleRead, Concurrency8){
  Combining_Tree tree(8);
  TreeTestHelper::getInstance().runner3(&tree,8,9,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*8);
}
TEST(SingleRead, Concurrency16){
  Combining_Tree tree(16);
  TreeTestHelper::getInstance().runner3(&tree,16,17,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*16);
}
TEST(SingleRead, Concurrency32){
  Combining_Tree tree(32);
  TreeTestHelper::getInstance().runner3(&tree,32,33,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*32);
}
TEST(SingleRead, Concurrency64){
  Combining_Tree tree(64);
  TreeTestHelper::getInstance().runner3(&tree,64,65,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*64);
}
TEST(SingleRead, Concurrency128){
  Combining_Tree tree(128);
  TreeTestHelper::getInstance().runner3(&tree,128,129,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*128);
}

/*********** single update multiple reads *********************/

TEST(SingleUpdate, Sequential) {
  Combining_Tree tree(1);
  TreeTestHelper::getInstance().runner4(&tree,1,2,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME);
}
TEST(SingleUpdate, Concurrent2) {
  Combining_Tree tree(1);
  TreeTestHelper::getInstance().runner4(&tree,1,3,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME);
}
TEST(SingleUpdate, Concurrent4) {
  Combining_Tree tree(1);
  TreeTestHelper::getInstance().runner4(&tree,1,5,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME);
}
TEST(SingleUpdate, Concurrent8) {
  Combining_Tree tree(1);
  TreeTestHelper::getInstance().runner4(&tree,1,9,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME);
}
TEST(SingleUpdate, Concurrent16) {
  Combining_Tree tree(1);
  TreeTestHelper::getInstance().runner4(&tree,1,17,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME);
}
TEST(SingleUpdate, Concurrent32) {
  Combining_Tree tree(1);
  TreeTestHelper::getInstance().runner4(&tree,1,33,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME);
}
TEST(SingleUpdate, Concurrent64) {
  Combining_Tree tree(1);
  TreeTestHelper::getInstance().runner4(&tree,1,65,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME);
}
TEST(SingleUpdate, Concurrent128) {
  Combining_Tree tree(1);
  TreeTestHelper::getInstance().runner4(&tree,1,129,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME);
}


} // unnamed namespace
int main(int argc, char *argv[]) {
  return RUN_TESTS();
}

