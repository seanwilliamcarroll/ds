#include "knapsack.hpp"

#include <gtest/gtest.h>

TEST(Knapsack, BasicExample) {
  EXPECT_EQ(knapsack({1, 3, 4, 5}, {1, 4, 5, 7}, 7), 9);
}

TEST(Knapsack, TiedChoices) {
  // Both weight-2 items (value 2) or the weight-3 item (value 2) — same result
  EXPECT_EQ(knapsack({2, 2, 3}, {1, 1, 2}, 4), 2);
}

TEST(Knapsack, NothingFits) { EXPECT_EQ(knapsack({5}, {10}, 4), 0); }

TEST(Knapsack, TakeLastTwo) {
  EXPECT_EQ(knapsack({1, 2, 3}, {6, 10, 12}, 5), 22);
}

TEST(Knapsack, SingleItemFits) { EXPECT_EQ(knapsack({3}, {7}, 3), 7); }

TEST(Knapsack, TakeAll) {
  // Everything fits
  EXPECT_EQ(knapsack({1, 2, 3}, {10, 20, 30}, 10), 60);
}

TEST(Knapsack, GreedyByValueFails) {
  // Greedy by value/weight ratio picks item 0 (ratio 10), then item 1 (ratio 5)
  // for total weight 3, value 20. But taking items 1+2 gives weight 5,
  // value 24.
  EXPECT_EQ(knapsack({1, 2, 3}, {10, 10, 14}, 5), 24);
}

TEST(Knapsack, GreedyByWeightFails) {
  // Greedy by lightest first picks items 0,1,2 (weight 6, value 6).
  // Optimal is items 2+3 (weight 7, value 11).
  EXPECT_EQ(knapsack({1, 2, 3, 4}, {1, 2, 3, 8}, 7), 11);
}

TEST(Knapsack, ExactCapacity) {
  // Items sum to exactly the capacity
  EXPECT_EQ(knapsack({2, 3, 5}, {3, 4, 8}, 10), 15);
}

TEST(Knapsack, ZeroCapacity) {
  EXPECT_EQ(knapsack({1, 2, 3}, {10, 20, 30}, 0), 0);
}

TEST(Knapsack, LargeCapacitySmallItems) {
  // Capacity much larger than all items combined — take everything
  EXPECT_EQ(knapsack({1, 1, 1, 1}, {5, 5, 5, 5}, 100), 20);
}

TEST(Knapsack, HeavyHighValue) {
  // One heavy, high-value item vs several light, low-value items
  // Heavy item: weight 8, value 100. Light items: total weight 8, total
  // value 10.
  EXPECT_EQ(knapsack({8, 2, 2, 2, 2}, {100, 2, 3, 2, 3}, 8), 100);
}

TEST(Knapsack, ManyItems) {
  // 10 items, need to pick the right subset
  std::vector<int> weights = {23, 31, 29, 44, 53, 38, 63, 85, 89, 82};
  std::vector<int> values = {92, 57, 49, 68, 60, 43, 67, 84, 87, 72};
  // Known optimal for capacity 165: items 0,1,2,3,5 → weight 165, value 309
  EXPECT_EQ(knapsack(weights, values, 165), 309);
}
