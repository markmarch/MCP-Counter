#include <tr1/functional>
#include "unit_test.hpp"
#include "thread.hpp"
#include "combining_tree.hpp"

namespace {

using std::tr1::bind;
using base::makeThread;
using base::Combining_Tree;

const int REPEAT_TIME = 1000;

class TreeTester_1{
public:
  TreeTester_1(Combining_Tree * tree);
  ~TreeTester_1(){};
  void start1(int thread_id);
  void start2(int thread_id);
  void join();
  void test1(int thread_id);
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

void TreeTester_1::start1(int thread_id){
  tid_ = makeThread(std::tr1::bind(&TreeTester_1::test1, this, thread_id));
}

void TreeTester_1::start2(int thread_id){
  tid_ = makeThread(std::tr1::bind(&TreeTester_1::test2, this, thread_id));
}


void TreeTester_1::join(){
  pthread_join(tid_, NULL);
}

void TreeTester_1::test1(int thread_id){
  for(int i = 0 ; i < REPEAT_TIME; i++){
    tree_->getAndIncrement(thread_id);
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
  for(int i = 0 ; i < thread_num ; i++){
    treeTester[i] = new TreeTester_1(tree);
  }
  for(int i = 0 ; i < thread_num ; i++){
    treeTester[i]->start1(i);
  }
  for(int i = 0 ; i < thread_num ; i++){
    treeTester[i]->join();
    delete treeTester[i];
  }
  delete [] treeTester;
}

TEST(Basics, SequentialOneNode){
  Combining_Tree tree(1,1);
  for(int i = 0 ;i < REPEAT_TIME; i++){
    tree.getAndIncrement(0);
  }
  //std::cout << tree.getResult() << std::endl;
  EXPECT_EQ(tree.getResult(), REPEAT_TIME);
}

TEST(Basics, SequentialTwoNode){
  Combining_Tree tree(2,1);
  for(int i = 0; i< REPEAT_TIME; i++){
    tree.getAndIncrement(0);
  }
  EXPECT_EQ(tree.getResult(), REPEAT_TIME);
}

TEST(Basics, ConcurrencyThreeNodeTwoThread){
  Combining_Tree tree(3,2);

  TreeTester_1* treeTester_1[2];
  for(int i = 0 ; i < 2; i++){
    treeTester_1[i] = new TreeTester_1(&tree);
  }
  //(&tree);
  for(int i = 0 ; i < 2; i++){
    treeTester_1[i]->start1(i);
  }
  for(int i = 0 ; i < 2; i++){
    treeTester_1[i]->join();
    delete treeTester_1[i];
  }
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*2);
}

TEST(Basics, ConcurrencyThreeNodeThreeThread){
  Combining_Tree tree(3,3);

  TreeTester_1* treeTester_1[3];
  for(int i = 0 ; i < 3; i++){
    treeTester_1[i] = new TreeTester_1(&tree);
  }
  //(&tree);
  for(int i = 0 ; i < 3; i++){
    treeTester_1[i]->start1(i);
  }
  for(int i = 0 ; i < 3; i++){
    treeTester_1[i]->join();
    delete treeTester_1[i];
  }
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*3);

}

TEST(Basics, ConcurrencyThreeNodeFourThread){
  Combining_Tree tree(3,4);
 
  TreeTestHelper::getInstance().runner(&tree,3,4,REPEAT_TIME);
  EXPECT_EQ(tree.getResult(), REPEAT_TIME*4);
}

} // unnamed namespace
int main(int argc, char *argv[]) {
  return RUN_TESTS();
}

