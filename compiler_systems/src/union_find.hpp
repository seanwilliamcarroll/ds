#pragma once

#include <vector>

// Union-Find (Disjoint Set Union)

class UnionFind {
public:
  explicit UnionFind(int n) : parent(n, 0), rank(n, 0) {
    for (int node = 0; node < n; ++node) {
      parent[node] = node;
    }
  }

  int find(int x) {
    int root_node = x;
    while (parent[root_node] != root_node) {
      root_node = parent[root_node];
    }
    while (parent[x] != root_node) {
      int next_node = parent[x];
      parent[x] = root_node;
      x = next_node;
    }
    return root_node;
  }

  void unite(int x, int y) {
    auto parent_x = find(x);
    auto parent_y = find(y);
    if (parent_x == parent_y) {
      return;
    }

    if (rank[parent_x] < rank[parent_y]) {
      parent[parent_x] = parent_y;
    } else if (rank[parent_x] > rank[parent_y]) {
      parent[parent_y] = parent_x;
    } else {
      parent[parent_x] = parent_y;
      rank[parent_y]++;
    }
  }

  bool connected(int x, int y) { return find(x) == find(y); }

  std::vector<int> parent;
  std::vector<int> rank;
};
