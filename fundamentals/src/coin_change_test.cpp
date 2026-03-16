#include "coin_change.hpp"

#include <gtest/gtest.h>

TEST(CoinChange, GreedyFails) {
  // Greedy picks 11 + 1+1+1+1 = 5 coins, but 5+5+5 = 3 coins is optimal
  EXPECT_EQ(coinChange({1, 5, 11}, 15), 3);
}

TEST(CoinChange, Impossible) { EXPECT_EQ(coinChange({2}, 3), -1); }

TEST(CoinChange, ZeroAmount) { EXPECT_EQ(coinChange({1}, 0), 0); }

TEST(CoinChange, SingleCoin) { EXPECT_EQ(coinChange({1}, 7), 7); }

TEST(CoinChange, ExactFit) {
  // 3 + 3 + 5 = 11? No. 1 + 5 + 5 = 11. That's 3 coins.
  // Or 1 + 1 + 1 + 1 + 1 + 1 + 5 = 11. That's 7.
  // Best: 5 + 5 + 1 = 3 coins.
  EXPECT_EQ(coinChange({1, 3, 5}, 11), 3);
}

TEST(CoinChange, LargeAmount) {
  // 100 pennies, or 20 nickels, or 10 dimes, or 4 quarters
  // 4 quarters = 100. That's 4 coins.
  EXPECT_EQ(coinChange({1, 5, 10, 25}, 100), 4);
}

TEST(CoinChange, SingleLargeCoin) { EXPECT_EQ(coinChange({5}, 15), 3); }

TEST(CoinChange, SingleLargeCoinImpossible) {
  EXPECT_EQ(coinChange({5}, 3), -1);
}

TEST(CoinChange, TwoCoins) {
  // 3 + 3 + 3 + 3 + 3 + 3 + 2 = 20? No, 3*6=18+2=20, that's 7 coins.
  // 2*10 = 20, that's 10 coins.
  // 3*4 + 2*4 = 12 + 8 = 20, that's 8 coins.
  // Best: 3*6 + 2 = 7 coins.
  EXPECT_EQ(coinChange({2, 3}, 20), 7);
}
