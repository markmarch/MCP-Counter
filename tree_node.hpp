#ifndef MCP_BASE_TREE_NODE_HEADER
#define MCP_BASE_TREE_NODE_HEADER

#include "lock.hpp"
#include "spinlock.hpp"

namespace base {
static const struct timespec SLEEP_TIME={0,30};

class Node{
friend   class Combining_Tree;
public:
  enum CStatus{
    IDLE,
    FIRST,
    SECOND,
    RESULT,
    ROOT
  };
  Node();                            // for the root
  Node(Node * myParent);             // non-root Node
  Node*        getParent();          // get the parent node of this node 
private:
  static const int RESULT_READY = 4;
  static const int OCCUPIED_AND_LOCKED = 3;
  static const int OCCUPIED_AND_UNLOCKED = 2;
  static const int UNOCCUPIED_AND_LOCKED = 1; 
  static const int UNOCCUPIED_AND_UNLOCKED = 0;

  static const int LOOP_TIME = 200;
  
  
  int          lock_;
  enum         CStatus  cStatus_;    // combining status
  int          firstValue_;          // 1st value to combine
  int          secondValue_;         // 2nd value to combine
  int          result_;              // result of combine
  Node*        parent_;              // pointer to parent
  ConditionVar cond_var_;            // cond_var for node_lock
  Mutex        node_lock_;           // lock on the the node
  char         padding_[32] ;
  // methods
  bool     pre_combine();
  int      combine(int combined);
  int      op(int combined);
  void     distribute(int prior); 
  
  
};


}

#endif
