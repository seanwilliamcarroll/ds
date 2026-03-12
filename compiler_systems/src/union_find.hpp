#pragma once

#include <iostream>
#include <memory>
#include <unordered_set>

// Union-Find (Disjoint Set Union)
//
// TODO: Implement with path compression + union by rank.

class UnionFind {
public:
  explicit UnionFind(int n) {
    for (int node = 0; node < n; ++node) {
      auto iter = node_to_sets.emplace(
          node, std::make_shared<std::unordered_set<int>>());
      auto &node_set = *(iter.first->second);
      node_set.insert(node);
    }
  }

  int find(int x) {
    const auto &x_set = node_to_sets[x];
    return *std::min_element(x_set->begin(), x_set->end());
  }

  void unite(int x, int y) {
    if (connected(x, y)) {
      return;
    }

    // Find the sets, pop from one into the other, update nodes to point to this
    // set as they are transfered
    const auto &x_set = node_to_sets[x];
    const auto y_set = node_to_sets[y];

    for (auto node : *y_set) {
      x_set->insert(node);
      node_to_sets[node] = x_set;
    }
  }

  bool connected(int x, int y) { return find(x) == find(y); }

  void print() const {
    std::cout << "Print:" << std::endl;
    for (const auto &[node, node_set] : node_to_sets) {
      std::cout << node << " : ";
      if (!node_set) {
        std::cout << "NULL" << std::endl;
        continue;
      }
      for (auto element : *node_set) {
        std::cout << element << ", ";
      }
      std::cout << std::endl;
    }
  }

  std::unordered_map<int, std::shared_ptr<std::unordered_set<int>>>
      node_to_sets;
};
