#include "word_search.hpp"

#include <algorithm>
#include <gtest/gtest.h>

// Helper: sort results for order-independent comparison
static std::vector<std::string> sorted(std::vector<std::string> v) {
  std::sort(v.begin(), v.end());
  return v;
}

TEST(WordSearchTest, LeetCodeExample) {
  // The classic example from the problem description
  std::vector<std::vector<char>> board = {{'o', 'a', 'a', 'n'},
                                          {'e', 't', 'a', 'e'},
                                          {'i', 'h', 'k', 'r'},
                                          {'i', 'f', 'l', 'v'}};
  std::vector<std::string> words = {"oath", "pea", "eat", "rain"};
  EXPECT_EQ(sorted(findWords(board, words)), sorted({"eat", "oath"}));
}

TEST(WordSearchTest, SingleCell) {
  // Board is 1x1 — word must be a single character
  std::vector<std::vector<char>> board = {{'a'}};
  std::vector<std::string> words = {"a", "ab"};
  EXPECT_EQ(findWords(board, words), std::vector<std::string>{"a"});
}

TEST(WordSearchTest, NoMatches) {
  std::vector<std::vector<char>> board = {{'a', 'b'}, {'c', 'd'}};
  std::vector<std::string> words = {"xyz", "qqq"};
  EXPECT_EQ(findWords(board, words), std::vector<std::string>{});
}

TEST(WordSearchTest, AllWordsFound) {
  // Every target word is present on the board
  std::vector<std::vector<char>> board = {{'a', 'b'}, {'c', 'd'}};
  std::vector<std::string> words = {"ab", "abc", "abcd", "abdc"};
  EXPECT_EQ(sorted(findWords(board, words)),
            sorted({"ab", "abc", "abcd", "abdc"}));
}

TEST(WordSearchTest, DuplicateWordsInInput) {
  // Same word appears twice in the input list — should only appear once in
  // output
  std::vector<std::vector<char>> board = {{'a', 'b'}, {'c', 'd'}};
  std::vector<std::string> words = {"ab", "ab"};
  EXPECT_EQ(findWords(board, words), std::vector<std::string>{"ab"});
}

TEST(WordSearchTest, WordRequiresBacktracking) {
  // "aab" can only be found by going right then down, not right-right
  // Tests that cells aren't reused in a single path
  std::vector<std::vector<char>> board = {{'a', 'a'}, {'b', 'b'}};
  std::vector<std::string> words = {"aab", "aba"};
  EXPECT_EQ(sorted(findWords(board, words)), sorted({"aab", "aba"}));
}

TEST(WordSearchTest, SharedPrefix) {
  // "the" and "them" share a prefix — the trie should find both
  // in a single traversal from 't'
  std::vector<std::vector<char>> board = {{'t', 'h', 'e', 'm'}};
  std::vector<std::string> words = {"the", "them", "then"};
  EXPECT_EQ(sorted(findWords(board, words)), sorted({"the", "them"}));
}

TEST(WordSearchTest, LargerBoard) {
  std::vector<std::vector<char>> board = {{'o', 'a', 'b', 'n'},
                                          {'o', 't', 'a', 'e'},
                                          {'a', 'h', 'k', 'r'},
                                          {'a', 'f', 'l', 'v'}};
  std::vector<std::string> words = {"oa", "oat", "oath", "oaths", "eat", "eta"};
  EXPECT_EQ(sorted(findWords(board, words)),
            sorted({"oa", "oat", "oath", "eat", "eta"}));
}

TEST(WordSearchTest, SingleRowSnake) {
  // Entire board is one row — adjacency is only left/right
  std::vector<std::vector<char>> board = {{'c', 'a', 't', 's'}};
  std::vector<std::string> words = {"cat", "cats", "tac", "sat"};
  EXPECT_EQ(sorted(findWords(board, words)), sorted({"cat", "cats", "tac"}));
}

TEST(WordSearchTest, NoCellReuse) {
  // "aba" requires going a->b->a, but the board only has one 'a'
  // adjacent to 'b' — can't reuse the same 'a' cell
  std::vector<std::vector<char>> board = {{'a', 'b', 'c'}};
  std::vector<std::string> words = {"aba"};
  EXPECT_EQ(findWords(board, words), std::vector<std::string>{});
}
