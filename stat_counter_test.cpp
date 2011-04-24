#include <tr1/functional>
#include "unit_test.hpp"
#include "thread.hpp"
#include "stat_counter.hpp"

namespace{
using std::tr1::bind;
using base::makeThread;
using base::Stat_Counter;


}

int main(int argc, char *argv[]) {
  return RUN_TESTS();
}

