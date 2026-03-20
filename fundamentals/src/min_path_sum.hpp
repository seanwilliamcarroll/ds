#pragma once

#include <limits>
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

inline int min_path_sum(const std::vector<std::vector<int>> &grid) {
  std::vector<std::vector<int>> min_path_to_position(
      grid.size(),
      std::vector<int>(grid.front().size(), std::numeric_limits<int>::max()));
  min_path_to_position[0][0] = grid[0][0];

  for (size_t row_index = 0; row_index < grid.size(); ++row_index) {
    for (size_t col_index = 0; col_index < grid[row_index].size();
         ++col_index) {
      auto current_value = grid[row_index][col_index];
      if (row_index > 0) {
        min_path_to_position[row_index][col_index] = std::min(
            min_path_to_position[row_index - 1][col_index] + current_value,
            min_path_to_position[row_index][col_index]);
      }
      if (col_index > 0) {
        min_path_to_position[row_index][col_index] = std::min(
            min_path_to_position[row_index][col_index - 1] + current_value,
            min_path_to_position[row_index][col_index]);
      }
    }
  }

  return min_path_to_position.back().back();
}
