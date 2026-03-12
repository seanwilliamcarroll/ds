#pragma once

#include <vector>

// Union-Find (Disjoint Set Union)
//
// TODO: Implement with union by rank.

class UnionFind {
public:
  explicit UnionFind(int n) : parent(n, 0) {
    for (int node = 0; node < n; ++node) {
      parent[node] = node;
    }
  }

  int find(int x) {
    int x_walker = x;
    while (parent[x_walker] != x_walker) {
      x_walker = parent[x_walker];
    }
    parent[x] = x_walker;
    return x_walker;
  }

  void unite(int x, int y) {

    auto parent_x = find(x);
    auto parent_y = find(y);
    if (parent_x == parent_y) {
      return;
    }

    parent[parent_y] = parent_x;
  }

  bool connected(int x, int y) { return find(x) == find(y); }

  std::vector<int> parent;
};
