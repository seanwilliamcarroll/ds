#include "min_heap.hpp"

#include <gtest/gtest.h>

// ── empty / size
// ──────────────────────────────────────────────────────────────

TEST(MinHeap, StartsEmpty) {
  MinHeap<int> h;
  EXPECT_TRUE(h.empty());
  EXPECT_EQ(h.size(), 0U);
}

TEST(MinHeap, NotEmptyAfterPush) {
  MinHeap<int> h;
  h.push(1);
  EXPECT_FALSE(h.empty());
  EXPECT_EQ(h.size(), 1U);
}

TEST(MinHeap, SizeTracksInsertions) {
  MinHeap<int> h;
  for (int i = 0; i < 5; ++i) {
    h.push(i);
    EXPECT_EQ(h.size(), static_cast<size_t>(i + 1));
  }
}

TEST(MinHeap, SizeDecrementsOnPop) {
  MinHeap<int> h;
  h.push(1);
  h.push(2);
  h.push(3);
  h.pop();
  EXPECT_EQ(h.size(), 2U);
}

TEST(MinHeap, EmptyAfterAllPopped) {
  MinHeap<int> h;
  h.push(42);
  h.pop();
  EXPECT_TRUE(h.empty());
  EXPECT_EQ(h.size(), 0U);
}

// ── top ──────────────────────────────────────────────────────────────────────

TEST(MinHeap, TopSingleElement) {
  MinHeap<int> h;
  h.push(7);
  EXPECT_EQ(h.top(), 7);
}

TEST(MinHeap, TopIsMinAfterAscendingInserts) {
  MinHeap<int> h;
  h.push(1);
  h.push(2);
  h.push(3);
  EXPECT_EQ(h.top(), 1);
}

TEST(MinHeap, TopIsMinAfterDescendingInserts) {
  MinHeap<int> h;
  h.push(3);
  h.push(2);
  h.push(1);
  EXPECT_EQ(h.top(), 1);
}

TEST(MinHeap, TopDoesNotRemoveElement) {
  MinHeap<int> h;
  h.push(5);
  auto _ = h.top();
  EXPECT_EQ(h.size(), 1U);
  EXPECT_EQ(h.top(), 5);
}

// ── pop order
// ─────────────────────────────────────────────────────────────────

TEST(MinHeap, PopReturnsMinFirst) {
  MinHeap<int> h;
  h.push(3);
  h.push(1);
  h.push(2);
  EXPECT_EQ(h.pop(), 1);
}

TEST(MinHeap, PopAscendingOrder_InsertedAscending) {
  MinHeap<int> h;
  for (int i = 1; i <= 5; ++i) {
    h.push(i);
  }
  for (int i = 1; i <= 5; ++i) {
    EXPECT_EQ(h.pop(), i);
  }
}

TEST(MinHeap, PopAscendingOrder_InsertedDescending) {
  MinHeap<int> h;
  for (int i = 5; i >= 1; --i) {
    h.push(i);
  }
  for (int i = 1; i <= 5; ++i) {
    EXPECT_EQ(h.pop(), i);
  }
}

TEST(MinHeap, PopAscendingOrder_InsertedRandom) {
  MinHeap<int> h;
  for (int v : {4, 1, 7, 3, 9, 2, 6, 5, 8}) {
    h.push(v);
  }
  for (int i = 1; i <= 9; ++i) {
    EXPECT_EQ(h.pop(), i);
  }
}

TEST(MinHeap, DuplicateValues) {
  MinHeap<int> h;
  h.push(2);
  h.push(2);
  h.push(1);
  h.push(1);
  EXPECT_EQ(h.pop(), 1);
  EXPECT_EQ(h.pop(), 1);
  EXPECT_EQ(h.pop(), 2);
  EXPECT_EQ(h.pop(), 2);
}

TEST(MinHeap, AllSameValue) {
  MinHeap<int> h;
  h.push(5);
  h.push(5);
  h.push(5);
  EXPECT_EQ(h.pop(), 5);
  EXPECT_EQ(h.pop(), 5);
  EXPECT_EQ(h.pop(), 5);
  EXPECT_TRUE(h.empty());
}

// ── interleaved push/pop
// ──────────────────────────────────────────────────────

TEST(MinHeap, InterleavedPushPop) {
  MinHeap<int> h;
  h.push(5);
  h.push(3);
  EXPECT_EQ(h.pop(), 3);
  h.push(1);
  EXPECT_EQ(h.pop(), 1);
  EXPECT_EQ(h.pop(), 5);
  EXPECT_TRUE(h.empty());
}

TEST(MinHeap, PushAfterPop) {
  MinHeap<int> h;
  h.push(10);
  h.pop();
  h.push(3);
  h.push(1);
  EXPECT_EQ(h.top(), 1);
}

// ── negative and zero values
// ──────────────────────────────────────────────────

TEST(MinHeap, NegativeValues) {
  MinHeap<int> h;
  h.push(-1);
  h.push(-5);
  h.push(-3);
  EXPECT_EQ(h.pop(), -5);
  EXPECT_EQ(h.pop(), -3);
  EXPECT_EQ(h.pop(), -1);
}

TEST(MinHeap, MixedNegativeAndPositive) {
  MinHeap<int> h;
  for (int v : {3, -2, 0, -5, 1}) {
    h.push(v);
  }
  EXPECT_EQ(h.pop(), -5);
  EXPECT_EQ(h.pop(), -2);
  EXPECT_EQ(h.pop(), 0);
  EXPECT_EQ(h.pop(), 1);
  EXPECT_EQ(h.pop(), 3);
}

// ── larger input
// ──────────────────────────────────────────────────────────────

TEST(MinHeap, LargeInput) {
  MinHeap<int> h;
  const int N = 1000;
  // Insert in reverse order
  for (int i = N; i >= 1; --i) {
    h.push(i);
  }
  EXPECT_EQ(h.size(), static_cast<size_t>(N));
  for (int i = 1; i <= N; ++i) {
    EXPECT_EQ(h.pop(), i);
  }
  EXPECT_TRUE(h.empty());
}

// ── pair (Dijkstra-style usage)
// ───────────────────────────────────────────────

TEST(MinHeap, PairMinByFirst) {
  // Simulates Dijkstra: (distance, node)
  MinHeap<std::pair<int, int>> h;
  h.push({5, 3});
  h.push({1, 7});
  h.push({3, 2});
  EXPECT_EQ(h.pop(), (std::make_pair(1, 7)));
  EXPECT_EQ(h.pop(), (std::make_pair(3, 2)));
  EXPECT_EQ(h.pop(), (std::make_pair(5, 3)));
}
