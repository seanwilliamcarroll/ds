#include "chaining_hash_map.hpp"

#include <gtest/gtest.h>

// --- Basic operations ---

TEST(ChainingHashMap, EmptyOnConstruction) {
  ChainingHashMap map;
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
}

TEST(ChainingHashMap, InsertAndFind) {
  ChainingHashMap map;
  map.insert(1, 100);
  auto result = map.find(1);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 100);
}

TEST(ChainingHashMap, FindMissing) {
  ChainingHashMap map;
  EXPECT_FALSE(map.find(42).has_value());
}

TEST(ChainingHashMap, InsertUpdatesExistingKey) {
  ChainingHashMap map;
  map.insert(1, 100);
  map.insert(1, 200);
  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(*map.find(1), 200);
}

TEST(ChainingHashMap, SizeTracksInserts) {
  ChainingHashMap map;
  map.insert(1, 10);
  map.insert(2, 20);
  map.insert(3, 30);
  EXPECT_EQ(map.size(), 3);
  EXPECT_FALSE(map.empty());
}

// --- Erase ---

TEST(ChainingHashMap, EraseExistingKey) {
  ChainingHashMap map;
  map.insert(1, 100);
  EXPECT_TRUE(map.erase(1));
  EXPECT_EQ(map.size(), 0);
  EXPECT_FALSE(map.find(1).has_value());
}

TEST(ChainingHashMap, EraseMissingKey) {
  ChainingHashMap map;
  map.insert(1, 100);
  EXPECT_FALSE(map.erase(99));
  EXPECT_EQ(map.size(), 1);
}

TEST(ChainingHashMap, EraseHeadOfChain) {
  // Force two keys into the same bucket by inserting enough to collide.
  // We can't control hashing exactly, but we test that erasing one key
  // in a multi-entry map doesn't break others.
  ChainingHashMap map;
  map.insert(1, 10);
  map.insert(2, 20);
  map.insert(3, 30);
  EXPECT_TRUE(map.erase(1));
  EXPECT_FALSE(map.find(1).has_value());
  EXPECT_EQ(*map.find(2), 20);
  EXPECT_EQ(*map.find(3), 30);
  EXPECT_EQ(map.size(), 2);
}

TEST(ChainingHashMap, EraseMiddleOfChain) {
  ChainingHashMap map;
  map.insert(1, 10);
  map.insert(2, 20);
  map.insert(3, 30);
  EXPECT_TRUE(map.erase(2));
  EXPECT_EQ(*map.find(1), 10);
  EXPECT_FALSE(map.find(2).has_value());
  EXPECT_EQ(*map.find(3), 30);
}

TEST(ChainingHashMap, EraseAndReinsert) {
  ChainingHashMap map;
  map.insert(5, 50);
  EXPECT_TRUE(map.erase(5));
  EXPECT_FALSE(map.find(5).has_value());
  map.insert(5, 500);
  EXPECT_EQ(*map.find(5), 500);
  EXPECT_EQ(map.size(), 1);
}

// --- Collisions ---

TEST(ChainingHashMap, ManyKeysWithSmallTable) {
  // With an initial bucket count of 8, inserting 20+ keys forces
  // multiple entries per chain and at least one rehash.
  ChainingHashMap map;
  for (int i = 0; i < 25; ++i) {
    map.insert(i, i * 10);
  }
  EXPECT_EQ(map.size(), 25);
  for (int i = 0; i < 25; ++i) {
    auto result = map.find(i);
    ASSERT_TRUE(result.has_value()) << "missing key " << i;
    EXPECT_EQ(*result, i * 10);
  }
}

// --- Rehash ---

TEST(ChainingHashMap, RehashPreservesEntries) {
  ChainingHashMap map;
  // Insert enough to trigger at least one rehash (8 * 0.75 = 6)
  for (int i = 0; i < 20; ++i) {
    map.insert(i, i * 100);
  }
  EXPECT_EQ(map.size(), 20);
  for (int i = 0; i < 20; ++i) {
    auto result = map.find(i);
    ASSERT_TRUE(result.has_value()) << "missing key " << i << " after rehash";
    EXPECT_EQ(*result, i * 100);
  }
}

TEST(ChainingHashMap, UpdateAfterRehash) {
  ChainingHashMap map;
  for (int i = 0; i < 20; ++i) {
    map.insert(i, i);
  }
  // Update a key that was rehashed into a new bucket
  map.insert(5, 999);
  EXPECT_EQ(*map.find(5), 999);
  EXPECT_EQ(map.size(), 20);
}

// --- Negative keys ---

TEST(ChainingHashMap, NegativeKeys) {
  ChainingHashMap map;
  map.insert(-1, 100);
  map.insert(-100, 200);
  EXPECT_EQ(*map.find(-1), 100);
  EXPECT_EQ(*map.find(-100), 200);
  EXPECT_EQ(map.size(), 2);
}

// --- Zero key ---

TEST(ChainingHashMap, ZeroKey) {
  ChainingHashMap map;
  map.insert(0, 42);
  EXPECT_EQ(*map.find(0), 42);
  EXPECT_TRUE(map.erase(0));
  EXPECT_FALSE(map.find(0).has_value());
}

// --- Erase all entries ---

TEST(ChainingHashMap, EraseAllEntries) {
  ChainingHashMap map;
  for (int i = 0; i < 10; ++i) {
    map.insert(i, i);
  }
  for (int i = 0; i < 10; ++i) {
    EXPECT_TRUE(map.erase(i));
  }
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
  for (int i = 0; i < 10; ++i) {
    EXPECT_FALSE(map.find(i).has_value());
  }
}

// --- Large scale ---

TEST(ChainingHashMap, LargeScale) {
  ChainingHashMap map;
  constexpr int n = 10000;
  for (int i = 0; i < n; ++i) {
    map.insert(i, i * 3);
  }
  EXPECT_EQ(map.size(), n);
  for (int i = 0; i < n; ++i) {
    EXPECT_EQ(*map.find(i), i * 3);
  }
  // Erase even keys
  for (int i = 0; i < n; i += 2) {
    EXPECT_TRUE(map.erase(i));
  }
  EXPECT_EQ(map.size(), n / 2);
  for (int i = 0; i < n; ++i) {
    if (i % 2 == 0) {
      EXPECT_FALSE(map.find(i).has_value());
    } else {
      EXPECT_EQ(*map.find(i), i * 3);
    }
  }
}

// --- Duplicate erase ---

TEST(ChainingHashMap, DoubleErase) {
  ChainingHashMap map;
  map.insert(7, 77);
  EXPECT_TRUE(map.erase(7));
  EXPECT_FALSE(map.erase(7));
  EXPECT_EQ(map.size(), 0);
}
