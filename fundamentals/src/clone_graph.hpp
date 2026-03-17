#pragma once

#include <unordered_map>
#include <vector>

// Clone Graph (LeetCode 133)
//
// Given a reference of a node in a connected undirected graph, return a deep
// copy (clone) of the graph.
//
// Each node in the graph contains a value (int) and a list of its neighbors.
//
//   class Node {
//   public:
//       int val;
//       vector<Node*> neighbors;
//   };
//
// The graph is represented in the test cases using an adjacency list. An
// adjacency list is a collection of unordered lists used to represent a finite
// graph. Each list describes the set of neighbors of a node in the graph.
//
// The given node will always be the first node with val = 1. You must return
// the copy of the given node as a reference to the cloned graph.
//
// Example 1:
//   adjList = [[2,4],[1,3],[2,4],[1,3]]
//   (Node 1 connects to 2,4; Node 2 connects to 1,3; etc.)
//   Output: same structure, but all new Node objects
//
// Example 2:
//   adjList = [[]]
//   (Single node with no neighbors)
//
// Example 3:
//   adjList = []
//   (Empty graph — null input)
//
// Constraints:
//   - The number of nodes in the graph is in the range [0, 100]
//   - 1 <= Node.val <= 100
//   - Node.val is unique for each node
//   - There are no repeated edges and no self-loops
//   - The graph is connected and all nodes can be visited starting from the
//     given node

struct Node {
  int val;
  std::vector<Node *> neighbors;

  explicit Node(int val = 0) : val(val) {}
  Node(int val, std::vector<Node *> neighbors)
      : val(val), neighbors(std::move(neighbors)) {}
};

inline Node *cloneGraph(Node *node) {
  if (node == nullptr) {
    return nullptr;
  }

  // Need to walk this graph and keep track of the neighbors found
  std::vector<Node *> nodes_to_try{node};
  std::unordered_map<int, Node *> new_nodes;

  // Add the entry to signify we've visited this node before
  new_nodes[node->val] =
      new Node(node->val); // NOLINT(cppcoreguidelines-owning-memory)
  while (!nodes_to_try.empty()) {
    auto *node_ptr = nodes_to_try.back();
    nodes_to_try.pop_back();
    auto *new_node_ptr = new_nodes[node_ptr->val];
    for (auto *neighbor_ptr : node_ptr->neighbors) {
      auto iter = new_nodes.find(neighbor_ptr->val);
      Node *new_neighbor_ptr = nullptr;
      if (iter == new_nodes.end()) {
        // Haven't seen this node before
        nodes_to_try.push_back(neighbor_ptr);
        new_neighbor_ptr = new Node(
            neighbor_ptr->val); // NOLINT(cppcoreguidelines-owning-memory)
        new_nodes[neighbor_ptr->val] =
            new_neighbor_ptr; // NOLINT(cppcoreguidelines-owning-memory)
      } else {
        new_neighbor_ptr = iter->second;
      }
      new_node_ptr->neighbors.push_back(new_neighbor_ptr);
    }
  }

  return new_nodes[node->val];
}
