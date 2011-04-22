#ifndef MCP_BASE_TREE_NODE_HEADER
#define MCP_BASE_TREE_NODE_HEADER

#include "lock.hpp"
#include "spinlock.hpp"
namespace base {
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

  /*static const int pad_size = 128 - ( sizeof(bool) + sizeof(CStatus) + sizeof(int)*3 + sizeof(Node*) + sizeof(ConditionVar) + sizeof(Mutex)); 
   */

  bool         locked_;              // locked status on this node
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
