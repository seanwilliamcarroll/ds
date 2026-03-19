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

  std::vector<int> max_value_so_far(capacity + 1, 0);
  std::vector<std::vector<bool>> items_taken_so_far(
      capacity + 1, std::vector<bool>(weights.size(), false));

  for (int capacity_here = 1; capacity_here < capacity + 1; ++capacity_here) {
    size_t new_item_index = weights.size();

    // Preload to haven taken no new items
    max_value_so_far[capacity_here] = max_value_so_far[capacity_here - 1];
    items_taken_so_far[capacity_here] = items_taken_so_far[capacity_here - 1];

    for (size_t item_index = 0; item_index < weights.size(); ++item_index) {
      if (weights[item_index] > capacity_here) {
        // Item too big to fit at all, don't bother looking
        continue;
      }

      // We want to check on the item as we look at the capacity minus this
      // weight, assuming we took it
      auto item_weight = weights[item_index];
      auto item_value = values[item_index];
      auto last_weight = capacity_here - item_weight;

      bool item_already_taken = items_taken_so_far[last_weight][item_index];

      if (item_already_taken) {
        // Can't take it again?
        continue;
      }

      auto last_value = max_value_so_far[last_weight];
      auto candidate_value = last_value + item_value;
      auto current_value = max_value_so_far[capacity_here];

      // Want to check if the value at the capacity directly before would be
      // greater than taking this item plus the value from item_weight ago
      if (candidate_value <= current_value) {
        // No sense taking this, doesn't improve things
        continue;
      }
      // We should take it
      new_item_index = item_index;
      max_value_so_far[capacity_here] = candidate_value;
    }
    if (new_item_index >= weights.size()) {
      // Not taking a new item at this capacity
      continue;
    }

    // We're taking a new item at new_item_index
    // Already updated the value, need to update the taken items
    // We're actually using the set from item_weight ago plus this new item
    items_taken_so_far[capacity_here] =
        items_taken_so_far[capacity_here - weights[new_item_index]];
    items_taken_so_far[capacity_here][new_item_index] = true;
  }

  return max_value_so_far.back();
}
