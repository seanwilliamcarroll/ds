#include "trie.hpp"

#include <gtest/gtest.h>

TEST(TrieTest, SearchEmptyTrie) {
  Trie trie;
  EXPECT_FALSE(trie.search("hello"));
  EXPECT_FALSE(trie.startsWith("h"));
}

TEST(TrieTest, InsertAndSearch) {
  Trie trie;
  trie.insert("apple");
  EXPECT_TRUE(trie.search("apple"));
  EXPECT_FALSE(trie.search("orange"));
}

TEST(TrieTest, PrefixIsNotAWord) {
  // "app" is a prefix of "apple" but was never inserted
  Trie trie;
  trie.insert("apple");
  EXPECT_FALSE(trie.search("app"));
  EXPECT_TRUE(trie.startsWith("app"));
}

TEST(TrieTest, WordThatIsAPrefixOfAnother) {
  // Insert both "app" and "apple" — both should be findable
  Trie trie;
  trie.insert("app");
  trie.insert("apple");
  EXPECT_TRUE(trie.search("app"));
  EXPECT_TRUE(trie.search("apple"));
}

TEST(TrieTest, StartsWithExisting) {
  Trie trie;
  trie.insert("apple");
  trie.insert("application");
  EXPECT_TRUE(trie.startsWith("app"));
  EXPECT_TRUE(trie.startsWith("apple"));
  EXPECT_TRUE(trie.startsWith("applic"));
}

TEST(TrieTest, StartsWithNonExisting) {
  Trie trie;
  trie.insert("apple");
  EXPECT_FALSE(trie.startsWith("b"));
  EXPECT_FALSE(trie.startsWith("ap0"));
}

TEST(TrieTest, MultipleWordsDisjointPrefixes) {
  Trie trie;
  trie.insert("apple");
  trie.insert("bed");
  trie.insert("cat");
  EXPECT_TRUE(trie.search("apple"));
  EXPECT_TRUE(trie.search("bed"));
  EXPECT_TRUE(trie.search("cat"));
  EXPECT_FALSE(trie.search("bat"));
}

TEST(TrieTest, EmptyString) {
  Trie trie;
  trie.insert("");
  EXPECT_TRUE(trie.search(""));
  EXPECT_TRUE(trie.startsWith(""));
}

TEST(TrieTest, SearchFullWordAsPrefix) {
  // startsWith should return true when the prefix is an exact word
  Trie trie;
  trie.insert("hello");
  EXPECT_TRUE(trie.startsWith("hello"));
}
