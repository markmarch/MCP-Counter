#include <tr1/functional>
#include "unit_test.hpp"
#include "thread.hpp"
#include "combining_tree.hpp"

namespace {

using std::tr1::bind;
using base::makeThread;
using base::Combining_Tree;

const int REPEAT_TIME = 1000;
const int MEGA_REPEAT_TIME = 10000000;

struct TestCombo{
  int thread_id;
  int repeat_time;
  TestCombo(int thread_id, int repeat_time):
    thread_id(thread_id),
    repeat_time(repeat_time){}
};


class TreeTester_1{
public:
  TreeTester_1(Combining_Tree * tree);
  ~TreeTester_1(){};
  void start1(TestCombo * tc_p);
  void start2(int thread_id);
  void join();
  void test1(TestCombo * tc_p);
  void test2(int thread_id);

private:
  Combining_Tree * tree_;
  pthread_t        tid_;
};

class TreeTestHelper{
public:
  void runner(Combining_Tree * tree,
              int              tree_size, 
              int              thread_num,
              int              repeat_time
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
TreeTester_1::TreeTester_1(Combining_Tree * tree)
  : tree_(tree){}

void TreeTester_1::start1(TestCombo * tc_p){
  tid_ = makeThread(std::tr1::bind(&TreeTester_1::test1, this, tc_p));
}

void TreeTester_1::start2(int thread_id){
  tid_ = makeThread(std::tr1::bind(&TreeTester_1::test2, this, thread_id));
}


void TreeTester_1::join(){
  pthread_join(tid_, NULL);
}

void TreeTester_1::test1(TestCombo * tc_p){  
  for(int i = 0 ; i < tc_p->repeat_time; i++){
    tree_->getAndIncrement(tc_p->thread_id);
  }
}

void TreeTester_1::test2(int thread_id){
  for(int i = 0 ; i < REPEAT_TIME; i++){
    tree_->getAndIncrement(thread_id);
    usleep(10);
  }
}
void TreeTestHelper::runner(Combining_Tree * tree, 
                            int              tree_size, 
                            int              thread_num, 
                            int              repeat_time)
{
  TreeTester_1 ** treeTester = new TreeTester_1* [thread_num];
  TestCombo    ** testCombo  = new TestCombo* [thread_num]; 
  for(int i = 0 ; i < thread_num ; i++){
    treeTester[i] = new TreeTester_1(tree);
    testCombo[i]  = new TestCombo(i, repeat_time);
  }
  for(int i = 0 ; i < thread_num ; i++){
    treeTester[i]->start1(testCombo[i]);
  }
  for(int i = 0 ; i < thread_num ; i++){
    treeTester[i]->join();
    delete treeTester[i];
    delete testCombo[i];
  }
  delete [] testCombo;
  delete [] treeTester;
}

TEST(Basics, SequentialOneNode){
  Combining_Tree tree(1);
  TreeTestHelper::getInstance().runner(&tree,1,1,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME);
}

TEST(Basics, SequentialTwoNode){
  Combining_Tree tree(2);
  TreeTestHelper::getInstance().runner(&tree,2,1,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME);
}

TEST(Basics, ConcurrencyThreeNodeTwoThread){
  Combining_Tree tree(3); 
  TreeTestHelper::getInstance().runner(&tree,3,2,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*2);
}

TEST(Basics, ConcurrencyThreeNodeThreeThread){
  Combining_Tree tree(3);
  TreeTestHelper::getInstance().runner(&tree,3,3,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*3);
}

TEST(Basics, ConcurrencyThreeNodeFourThread){
  Combining_Tree tree(3); 
  TreeTestHelper::getInstance().runner(&tree,3,4,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*4);
}

TEST(Basics, ConcurrencySevenNodeEightThread){
  Combining_Tree tree(7); 
  TreeTestHelper::getInstance().runner(&tree,7,8,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*8);
}

TEST(BasicsMega, SequentialOneNode){
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

} // unnamed namespace
int main(int argc, char *argv[]) {
  return RUN_TESTS();
}

