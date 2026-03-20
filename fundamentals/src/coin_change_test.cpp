#include "coin_change.hpp"

#include <gtest/gtest.h>

TEST(CoinChange, GreedyFails) {
  // Greedy picks 11 + 1+1+1+1 = 5 coins, but 5+5+5 = 3 coins is optimal
  EXPECT_EQ(coin_change({1, 5, 11}, 15), 3);
}

TEST(CoinChange, Impossible) { EXPECT_EQ(coin_change({2}, 3), -1); }

TEST(CoinChange, ZeroAmount) { EXPECT_EQ(coin_change({1}, 0), 0); }

TEST(CoinChange, SingleCoin) { EXPECT_EQ(coin_change({1}, 7), 7); }

TEST(CoinChange, ExactFit) {
  // 3 + 3 + 5 = 11? No. 1 + 5 + 5 = 11. That's 3 coins.
  // Or 1 + 1 + 1 + 1 + 1 + 1 + 5 = 11. That's 7.
  // Best: 5 + 5 + 1 = 3 coins.
  EXPECT_EQ(coin_change({1, 3, 5}, 11), 3);
}

TEST(CoinChange, LargeAmount) {
  // 100 pennies, or 20 nickels, or 10 dimes, or 4 quarters
  // 4 quarters = 100. That's 4 coins.
  EXPECT_EQ(coin_change({1, 5, 10, 25}, 100), 4);
}

TEST(CoinChange, SingleLargeCoin) { EXPECT_EQ(coin_change({5}, 15), 3); }

TEST(CoinChange, SingleLargeCoinImpossible) {
  EXPECT_EQ(coin_change({5}, 3), -1);
}

TEST(CoinChange, TwoCoins) {
  // 3 + 3 + 3 + 3 + 3 + 3 + 2 = 20? No, 3*6=18+2=20, that's 7 coins.
  // 2*10 = 20, that's 10 coins.
  // 3*4 + 2*4 = 12 + 8 = 20, that's 8 coins.
  // Best: 3*6 + 2 = 7 coins.
  EXPECT_EQ(coin_change({2, 3}, 20), 7);
}

TEST(CoinChange, LargeAmountMaxConstraint) {
  // amount = 10000 (max constraint), coins = {1, 5, 10, 25}
  // 10000 / 25 = 400 quarters exactly
  EXPECT_EQ(coin_change({1, 5, 10, 25}, 10000), 400);
}

TEST(CoinChange, LargeAmountGreedyFails) {
  // Greedy picks 186 + 7 + 7 + 7 + 7 = 214, needs 4 sevens to fill the gap.
  // That's 5 coins. But 186 + 28 = 214 and 28 = 7*4, so 1 + 4 = 5 coins.
  // Actually: can we do better? 7*30 = 210, need 4 more, no coin for 4. No.
  // 186 + 7*4 = 186 + 28 = 214. 5 coins.
  // But 7*30 = 210, remainder 4, can't make 4 from {7, 186}. So -1? No, 186*1 +
  // 7*4 = 214. What about 186*0? 214/7 = 30.57, not exact. So must use at least
  // one 186. 214 - 186 = 28 = 7*4. Total: 5 coins.
  EXPECT_EQ(coin_change({7, 186}, 214), 5);
}

TEST(CoinChange, LargeAmountManyCoinTypes) {
  // 12 denominations (max constraint on coins.length)
  // 9999 = 499*17 + 416? Let's just trust the DP and verify it terminates.
  // With coin=1 present, it's always solvable.
  // Best will heavily use the largest coins: 9999/97 = 103 remainder 8, so 103
  // + 8 = 111 But DP might find better combos with mid-range coins.
  int result = coin_change({1, 3, 7, 11, 17, 23, 31, 43, 59, 67, 83, 97}, 9999);
  EXPECT_GT(result, 0);
  // Greedy upper bound: 9999/97 = 103, remainder 8 = 1*8, total 111
  // DP should do at least as well as greedy
  EXPECT_LE(result, 111);
}

TEST(CoinChange, LargeImpossible) {
  // Large amount, no coin=1, and amount not reachable
  // 9999 is odd, but both coins are even — impossible
  EXPECT_EQ(coin_change({2, 4, 6, 8, 10}, 9999), -1);
}

TEST(CoinChange, CoprimeCoins) {
  // coins {3, 7} are coprime, so all sufficiently large amounts are reachable
  // (Chicken McNugget theorem: largest unreachable = 3*7 - 3 - 7 = 11)
  // 100 = 7*11 + 3*1 = 78 coins? No: 7*11 = 77, 100-77 = 23, 23/3 = 7r2. No.
  // 7*10 = 70, 30/3 = 10. Total 20.
  // 7*7 = 49, 51/3 = 17. Total 24.
  // 7*13 = 91, 9/3 = 3. Total 16.
  // Best: 16 coins (7*13 + 3*3)
  EXPECT_EQ(coin_change({3, 7}, 100), 16);
}
