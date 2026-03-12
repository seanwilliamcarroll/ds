#pragma once

#include "union_find.hpp"
#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>

// Given an undirected graph that was a tree with one extra edge added,
// find and return the edge that creates a cycle.
//
// Edges are 1-indexed: {{1,2}, {2,3}, ...}
// Returns the redundant edge as a pair.
inline std::pair<int, int>
findRedundantConnection(const std::vector<std::pair<int, int>> &edges) {
  using UF = UnionFind<true, true>;
  int max_edge = 0;
  for (const auto &[node_a, node_b] : edges) {
    max_edge = std::max({node_a, node_b, max_edge});
  }

  // 1-indexed
  UF union_find(max_edge);
  for (const auto &[node_a, node_b] : edges) {
    if (union_find.connected(node_a - 1, node_b - 1)) {
      return {node_a, node_b};
    }
    union_find.unite(node_a - 1, node_b - 1);
  }

  return {-1, -1};
}
