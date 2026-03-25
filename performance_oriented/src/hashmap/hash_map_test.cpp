#include "chaining_hash_map.hpp"
#include "chaining_pool_hash_map.hpp"
#include "linear_probing_hash_map.hpp"
#include "linear_probing_hash_map_merged.hpp"
#include "robin_hood_hash_map.hpp"
#include "robin_hood_stored_dist_hash_map.hpp"
#include "std_unordered_map_adapter.hpp"

#include <gtest/gtest.h>

template <typename T> class HashMapTest : public ::testing::Test {};

// clang-format off
using HashMapTypes = ::testing::Types<
    ChainingHashMap<0.5>,            ChainingHashMap<0.75>,            ChainingHashMap<0.9>,
    ChainingPoolHashMap<0.5>,          ChainingPoolHashMap<0.75>,          ChainingPoolHashMap<0.9>,
    LinearProbingHashMap<0.5>,       LinearProbingHashMap<0.75>,       LinearProbingHashMap<0.9>,
    LinearProbingHashMap<0.5, true>, LinearProbingHashMap<0.75, true>, LinearProbingHashMap<0.9, true>,
    LinearProbingHashMap<0.5, false, true>, LinearProbingHashMap<0.75, false, true>, LinearProbingHashMap<0.9, false, true>,
    LinearProbingMergedStructHashMap<0.5>, LinearProbingMergedStructHashMap<0.75>, LinearProbingMergedStructHashMap<0.9>,
    RobinHoodHashMap<0.5>,           RobinHoodHashMap<0.75>,           RobinHoodHashMap<0.9>,
    RobinHoodHashMap<0.5, true>,     RobinHoodHashMap<0.75, true>,     RobinHoodHashMap<0.9, true>,
    RobinHoodStoredDistHashMap<0.5>,         RobinHoodStoredDistHashMap<0.75>,         RobinHoodStoredDistHashMap<0.9>,
    StdUnorderedMapAdapter<0.5>,     StdUnorderedMapAdapter<0.75>,     StdUnorderedMapAdapter<0.9>
>;
// clang-format on
TYPED_TEST_SUITE(HashMapTest, HashMapTypes);

// --- Basic operations ---

TYPED_TEST(HashMapTest, EmptyOnConstruction) {
  TypeParam map;
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
}

TYPED_TEST(HashMapTest, InsertAndFind) {
  TypeParam map;
  map.insert(1, 100);
  auto result = map.find(1);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 100);
}

TYPED_TEST(HashMapTest, FindMissing) {
  TypeParam map;
  EXPECT_FALSE(map.find(42).has_value());
}

TYPED_TEST(HashMapTest, InsertUpdatesExistingKey) {
  TypeParam map;
  map.insert(1, 100);
  map.insert(1, 200);
  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(*map.find(1), 200);
}

TYPED_TEST(HashMapTest, SizeTracksInserts) {
  TypeParam map;
  map.insert(1, 10);
  map.insert(2, 20);
  map.insert(3, 30);
  EXPECT_EQ(map.size(), 3);
  EXPECT_FALSE(map.empty());
}

// --- Erase ---

TYPED_TEST(HashMapTest, EraseExistingKey) {
  TypeParam map;
  map.insert(1, 100);
  EXPECT_TRUE(map.erase(1));
  EXPECT_EQ(map.size(), 0);
  EXPECT_FALSE(map.find(1).has_value());
}

TYPED_TEST(HashMapTest, EraseMissingKey) {
  TypeParam map;
  map.insert(1, 100);
  EXPECT_FALSE(map.erase(99));
  EXPECT_EQ(map.size(), 1);
}

TYPED_TEST(HashMapTest, EraseOneOfMany) {
  TypeParam map;
  map.insert(1, 10);
  map.insert(2, 20);
  map.insert(3, 30);
  EXPECT_TRUE(map.erase(1));
  EXPECT_FALSE(map.find(1).has_value());
  EXPECT_EQ(*map.find(2), 20);
  EXPECT_EQ(*map.find(3), 30);
  EXPECT_EQ(map.size(), 2);
}

TYPED_TEST(HashMapTest, EraseMiddleKey) {
  TypeParam map;
  map.insert(1, 10);
  map.insert(2, 20);
  map.insert(3, 30);
  EXPECT_TRUE(map.erase(2));
  EXPECT_EQ(*map.find(1), 10);
  EXPECT_FALSE(map.find(2).has_value());
  EXPECT_EQ(*map.find(3), 30);
}

TYPED_TEST(HashMapTest, EraseAndReinsert) {
  TypeParam map;
  map.insert(5, 50);
  EXPECT_TRUE(map.erase(5));
  EXPECT_FALSE(map.find(5).has_value());
  map.insert(5, 500);
  EXPECT_EQ(*map.find(5), 500);
  EXPECT_EQ(map.size(), 1);
}

// --- Tombstone / probe chain correctness ---
// These exercise the same insert/erase/find paths for chaining,
// but are critical for open addressing where tombstones matter.

TYPED_TEST(HashMapTest, EraseDoesNotBreakLaterFinds) {
  TypeParam map;
  map.insert(1, 10);
  map.insert(2, 20);
  map.insert(3, 30);
  EXPECT_TRUE(map.erase(2));
  EXPECT_FALSE(map.find(2).has_value());
  EXPECT_EQ(*map.find(1), 10);
  EXPECT_EQ(*map.find(3), 30);
  EXPECT_EQ(map.size(), 2);
}

TYPED_TEST(HashMapTest, InsertAfterErase) {
  TypeParam map;
  map.insert(1, 10);
  map.insert(2, 20);
  EXPECT_TRUE(map.erase(1));
  map.insert(3, 30);
  EXPECT_FALSE(map.find(1).has_value());
  EXPECT_EQ(*map.find(2), 20);
  EXPECT_EQ(*map.find(3), 30);
  EXPECT_EQ(map.size(), 2);
}

TYPED_TEST(HashMapTest, HeavyEraseAndReinsertChurn) {
  TypeParam map;
  for (int round = 0; round < 5; ++round) {
    for (int i = 0; i < 50; ++i) {
      map.insert(i, round * 100 + i);
    }
    for (int i = 0; i < 40; ++i) {
      map.erase(i);
    }
  }
  // After 5 rounds: keys 0-39 erased, keys 40-49 have round-4 values
  EXPECT_EQ(map.size(), 10);
  for (int i = 40; i < 50; ++i) {
    EXPECT_EQ(*map.find(i), 400 + i);
  }
}

// --- Collisions / rehash ---

TYPED_TEST(HashMapTest, ManyKeys) {
  TypeParam map;
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

TYPED_TEST(HashMapTest, RehashPreservesEntries) {
  TypeParam map;
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

TYPED_TEST(HashMapTest, UpdateAfterRehash) {
  TypeParam map;
  for (int i = 0; i < 20; ++i) {
    map.insert(i, i);
  }
  map.insert(5, 999);
  EXPECT_EQ(*map.find(5), 999);
  EXPECT_EQ(map.size(), 20);
}

TYPED_TEST(HashMapTest, RehashAfterManyErases) {
  TypeParam map;
  for (int i = 0; i < 10; ++i) {
    map.insert(i, i);
  }
  for (int i = 0; i < 8; ++i) {
    map.erase(i);
  }
  // 2 entries remaining. Insert enough to trigger rehash.
  for (int i = 100; i < 115; ++i) {
    map.insert(i, i);
  }
  EXPECT_EQ(map.size(), 17);
  EXPECT_EQ(*map.find(8), 8);
  EXPECT_EQ(*map.find(9), 9);
  for (int i = 100; i < 115; ++i) {
    EXPECT_EQ(*map.find(i), i);
  }
}

// --- Edge cases ---

TYPED_TEST(HashMapTest, NegativeKeys) {
  TypeParam map;
  map.insert(-1, 100);
  map.insert(-100, 200);
  EXPECT_EQ(*map.find(-1), 100);
  EXPECT_EQ(*map.find(-100), 200);
  EXPECT_EQ(map.size(), 2);
}

TYPED_TEST(HashMapTest, ZeroKey) {
  TypeParam map;
  map.insert(0, 42);
  EXPECT_EQ(*map.find(0), 42);
  EXPECT_TRUE(map.erase(0));
  EXPECT_FALSE(map.find(0).has_value());
}

TYPED_TEST(HashMapTest, EraseAllEntries) {
  TypeParam map;
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

TYPED_TEST(HashMapTest, DoubleErase) {
  TypeParam map;
  map.insert(7, 77);
  EXPECT_TRUE(map.erase(7));
  EXPECT_FALSE(map.erase(7));
  EXPECT_EQ(map.size(), 0);
}

// --- Large scale ---

TYPED_TEST(HashMapTest, LargeScale) {
  TypeParam map;
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
