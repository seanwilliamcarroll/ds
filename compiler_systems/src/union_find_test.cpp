#include "union_find.hpp"

#include <gtest/gtest.h>

// Test all four variants:
// UnionFind<false, false> — no optimizations
// UnionFind<true, false>  — rank only
// UnionFind<false, true>  — compression only
// UnionFind<true, true>   — both
using UF_None = UnionFind<false, false>;
using UF_RankOnly = UnionFind<true, false>;
using UF_CompressionOnly = UnionFind<false, true>;
using UF_Both = UnionFind<true, true>;

template <typename T> class UnionFindTest : public ::testing::Test {};

using UnionFindTypes =
    ::testing::Types<UF_None, UF_RankOnly, UF_CompressionOnly, UF_Both>;
TYPED_TEST_SUITE(UnionFindTest, UnionFindTypes);

TYPED_TEST(UnionFindTest, HelloWorld) {
  TypeParam uf(5);
  // Sanity check: nothing is connected yet
  EXPECT_FALSE(uf.connected(0, 1));
}

TYPED_TEST(UnionFindTest, BasicUnion) {
  TypeParam uf(5);
  uf.unite(0, 1);
  EXPECT_TRUE(uf.connected(0, 1));
  EXPECT_FALSE(uf.connected(0, 2));
}

TYPED_TEST(UnionFindTest, SelfUniteAndConnected) {
  TypeParam uf(3);
  uf.unite(0, 0);
  EXPECT_TRUE(uf.connected(0, 0));
  EXPECT_TRUE(uf.connected(1, 1));
  EXPECT_FALSE(uf.connected(0, 1));
}

TYPED_TEST(UnionFindTest, ChainConnectivity) {
  // Build a chain: 0-1-2-3-4
  // All nodes should end up connected, and find should return
  // a consistent root for all of them.
  TypeParam uf(5);
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

TYPED_TEST(UnionFindTest, MergeTwoGroups) {
  // Build two separate groups, then merge them
  TypeParam uf(6);
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

TYPED_TEST(UnionFindTest, ConnectedComponents) {
  // n=7, edges: {0,1}, {1,2}, {3,4}, {5,6}
  // Expected: 3 components: {0,1,2}, {3,4}, {5,6}
  TypeParam uf(7);
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
