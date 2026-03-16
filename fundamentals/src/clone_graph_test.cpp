#include "clone_graph.hpp"

#include <queue>
#include <unordered_map>
#include <unordered_set>

#include <gtest/gtest.h>

// Helper: build a graph from an adjacency list (1-indexed).
// adjList[0] = neighbors of node 1, adjList[1] = neighbors of node 2, etc.
// Returns pointer to node 1.
static Node *buildGraph(const std::vector<std::vector<int>> &adjList) {
  if (adjList.empty()) {
    return nullptr;
  }
  std::vector<Node *> nodes;
  nodes.reserve(adjList.size());
  for (size_t i = 0; i < adjList.size(); ++i) {
    nodes.push_back(new Node(static_cast<int>(i) + 1));
  }
  for (size_t i = 0; i < adjList.size(); ++i) {
    for (int neighbor_val : adjList[i]) {
      nodes[i]->neighbors.push_back(nodes[neighbor_val - 1]);
    }
  }
  return nodes[0];
}

// Helper: collect all nodes reachable from a start node.
static std::unordered_set<Node *> collectAll(Node *start) {
  std::unordered_set<Node *> visited;
  if (start == nullptr) {
    return visited;
  }
  std::queue<Node *> queue;
  queue.push(start);
  visited.insert(start);
  while (!queue.empty()) {
    Node *current = queue.front();
    queue.pop();
    for (Node *neighbor : current->neighbors) {
      if (visited.insert(neighbor).second) {
        queue.push(neighbor);
      }
    }
  }
  return visited;
}

// Helper: free all nodes reachable from a start node.
static void freeGraph(Node *start) {
  for (Node *node : collectAll(start)) {
    delete node;
  }
}

// Verify that the clone is a correct deep copy of the original.
static void verifyClone(Node *original, Node *clone) {
  if (original == nullptr) {
    EXPECT_EQ(clone, nullptr);
    return;
  }

  auto origNodes = collectAll(original);
  auto cloneNodes = collectAll(clone);

  // Same number of nodes
  ASSERT_EQ(origNodes.size(), cloneNodes.size());

  // No node pointers shared between original and clone
  for (Node *cn : cloneNodes) {
    EXPECT_EQ(origNodes.count(cn), 0u)
        << "Clone shares a node pointer with original (val=" << cn->val << ")";
  }

  // Build val -> node maps
  std::unordered_map<int, Node *> origByVal;
  for (Node *n : origNodes) {
    origByVal[n->val] = n;
  }
  std::unordered_map<int, Node *> cloneByVal;
  for (Node *n : cloneNodes) {
    cloneByVal[n->val] = n;
  }

  // Same vals exist
  ASSERT_EQ(origByVal.size(), cloneByVal.size());

  // Each node has the same neighbor vals
  for (auto &[val, origNode] : origByVal) {
    ASSERT_TRUE(cloneByVal.count(val)) << "Missing node with val=" << val;
    Node *cloneNode = cloneByVal[val];

    ASSERT_EQ(origNode->neighbors.size(), cloneNode->neighbors.size())
        << "Neighbor count mismatch for val=" << val;

    std::vector<int> origNeighborVals;
    for (Node *n : origNode->neighbors) {
      origNeighborVals.push_back(n->val);
    }
    std::vector<int> cloneNeighborVals;
    for (Node *n : cloneNode->neighbors) {
      cloneNeighborVals.push_back(n->val);
    }
    EXPECT_EQ(origNeighborVals, cloneNeighborVals)
        << "Neighbor mismatch for val=" << val;
  }
}

TEST(CloneGraph, NullInput) { EXPECT_EQ(cloneGraph(nullptr), nullptr); }

TEST(CloneGraph, SingleNode) {
  Node *original = buildGraph({{}});
  Node *clone = cloneGraph(original);
  verifyClone(original, clone);
  freeGraph(original);
  freeGraph(clone);
}

TEST(CloneGraph, LeetCodeExample1) {
  // 1--2
  // |  |
  // 4--3
  Node *original = buildGraph({{2, 4}, {1, 3}, {2, 4}, {1, 3}});
  Node *clone = cloneGraph(original);
  verifyClone(original, clone);
  freeGraph(original);
  freeGraph(clone);
}

TEST(CloneGraph, TwoNodes) {
  // 1--2
  Node *original = buildGraph({{2}, {1}});
  Node *clone = cloneGraph(original);
  verifyClone(original, clone);
  freeGraph(original);
  freeGraph(clone);
}

TEST(CloneGraph, LinearChain) {
  // 1--2--3--4--5
  Node *original = buildGraph({{2}, {1, 3}, {2, 4}, {3, 5}, {4}});
  Node *clone = cloneGraph(original);
  verifyClone(original, clone);
  freeGraph(original);
  freeGraph(clone);
}

TEST(CloneGraph, FullyConnected) {
  // Every node connects to every other node
  Node *original = buildGraph({{2, 3, 4}, {1, 3, 4}, {1, 2, 4}, {1, 2, 3}});
  Node *clone = cloneGraph(original);
  verifyClone(original, clone);
  freeGraph(original);
  freeGraph(clone);
}

TEST(CloneGraph, StarTopology) {
  // Node 1 connects to all others; others only connect to 1
  Node *original = buildGraph({{2, 3, 4, 5}, {1}, {1}, {1}, {1}});
  Node *clone = cloneGraph(original);
  verifyClone(original, clone);
  freeGraph(original);
  freeGraph(clone);
}
