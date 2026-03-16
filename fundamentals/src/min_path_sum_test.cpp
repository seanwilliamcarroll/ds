#include "min_path_sum.hpp"

#include <gtest/gtest.h>

TEST(MinPathSum, LeetCodeExample1) {
  EXPECT_EQ(minPathSum({{1, 3, 1}, {1, 5, 1}, {4, 2, 1}}), 7);
}

TEST(MinPathSum, LeetCodeExample2) {
  EXPECT_EQ(minPathSum({{1, 2, 3}, {4, 5, 6}}), 12);
}

TEST(MinPathSum, SingleCell) { EXPECT_EQ(minPathSum({{5}}), 5); }

TEST(MinPathSum, SingleRow) {
  // Only one path: right, right, right
  EXPECT_EQ(minPathSum({{1, 2, 3, 4}}), 10);
}

TEST(MinPathSum, SingleColumn) {
  // Only one path: down, down, down
  EXPECT_EQ(minPathSum({{1}, {2}, {3}, {4}}), 10);
}

TEST(MinPathSum, AllZeros) {
  EXPECT_EQ(minPathSum({{0, 0, 0}, {0, 0, 0}, {0, 0, 0}}), 0);
}

TEST(MinPathSum, PreferDownThenRight) {
  // Going down first avoids the expensive top row
  //   1  100  100
  //   1    1    1
  //   1    1    1
  // Best path: down, down, right, right = 1+1+1+1+1 = 5
  EXPECT_EQ(minPathSum({{1, 100, 100}, {1, 1, 1}, {1, 1, 1}}), 5);
}

TEST(MinPathSum, PreferRightThenDown) {
  // Going right first avoids the expensive left column
  //   1    1    1
  //   100  1    1
  //   100  1    1
  // Best path: right, right, down, down = 1+1+1+1+1 = 5
  EXPECT_EQ(minPathSum({{1, 1, 1}, {100, 1, 1}, {100, 1, 1}}), 5);
}

TEST(MinPathSum, LargerGrid) {
  // 5x5 grid where the optimal path isn't obvious
  std::vector<std::vector<int>> grid = {
      {1, 3, 1, 2, 9}, {7, 1, 1, 3, 1}, {8, 2, 1, 1, 1},
      {4, 5, 2, 1, 3}, {1, 1, 9, 1, 1},
  };
  // Path: 1->3->1->1->1->1->1->1->1 = 11
  // (right, down, right, right, down, down, right, down)
  // Actually let's trace: (0,0)=1 -> (0,1)=3 -> (1,1)=1 -> (1,2)=1 ->
  // (2,2)=1 -> (2,3)=1 -> (2,4)=1 -> (3,4)=3 -> (4,4)=1 = 13
  // Or: (0,0)=1 -> (0,1)=3 -> (1,1)=1 -> (1,2)=1 -> (2,2)=1 ->
  // (3,2)=2 -> (3,3)=1 -> (4,3)=1 -> (4,4)=1 = 12
  EXPECT_EQ(minPathSum(grid), 12);
}

TEST(MinPathSum, ExpensiveCenter) {
  // Force path around an expensive center
  //   1   1   1   1
  //   1  99  99   1
  //   1  99  99   1
  //   1   1   1   1
  // Top path: right,right,right,down,down,down = 1+1+1+1+1+1+1 = 7
  // Bottom path: down,down,down,right,right,right = 1+1+1+1+1+1+1 = 7
  EXPECT_EQ(
      minPathSum({{1, 1, 1, 1}, {1, 99, 99, 1}, {1, 99, 99, 1}, {1, 1, 1, 1}}),
      7);
}
