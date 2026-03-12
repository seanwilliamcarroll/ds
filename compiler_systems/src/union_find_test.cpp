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
