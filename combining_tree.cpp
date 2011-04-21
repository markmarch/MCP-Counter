#include "combining_tree.hpp"
#include <vector>
using namespace base;
/*
the tree should be complete binary tree
*/
Combining_Tree::Combining_Tree(int size, int thread_num):
  thread_num_(thread_num),
  tree_size_(size)
{ 
  int leaf_num       =  (this->tree_size_ + 1) / 2;             // leaf num in the node 
  this->leaf_        =  new Node* [leaf_num];                   // create the leaf node in a pointer array.
  this->nodes_       =  new Node* [this->tree_size_];            // create the tree body.
  this->nodes_[0]    =  new Node ();                            // initialization then root node.
  
  // initialization all other nodes in the tree
  for(int i = 1; i < this->tree_size_; i++){
    this->nodes_[i] = new Node(this->nodes_[(i-1)/2]);
  }

  // set each leaf pointer point to its corresponding node
  for(int i = 0; i < leaf_num; i++){
    this->leaf_[i] =  this->nodes_[this->tree_size_ - leaf_num + i]; // seems bug is here
  }
}

Combining_Tree::~Combining_Tree(){
  for(int i = 0 ; i < this->tree_size_; i++){
    delete this->nodes_[i];
  }
  delete []this->leaf_;
  delete []this->nodes_;
}

int Combining_Tree::getResult(){
  return nodes_[0]->result_;
}
int Combining_Tree::getAndIncrement(int thread_id){
  std::vector <Node*> nodes_stack;
  Node * myLeaf      = this->leaf_[thread_id / 2];
  Node * node        = myLeaf;
  
  // determine first or second node reach to a point
  while(node->pre_combine()){
    node             = node->getParent();    // climb to the parent node
  }
  Node * stop        = node;            // set the highest point the climb reached
  node               = myLeaf;          // set node to original point
  
  int combined       = 1;               // make an increase on here.
  
  while (node != stop){                 // do combining until reach the stop point.
    combined         = node->combine(combined);  // combine each point along with climbing.
    nodes_stack.push_back(node);                 // trace the node reached.
    node             = node->getParent();        // climb up 
  }

  int prior = stop->op(combined);        // set the final increased value to the stop point

  while(!nodes_stack.empty()) {          // iterater all the element in the stack
    node             = nodes_stack.back();  
    nodes_stack.pop_back();              
    node->distribute(prior);             // call distribute function to all waiting thread
  }

  return prior;
}
