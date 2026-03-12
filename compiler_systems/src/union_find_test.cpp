#include "union_find.hpp"

#include <gtest/gtest.h>

// Tests run against the fully-optimized variant
using UF = UnionFind<true, true>;

TEST(UnionFindTest, HelloWorld) {
  UF uf(5);
  // Sanity check: nothing is connected yet
  EXPECT_FALSE(uf.connected(0, 1));
}

TEST(UnionFindTest, BasicUnion) {
  UF uf(5);
  uf.unite(0, 1);
  EXPECT_TRUE(uf.connected(0, 1));
  EXPECT_FALSE(uf.connected(0, 2));
}

TEST(UnionFindTest, SelfUniteAndConnected) {
  UF uf(3);
  uf.unite(0, 0);
  EXPECT_TRUE(uf.connected(0, 0));
  EXPECT_TRUE(uf.connected(1, 1));
  EXPECT_FALSE(uf.connected(0, 1));
}

TEST(UnionFindTest, ChainConnectivity) {
  // Build a chain: 0-1-2-3-4
  // All nodes should end up connected, and find should return
  // a consistent root for all of them.
  UF uf(5);
  uf.unite(0, 1);
  uf.unite(1, 2);
  uf.unite(2, 3);
  uf.unite(3, 4);

  // All pairs connected
  for (int i = 0; i < 5; ++i) {
    for (int j = i + 1; j < 5; ++j) {
      EXPECT_TRUE(uf.connected(i, j));
    }
  }

  // All finds return the same root
  int root = uf.find(0);
  for (int i = 1; i < 5; ++i) {
    EXPECT_EQ(uf.find(i), root);
  }
}

TEST(UnionFindTest, MergeTwoGroups) {
  // Build two separate groups, then merge them
  UF uf(6);
  uf.unite(0, 1);
  uf.unite(1, 2);
  uf.unite(3, 4);
  uf.unite(4, 5);

  // Groups are separate
  EXPECT_FALSE(uf.connected(0, 3));
  EXPECT_FALSE(uf.connected(2, 5));

  // Bridge the groups
  uf.unite(2, 3);

  // Now everything is connected
  for (int i = 0; i < 6; ++i) {
    for (int j = i + 1; j < 6; ++j) {
      EXPECT_TRUE(uf.connected(i, j));
    }
  }
}

TEST(UnionFindTest, ConnectedComponents) {
  // n=7, edges: {0,1}, {1,2}, {3,4}, {5,6}
  // Expected: 3 components: {0,1,2}, {3,4}, {5,6}
  UF uf(7);
  uf.unite(0, 1);
  uf.unite(1, 2);
  uf.unite(3, 4);
  uf.unite(5, 6);

  // Count roots: a node is a root if find(i) == i
  int components = 0;
  for (int i = 0; i < 7; ++i) {
    if (uf.find(i) == i) {
      components++;
    }
  }
  EXPECT_EQ(components, 3);
}
