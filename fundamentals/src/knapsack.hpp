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

  std::vector<int> value_by_capacity_so_far(static_cast<size_t>(capacity + 1),
                                            0);

  for (size_t item_index = 0; item_index < weights.size(); ++item_index) {
    auto item_value = values[item_index];
    auto item_weight = weights[item_index];
    for (int capacity_so_far = capacity; capacity_so_far >= item_weight;
         --capacity_so_far) {
      // We're deciding here if we are taking item_index or not
      // By iterating backwards, we can treat the lower weight values as the
      // same as the "previous row", so no need for an actual previous row
      auto capacity_so_far_as_index = static_cast<size_t>(capacity_so_far);
      auto skip_item_value = value_by_capacity_so_far[capacity_so_far_as_index];
      auto item_weight_as_index = static_cast<size_t>(item_weight);
      auto take_item_value = value_by_capacity_so_far[capacity_so_far_as_index -
                                                      item_weight_as_index] +
                             item_value;
      if (take_item_value > skip_item_value) {
        value_by_capacity_so_far[capacity_so_far_as_index] = take_item_value;
      }
    }
  }

  return value_by_capacity_so_far.back();
}
