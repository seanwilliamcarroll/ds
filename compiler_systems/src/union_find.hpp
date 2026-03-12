#pragma once

// Union-Find (Disjoint Set Union)
//
// TODO: Implement with path compression + union by rank.

class UnionFind {
public:
  explicit UnionFind(int n) {
    (void)n; // TODO
  }

  int find(int x) {
    (void)x; // TODO
    return -1;
  }

  void unite(int x, int y) {
    (void)x; // TODO
    (void)y;
  }

  bool connected(int x, int y) { return find(x) == find(y); }
};
