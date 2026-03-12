#include "union_find.hpp"

#include <gtest/gtest.h>

TEST(UnionFindTest, HelloWorld) {
  UnionFind uf(5);
  // Sanity check: nothing is connected yet
  EXPECT_FALSE(uf.connected(0, 1));
}

TEST(UnionFindTest, BasicUnion) {
  UnionFind uf(5);
  uf.unite(0, 1);
  EXPECT_TRUE(uf.connected(0, 1));
  EXPECT_FALSE(uf.connected(0, 2));
}

TEST(UnionFindTest, Example1) {
  UnionFind uf(5);
  uf.unite(0, 1);
  uf.unite(1, 2);
  uf.unite(3, 4);
  EXPECT_TRUE(uf.connected(0, 1));
  EXPECT_TRUE(uf.connected(0, 2));
  EXPECT_FALSE(uf.connected(0, 3));
  EXPECT_FALSE(uf.connected(4, 2));
}
