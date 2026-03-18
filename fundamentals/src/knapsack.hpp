#pragma once

#include <vector>

// 0/1 Knapsack
//
// Given n items, each with a weight and a value, and a knapsack with a maximum
// weight capacity, return the maximum total value you can carry. Each item can
// be used at most once (0/1 choice: take it or leave it).
//
// Example 1:
//   weights = {1, 3, 4, 5}, values = {1, 4, 5, 7}, capacity = 7
//   Answer: 9 (items with weight 3+4=7, value 4+5=9)
//
// Example 2:
//   weights = {2, 2, 3}, values = {1, 1, 2}, capacity = 4
//   Answer: 2 (take the item with weight 3, value 2)
//     Note: taking both weight-2 items gives value 2 as well, either works
//
// Example 3:
//   weights = {5}, values = {10}, capacity = 4
//   Answer: 0 (item doesn't fit)
//
// Example 4:
//   weights = {1, 2, 3}, values = {6, 10, 12}, capacity = 5
//   Answer: 22 (items with weight 2+3=5, value 10+12=22)
//
// Constraints:
//   - 1 <= n <= 100
//   - 1 <= weights[i] <= 1000
//   - 1 <= values[i] <= 1000
//   - 1 <= capacity <= 1000
//   - weights.size() == values.size()

inline int knapsack(const std::vector<int> &weights,
                    const std::vector<int> &values, int capacity) {
  return -1; // TODO
}
