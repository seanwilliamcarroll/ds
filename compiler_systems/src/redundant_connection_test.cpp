#include "redundant_connection.hpp"

#include <gtest/gtest.h>

TEST(RedundantConnectionTest, Triangle) {
  // Tree: 1-2, 1-3. Extra edge: 2-3
  std::vector<std::pair<int, int>> edges = {{1, 2}, {1, 3}, {2, 3}};
  auto result = find_redundant_connection(edges);
  EXPECT_EQ(result, std::make_pair(2, 3));
}

TEST(RedundantConnectionTest, FourNodeCycle) {
  // Tree: 1-2, 2-3, 3-4. Extra edge: 1-4
  std::vector<std::pair<int, int>> edges = {
      {1, 2}, {2, 3}, {3, 4}, {1, 4}, {1, 5}};
  auto result = find_redundant_connection(edges);
  EXPECT_EQ(result, std::make_pair(1, 4));
}

TEST(RedundantConnectionTest, LastEdgeRedundant) {
  // Linear tree with final edge closing the loop
  std::vector<std::pair<int, int>> edges = {
      {1, 2}, {2, 3}, {3, 4}, {4, 5}, {1, 5}};
  auto result = find_redundant_connection(edges);
  EXPECT_EQ(result, std::make_pair(1, 5));
}

TEST(RedundantConnectionTest, FirstEdgeRedundant) {
  // The redundant edge appears early — still must be the one that
  // would create a cycle given the processing order
  std::vector<std::pair<int, int>> edges = {{1, 2}, {1, 2}, {2, 3}};
  auto result = find_redundant_connection(edges);
  EXPECT_EQ(result, std::make_pair(1, 2));
}

TEST(RedundantConnectionTest, LargerGraph) {
  std::vector<std::pair<int, int>> edges = {{1, 2}, {2, 3}, {3, 4},
                                            {4, 5}, {5, 6}, {6, 3}};
  auto result = find_redundant_connection(edges);
  EXPECT_EQ(result, std::make_pair(6, 3));
}
