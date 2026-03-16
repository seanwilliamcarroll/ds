#include "house_robber.hpp"

#include <gtest/gtest.h>

TEST(HouseRobber, BasicSkipMiddle) {
  // Rob house 0 and 2: 1 + 3 = 4
  EXPECT_EQ(rob({1, 2, 3, 1}), 4);
}

TEST(HouseRobber, SkipAlternating) {
  // Rob house 0, 2, 4: 2 + 9 + 1 = 12
  EXPECT_EQ(rob({2, 7, 9, 3, 1}), 12);
}

TEST(HouseRobber, GreedyFails) {
  // Greedy "pick largest" takes house 1 (7), then can't take 0 or 2.
  // Takes house 3 (4). Gets 11. But optimal is 0+2+4: 6+5+6 = 17? No...
  // Actually: {2, 1, 1, 2}. Greedy picks 2 (house 0), skips 1, picks 1
  // (house 2), skips 3. Gets 3. Optimal: house 0 + house 3 = 2 + 2 = 4.
  EXPECT_EQ(rob({2, 1, 1, 2}), 4);
}

TEST(HouseRobber, SingleHouse) { EXPECT_EQ(rob({5}), 5); }

TEST(HouseRobber, TwoHouses) {
  // Can only pick one — pick the larger
  EXPECT_EQ(rob({3, 7}), 7);
}

TEST(HouseRobber, AllEqual) {
  // {4, 4, 4, 4, 4} — rob every other: houses 0, 2, 4 = 12
  EXPECT_EQ(rob({4, 4, 4, 4, 4}), 12);
}

TEST(HouseRobber, AllZeros) { EXPECT_EQ(rob({0, 0, 0, 0}), 0); }

TEST(HouseRobber, LargeValueInMiddle) {
  // {1, 100, 1} — obviously take the middle house
  EXPECT_EQ(rob({1, 100, 1}), 100);
}

TEST(HouseRobber, SkipTwoInARow) {
  // Sometimes optimal to skip two consecutive houses
  // {10, 1, 1, 10} — take house 0 and 3: 10 + 10 = 20
  EXPECT_EQ(rob({10, 1, 1, 10}), 20);
}

TEST(HouseRobber, LongStreet) {
  // {5, 1, 1, 5, 1, 1, 5, 1, 1, 5}
  // Take houses 0, 3, 6, 9: 5+5+5+5 = 20
  EXPECT_EQ(rob({5, 1, 1, 5, 1, 1, 5, 1, 1, 5}), 20);
}
