#include "data_oriented_index_arena_trie.hpp"
#include "deque_arena_trie.hpp"
#include "index_arena_trie.hpp"
#include "ptr_trie.hpp"

#include <algorithm>
#include <gtest/gtest.h>

using IndexArenaSentinel = IndexArenaTrie<true>;
using IndexArenaZeroNull = IndexArenaTrie<false>;

template <typename T> class TrieTest : public ::testing::Test {};

using TrieTypes =
    ::testing::Types<IndexArenaSentinel, IndexArenaZeroNull,
                     DataOrientedIndexArenaTrie, DequeArenaTrie, PtrTrie>;
TYPED_TEST_SUITE(TrieTest, TrieTypes);

TYPED_TEST(TrieTest, SearchEmptyTrie) {
  TypeParam trie;
  EXPECT_FALSE(trie.search("hello"));
  EXPECT_FALSE(trie.startsWith("h"));
}

TYPED_TEST(TrieTest, InsertAndSearch) {
  TypeParam trie;
  trie.insert("apple");
  EXPECT_TRUE(trie.search("apple"));
  EXPECT_FALSE(trie.search("orange"));
}

TYPED_TEST(TrieTest, PrefixIsNotAWord) {
  // "app" is a prefix of "apple" but was never inserted
  TypeParam trie;
  trie.insert("apple");
  EXPECT_FALSE(trie.search("app"));
  EXPECT_TRUE(trie.startsWith("app"));
}

TYPED_TEST(TrieTest, WordThatIsAPrefixOfAnother) {
  // Insert both "app" and "apple" — both should be findable
  TypeParam trie;
  trie.insert("app");
  trie.insert("apple");
  EXPECT_TRUE(trie.search("app"));
  EXPECT_TRUE(trie.search("apple"));
}

TYPED_TEST(TrieTest, StartsWithExisting) {
  TypeParam trie;
  trie.insert("apple");
  trie.insert("application");
  EXPECT_TRUE(trie.startsWith("app"));
  EXPECT_TRUE(trie.startsWith("apple"));
  EXPECT_TRUE(trie.startsWith("applic"));
}

TYPED_TEST(TrieTest, StartsWithNonExisting) {
  TypeParam trie;
  trie.insert("apple");
  EXPECT_FALSE(trie.startsWith("b"));
  EXPECT_FALSE(trie.startsWith("apc"));
}

TYPED_TEST(TrieTest, MultipleWordsDisjointPrefixes) {
  TypeParam trie;
  trie.insert("apple");
  trie.insert("bed");
  trie.insert("cat");
  EXPECT_TRUE(trie.search("apple"));
  EXPECT_TRUE(trie.search("bed"));
  EXPECT_TRUE(trie.search("cat"));
  EXPECT_FALSE(trie.search("bat"));
}

TYPED_TEST(TrieTest, EmptyString) {
  TypeParam trie;
  trie.insert("");
  EXPECT_TRUE(trie.search(""));
  EXPECT_TRUE(trie.startsWith(""));
}

TYPED_TEST(TrieTest, EmptyPrefixOnEmptyTrie) {
  // Every trie starts with the empty prefix
  TypeParam trie;
  EXPECT_TRUE(trie.startsWith(""));
}

TYPED_TEST(TrieTest, SearchFullWordAsPrefix) {
  // startsWith should return true when the prefix is an exact word
  TypeParam trie;
  trie.insert("hello");
  EXPECT_TRUE(trie.startsWith("hello"));
}

// --- remove tests ---

TYPED_TEST(TrieTest, RemoveExistingWord) {
  TypeParam trie;
  trie.insert("apple");
  EXPECT_TRUE(trie.remove("apple"));
  EXPECT_FALSE(trie.search("apple"));
}

TYPED_TEST(TrieTest, RemoveNonExistingWord) {
  TypeParam trie;
  trie.insert("apple");
  EXPECT_FALSE(trie.remove("orange"));
  EXPECT_TRUE(trie.search("apple"));
}

TYPED_TEST(TrieTest, RemoveWordPreservesLongerWord) {
  // Removing "app" should not affect "apple"
  TypeParam trie;
  trie.insert("app");
  trie.insert("apple");
  EXPECT_TRUE(trie.remove("app"));
  EXPECT_FALSE(trie.search("app"));
  EXPECT_TRUE(trie.search("apple"));
}

TYPED_TEST(TrieTest, RemoveLongerWordPreservesShorterWord) {
  // Removing "apple" should not affect "app"
  TypeParam trie;
  trie.insert("app");
  trie.insert("apple");
  EXPECT_TRUE(trie.remove("apple"));
  EXPECT_FALSE(trie.search("apple"));
  EXPECT_TRUE(trie.search("app"));
}

TYPED_TEST(TrieTest, RemovePrefixThatIsNotAWord) {
  // "app" was never inserted, so removing it should return false
  TypeParam trie;
  trie.insert("apple");
  EXPECT_FALSE(trie.remove("app"));
  EXPECT_TRUE(trie.search("apple"));
}

// --- getWordsWithPrefix tests ---

TYPED_TEST(TrieTest, GetWordsWithPrefix) {
  TypeParam trie;
  trie.insert("apple");
  trie.insert("app");
  trie.insert("application");
  trie.insert("bed");

  auto words = trie.getWordsWithPrefix("app");
  std::sort(words.begin(), words.end());

  std::vector<std::string> expected = {"app", "apple", "application"};
  EXPECT_EQ(words, expected);
}

TYPED_TEST(TrieTest, GetWordsWithPrefixNoMatches) {
  TypeParam trie;
  trie.insert("apple");
  auto words = trie.getWordsWithPrefix("z");
  EXPECT_TRUE(words.empty());
}

TYPED_TEST(TrieTest, GetWordsWithEmptyPrefix) {
  // Empty prefix should return all words
  TypeParam trie;
  trie.insert("apple");
  trie.insert("bed");

  auto words = trie.getWordsWithPrefix("");
  std::sort(words.begin(), words.end());

  std::vector<std::string> expected = {"apple", "bed"};
  EXPECT_EQ(words, expected);
}

TYPED_TEST(TrieTest, GetWordsWithExactWordAsPrefix) {
  // The prefix itself is a stored word
  TypeParam trie;
  trie.insert("apple");
  auto words = trie.getWordsWithPrefix("apple");
  EXPECT_EQ(words, std::vector<std::string>{"apple"});
}
