#pragma once

#include <vector>

// House Robber (LeetCode 198)
//
// You are a robber planning to rob houses along a street. Each house has a
// certain amount of money stashed. Adjacent houses have connected security
// systems — if two adjacent houses are broken into on the same night, the
// police are automatically contacted.
//
// Given a vector of non-negative integers representing the amount of money
// in each house, return the maximum amount you can rob without alerting the
// police (i.e., no two adjacent houses).
//
// Example 1:
//   nums = {1, 2, 3, 1}
//   Answer: 4 (rob house 0 and house 2: 1 + 3)
//
// Example 2:
//   nums = {2, 7, 9, 3, 1}
//   Answer: 12 (rob house 0, 2, 4: 2 + 9 + 1)
//
// Example 3:
//   nums = {2, 1, 1, 2}
//   Answer: 4 (rob house 0 and house 3: 2 + 2)
//   Note: greedy "pick the largest" would take 2 (house 0), then can't take
//   house 1, takes 1 (house 2), misses house 3. Gets 3, not 4.
//
// Constraints:
//   - 1 <= nums.size() <= 100
//   - 0 <= nums[i] <= 400
//
// Approach:
//   State:      dp[i] = max money robable from houses 0..i
//   Recurrence: dp[i] = max(dp[i-1], dp[i-2] + nums[i])
//                        ^^ skip i    ^^ take i
//   Base cases: dp[0] = nums[0], dp[1] = max(nums[0], nums[1])
//   Answer:     dp[n-1]
//
//   The recurrence only looks back 2 steps, so space can be optimized
//   from O(n) to O(1) using two variables.
//
//   Time:  O(n)
//   Space: O(1) with the two-variable optimization, O(n) with full table

inline int rob(const std::vector<int> &nums) {
  // std::vector<int> max_money_robable(nums.size(), 0);

  auto two_houses_back = nums[0];
  if (nums.size() == 1) {
    return two_houses_back;
  }
  auto one_house_back = std::max(nums[0], nums[1]);
  if (nums.size() == 2) {
    return one_house_back;
  }

  for (size_t house_index = 2; house_index < nums.size(); ++house_index) {
    auto current_house_value = nums[house_index];
    auto original_one_house_back = one_house_back;

    one_house_back =
        std::max(one_house_back, two_houses_back + current_house_value);
    two_houses_back = original_one_house_back;
  }

  return std::max(two_houses_back, one_house_back);
}
