#pragma once

#include <utility>
#include <vector>

// Given an undirected graph that was a tree with one extra edge added,
// find and return the edge that creates a cycle.
//
// Edges are 1-indexed: {{1,2}, {2,3}, ...}
// Returns the redundant edge as a pair.
std::pair<int, int>
findRedundantConnection(const std::vector<std::pair<int, int>> &edges) {
  // TODO
  return {-1, -1};
}
