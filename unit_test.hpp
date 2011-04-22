#ifndef MCP_BASE_UNIT_TEST_HEADER
#define MCP_BASE_UNIT_TEST_HEADER

// TODO allow EXPECT_XX() << "details";

#include <iostream>
#include <string>
#include <vector>

extern bool MCP_BASE_exit_on_fatal;
extern bool MCP_BASE_has_fatal_message;

namespace base {

using std::string;
using std::vector;

class TestCase {
public:
  virtual ~TestCase() {}

  virtual void TestBody() = 0;

  const string& group() const { return group_; }
  const string& name() const  { return name_; }
  int errors() const          { return errors_; }

protected:
  TestCase(const string& group, const string& name)
    : group_(group), name_(name), errors_(0) {}

  void incErrors()     { ++errors_; }

private:
  string group_;
  string name_;
  int    errors_;

};

class TestRegistry {
public:
  ~TestRegistry() {}

  static TestRegistry* getInstance();

  TestCase* registerCase(TestCase* test);
  int runAndReset();

private:
  typedef vector<TestCase*> Tests;

  static TestRegistry*      instance_;
  Tests                     tests_;  // owned by this

  TestRegistry() {}
};

TestRegistry* TestRegistry::instance_ = NULL;

TestRegistry* TestRegistry::getInstance() {
  if (instance_ == NULL) instance_ = new TestRegistry;
  return instance_;
}
  
TestCase* TestRegistry::registerCase(TestCase* test) {
  tests_.push_back(test);
  return test;
}

int TestRegistry::runAndReset() {
  int errors = 0;
  //MCP_BASE_exit_on_fatal = false;
  
  for (Tests::const_iterator it = tests_.begin(); it != tests_.end(); ++it) {
    TestCase* test = *it;
    std::cout << "Running " << test->group() << "(" << test->name() << "): ";
    test->TestBody();
    if (test->errors()) {
      errors += test->errors();
      std::cout << std::endl;
    } else {
      std::cout << "PASSED" << std::endl;
    }
    delete test;
  } 

  tests_.clear();
  TestRegistry* me = instance_;
  instance_ = NULL;
  delete me;

  return errors; 
}

} // namespace base

#define EXPECT_TRUE(a) \
  if (!(a)) { \
    if (errors() == 0) std::cout << "FAILED"; \
    std::cout << "\n    Line " << __LINE__ \
              << " Expected true " << # a << ": " <<  (a); \
    incErrors(); \
  }

#define EXPECT_FALSE(a) \
  if (a) { \
    if (errors() == 0) std::cout << "FAILED"; \
    std::cout << "\n    Line " << __LINE__ \
              << " Expected false " << # a << ": " <<  (a); \
    incErrors(); \
  }

#define EXPECT_EQ(a, b) \
  if ((a) != (b)) { \
    if (errors() == 0) std::cout << "FAILED"; \
    std::cout << "\n    Line " << __LINE__ \
              << " Expected equal " << # a << " and " << # b << ": (" \
              << (a) << " " << (b) << ")";  \
    incErrors(); \
  }

#define EXPECT_NEQ(a, b) \
  if ((a) == (b)) { \
    if (errors() == 0) std::cout << "FAILED"; \
    std::cout << "\n    Line " << __LINE__ \
              << " Expected different " << # a << " and " << # b << ": (" \
              << (a) << " " << (b) << ")";  \
    incErrors(); \
  }

#define EXPECT_SL(a, b) \
  if ((a) >= (b)) { \
    if (errors() == 0) std::cout << "FAILED"; \
    std::cout << "\n    Line " << __LINE__ \
              << " Expected smaller " << # a << " and " << # b << ": (" \
              << (a) << " " << (b) << ")"; \
    incErrors(); \
  }


#define EXPECT_GT(a, b) \
  if ((a) <= (b)) { \
    if (errors() == 0) std::cout << "FAILED"; \
    std::cout << "\n    Line " << __LINE__ \
              << " Expected greater " << # a << " and " << # b << ": (" \
              << (a) << " " << (b) << ")"; \
    incErrors(); \
  }

#define EXPECT_FATAL(a) \
  MCP_BASE_has_fatal_message = false; \
  (a); \
  if (!MCP_BASE_has_fatal_message) { \
    if (errors() == 0) std::cout << "FAILED"; \
    std::cout << "\n    Line " << __LINE__ << " Expected fatal " << # a; \
    incErrors(); \
  }

#define TEST_CLASS_NAME(test_group, test_name) \
  test_group ## _ ## test_name ## _Test

#define TEST(test_group, test_name) \
  class TEST_CLASS_NAME(test_group, test_name) : public base::TestCase { \
  public: \
    TEST_CLASS_NAME(test_group, test_name)(); \
    virtual void TestBody(); \
  private: \
    static TestCase* const me_; \
  }; \
  \
  base::TestCase* const TEST_CLASS_NAME(test_group, test_name)::me_ = \
    base::TestRegistry::getInstance()->registerCase( \
      new TEST_CLASS_NAME(test_group, test_name)); \
  \
  TEST_CLASS_NAME(test_group, test_name)::\
    TEST_CLASS_NAME(test_group, test_name)() \
    : TestCase(# test_group, # test_name) {} \
  void TEST_CLASS_NAME(test_group, test_name)::TestBody() 

#define RUN_TESTS() \
  (base::TestRegistry::getInstance()->runAndReset())


#endif // MCP_BASE_UNIT_TEST_HEADER
