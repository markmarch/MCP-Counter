#ifndef MCP_BASE_COMBINING_TREE_HEADER
#define MCP_BASE_COMBINING_TREE_HEADER

#include "tree_node.hpp"
namespace base{
class Combining_Tree{
public:
  Combining_Tree(int size);
  int getAndIncrement(int thread_id);
  int getResult();
  ~Combining_Tree();
private:
  int      tree_size_;          // record the tree size
  Node **  leaf_;               // the pointer point to the leaf nodes 
  Node **  nodes_;              // the pointer point to the root node
  Combining_Tree(Combining_Tree&);
  Combining_Tree& operator=(Combining_Tree&);
};
}

#endif
