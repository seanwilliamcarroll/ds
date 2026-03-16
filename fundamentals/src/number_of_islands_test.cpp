#include "number_of_islands.hpp"

#include <gtest/gtest.h>

TEST(NumberOfIslands, LeetCodeExample1) {
  std::vector<std::vector<char>> grid = {
      {'1', '1', '1', '1', '0'},
      {'1', '1', '0', '1', '0'},
      {'1', '1', '0', '0', '0'},
      {'0', '0', '0', '0', '0'},
  };
  EXPECT_EQ(numIslands(grid), 1);
}

TEST(NumberOfIslands, LeetCodeExample2) {
  std::vector<std::vector<char>> grid = {
      {'1', '1', '0', '0', '0'},
      {'1', '1', '0', '0', '0'},
      {'0', '0', '1', '0', '0'},
      {'0', '0', '0', '1', '1'},
  };
  EXPECT_EQ(numIslands(grid), 3);
}

TEST(NumberOfIslands, AllWater) {
  std::vector<std::vector<char>> grid = {
      {'0', '0', '0'},
      {'0', '0', '0'},
      {'0', '0', '0'},
  };
  EXPECT_EQ(numIslands(grid), 0);
}

TEST(NumberOfIslands, AllLand) {
  std::vector<std::vector<char>> grid = {
      {'1', '1', '1'},
      {'1', '1', '1'},
      {'1', '1', '1'},
  };
  EXPECT_EQ(numIslands(grid), 1);
}

TEST(NumberOfIslands, SingleCell) {
  std::vector<std::vector<char>> land = {{'1'}};
  EXPECT_EQ(numIslands(land), 1);

  std::vector<std::vector<char>> water = {{'0'}};
  EXPECT_EQ(numIslands(water), 0);
}

TEST(NumberOfIslands, SingleRow) {
  std::vector<std::vector<char>> grid = {{'1', '0', '1', '0', '1'}};
  EXPECT_EQ(numIslands(grid), 3);
}

TEST(NumberOfIslands, SingleColumn) {
  std::vector<std::vector<char>> grid = {{'1'}, {'0'}, {'1'}, {'0'}, {'1'}};
  EXPECT_EQ(numIslands(grid), 3);
}

TEST(NumberOfIslands, DiagonalNotConnected) {
  // Diagonal cells are NOT adjacent — these are separate islands
  std::vector<std::vector<char>> grid = {
      {'1', '0', '0'},
      {'0', '1', '0'},
      {'0', '0', '1'},
  };
  EXPECT_EQ(numIslands(grid), 3);
}

TEST(NumberOfIslands, LShapedIsland) {
  std::vector<std::vector<char>> grid = {
      {'1', '0', '0'},
      {'1', '0', '0'},
      {'1', '1', '1'},
  };
  EXPECT_EQ(numIslands(grid), 1);
}

TEST(NumberOfIslands, IslandsWithNarrowChannels) {
  // Two islands separated by a single column of water
  std::vector<std::vector<char>> grid = {
      {'1', '1', '0', '1', '1'},
      {'1', '1', '0', '1', '1'},
  };
  EXPECT_EQ(numIslands(grid), 2);
}
