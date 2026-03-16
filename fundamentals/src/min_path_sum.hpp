#pragma once

#include <vector>

// Minimum Path Sum (LeetCode 64)
//
// Given an m x n grid filled with non-negative integers, find a path from the
// top-left corner to the bottom-right corner that minimizes the sum of all
// numbers along the path.
//
// You can only move right or down at each step.
//
// Example 1:
//   grid = {{1, 3, 1},
//           {1, 5, 1},
//           {4, 2, 1}}
//   Answer: 7
//   Path: 1 -> 3 -> 1 -> 1 -> 1 = 7
//   (right, right, down, down)
//
// Example 2:
//   grid = {{1, 2, 3},
//           {4, 5, 6}}
//   Answer: 12
//   Path: 1 -> 2 -> 3 -> 6 = 12
//   (right, right, down)
//
// Constraints:
//   - 1 <= m, n <= 200
//   - 0 <= grid[i][j] <= 200

int minPathSum(const std::vector<std::vector<int>> &grid);
