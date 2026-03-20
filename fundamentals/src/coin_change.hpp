#pragma once

#include <cstddef>
#include <vector>

// Coin Change (LeetCode 322)
//
// Given an array of coin denominations and a target amount, return the fewest
// number of coins needed to make up that amount. If it's not possible, return
// -1. You have an infinite supply of each coin denomination.
//
// Example 1:
//   coins = {1, 5, 11}, amount = 15
//   Answer: 3 (5 + 5 + 5)
//   Note: the greedy approach (take the largest coin first) would pick
//   11 + 1 + 1 + 1 + 1 = 5 coins. Greedy fails here because locally
//   optimal choices don't lead to a globally optimal solution.
//
// Example 2:
//   coins = {2}, amount = 3
//   Answer: -1 (no combination of 2s sums to 3)
//
// Example 3:
//   coins = {1}, amount = 0
//   Answer: 0 (no coins needed to make amount 0)
//
// Constraints:
//   - 1 <= coins.length <= 12
//   - 1 <= coins[i] <= 2^31 - 1
//   - 0 <= amount <= 10^4
//
// Approach:
//   State:    dp[i] = minimum coins needed to make amount i
//   Recurrence: dp[i] = min(dp[i - coin] + 1) for each coin where coin <= i
//   Base case:  dp[0] = 0 (zero coins to make zero amount)
//   Answer:     dp[amount], or -1 if dp[amount] was never reached
//
//   This is an unbounded knapsack variant — each coin can be used any number
//   of times, and we minimize the count rather than maximizing a value.
//
//   Time:  O(amount * coins.size())
//   Space: O(amount)

inline int coin_change(const std::vector<int> &coins, int amount) {
  const auto amount_index = static_cast<size_t>(amount);
  std::vector<int> coins_needed(amount_index + 1, amount + 1);
  coins_needed[0] = 0;

  for (size_t index = 0; index <= amount_index; ++index) {
    const auto current_amount = static_cast<int>(index);
    for (const auto coin : coins) {
      if (coin > current_amount) {
        // coin too big to try
        continue;
      }
      // Can try this coin
      const auto coin_as_index = static_cast<size_t>(coin);
      const auto last_number_coins = coins_needed[index - coin_as_index];
      if (last_number_coins == -1) {
        // Can't make this value with this coin
        continue;
      }
      // First time finding this coin?
      // or
      // Did we find a way with fewer coins?
      coins_needed[index] =
          std::min(last_number_coins + 1, coins_needed[index]);
    }
  }

  if (coins_needed[amount_index] > amount) {
    return -1;
  }

  return coins_needed[amount_index];
}
