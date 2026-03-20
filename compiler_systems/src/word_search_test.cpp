#include "word_search.hpp"

#include "data_oriented_index_arena_trie.hpp"
#include "deque_arena_trie.hpp"
#include "index_arena_trie.hpp"
#include "ptr_trie.hpp"

#include <algorithm>
#include <gtest/gtest.h>

// Helper: sort results for order-independent comparison
static std::vector<std::string> sorted(std::vector<std::string> v) {
  std::sort(v.begin(), v.end());
  return v;
}

using IndexArenaSentinel = IndexArenaTrie<true>;
using IndexArenaZeroNull = IndexArenaTrie<false>;

template <typename T> class WordSearchTest : public ::testing::Test {};

using TrieTypes =
    ::testing::Types<IndexArenaSentinel, IndexArenaZeroNull,
                     DataOrientedIndexArenaTrie, DequeArenaTrie, PtrTrie>;
TYPED_TEST_SUITE(WordSearchTest, TrieTypes);

TYPED_TEST(WordSearchTest, LeetCodeExample) {
  // The classic example from the problem description
  std::vector<std::vector<char>> board = {{'o', 'a', 'a', 'n'},
                                          {'e', 't', 'a', 'e'},
                                          {'i', 'h', 'k', 'r'},
                                          {'i', 'f', 'l', 'v'}};
  std::vector<std::string> words = {"oath", "pea", "eat", "rain"};
  EXPECT_EQ(sorted(find_words<TypeParam>(board, words)),
            sorted({"eat", "oath"}));
}

TYPED_TEST(WordSearchTest, SingleCell) {
  // Board is 1x1 — word must be a single character
  std::vector<std::vector<char>> board = {{'a'}};
  std::vector<std::string> words = {"a", "ab"};
  EXPECT_EQ(find_words<TypeParam>(board, words), std::vector<std::string>{"a"});
}

TYPED_TEST(WordSearchTest, NoMatches) {
  std::vector<std::vector<char>> board = {{'a', 'b'}, {'c', 'd'}};
  std::vector<std::string> words = {"xyz", "qqq"};
  EXPECT_EQ(find_words<TypeParam>(board, words), std::vector<std::string>{});
}

TYPED_TEST(WordSearchTest, AllWordsFound) {
  // Every target word is present on the board
  // Board:  a b
  //         c d
  // Adjacencies: a-b (right), a-c (down), b-d (down), c-d (right)
  // Note: a-d and b-c are diagonal — NOT adjacent
  std::vector<std::vector<char>> board = {{'a', 'b'}, {'c', 'd'}};
  std::vector<std::string> words = {"ab", "abd", "abdc", "cabd"};
  EXPECT_EQ(sorted(find_words<TypeParam>(board, words)),
            sorted({"ab", "abd", "abdc", "cabd"}));
}

TYPED_TEST(WordSearchTest, DuplicateWordsInInput) {
  // Same word appears twice in the input list — should only appear once in
  // output
  std::vector<std::vector<char>> board = {{'a', 'b'}, {'c', 'd'}};
  std::vector<std::string> words = {"ab", "ab"};
  EXPECT_EQ(find_words<TypeParam>(board, words),
            std::vector<std::string>{"ab"});
}

TYPED_TEST(WordSearchTest, WordRequiresBacktracking) {
  // "aab": a(0,0)->a(0,1)->b(1,1), right then down. Valid.
  // "aba": a->b->a requires returning to a cell not yet used, but
  //   every a->b pair leaves no unused 'a' adjacent to that b. Invalid.
  std::vector<std::vector<char>> board = {{'a', 'a'}, {'b', 'b'}};
  std::vector<std::string> words = {"aab", "aba"};
  EXPECT_EQ(find_words<TypeParam>(board, words),
            std::vector<std::string>{"aab"});
}

TYPED_TEST(WordSearchTest, SharedPrefix) {
  // "the" and "them" share a prefix — the trie should find both
  // in a single traversal from 't'
  std::vector<std::vector<char>> board = {{'t', 'h', 'e', 'm'}};
  std::vector<std::string> words = {"the", "them", "then"};
  EXPECT_EQ(sorted(find_words<TypeParam>(board, words)),
            sorted({"the", "them"}));
}

TYPED_TEST(WordSearchTest, LargerBoard) {
  std::vector<std::vector<char>> board = {{'o', 'a', 'b', 'n'},
                                          {'o', 't', 'a', 'e'},
                                          {'a', 'h', 'k', 'r'},
                                          {'a', 'f', 'l', 'v'}};
  std::vector<std::string> words = {"oa", "oat", "oath", "oaths", "eat", "eta"};
  // oa: o(0,0)->a(0,1), right. Valid.
  // oat: o(0,0)->a(0,1)->t(1,1), right, down. Valid.
  // oath: o(0,0)->a(0,1)->t(1,1)->h(2,1), right, down, down. Valid.
  // oaths: no 's' on the board. Invalid.
  // eat: e(1,3)->a(1,2)->t(1,1), left, left. Valid.
  // eta: e(1,3)->t(1,1)? Not adjacent (2 cols apart). Invalid.
  EXPECT_EQ(sorted(find_words<TypeParam>(board, words)),
            sorted({"oa", "oat", "oath", "eat"}));
}

TYPED_TEST(WordSearchTest, SingleRowSnake) {
  // Entire board is one row — adjacency is only left/right
  std::vector<std::vector<char>> board = {{'c', 'a', 't', 's'}};
  std::vector<std::string> words = {"cat", "cats", "tac", "sat"};
  EXPECT_EQ(sorted(find_words<TypeParam>(board, words)),
            sorted({"cat", "cats", "tac"}));
}

TYPED_TEST(WordSearchTest, MaxSizeBoard) {
  // 12x12 board — maximum dimensions per constraints
  // clang-format off
  std::vector<std::vector<char>> board = {
      {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l'},
      {'m', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x'},
      {'y', 'z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'},
      {'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v'},
      {'w', 'x', 'y', 'z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'},
      {'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't'},
      {'u', 'v', 'w', 'x', 'y', 'z', 'a', 'b', 'c', 'd', 'e', 'f'},
      {'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r'},
      {'s', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'a', 'b', 'c', 'd'},
      {'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p'},
      {'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'a', 'b'},
      {'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n'},
  };
  // clang-format on
  // "abcdef": a(0,0)->b(0,1)->c(0,2)->d(0,3)->e(0,4)->f(0,5). All right. Valid.
  // "mnoy":   m(1,0)->n(1,1)->o(1,2)->y(2,0)? o(1,2) neighbors: b(0,2), n(1,1)
  // used,
  //           p(1,3), a(2,2). No 'y' adjacent. Invalid.
  // "mno":    m(1,0)->n(1,1)->o(1,2). right, right. Valid.
  // "zan":    z(2,1)->a(2,2)->n(3,3)? a(2,2) neighbors: o(1,2), z(2,1) used,
  //           b(2,3), m(3,2). No 'n' adjacent. Invalid.
  //           But also z(4,3)->a(4,4)->n(5,5)? a(4,4) neighbors: z(4,3) used,
  //           b(4,5), m(5,4), o(3,4). No 'n'. Invalid.
  // "zyxwvu": z(2,1) has no 'y' neighbor. z(4,3)->y(4,2)->x(4,1)->w(4,0)->
  //           v(3,0)? (3,0) is 'k'. No. w(4,0) neighbors: x(4,1) used, k(3,0),
  //           i(5,0). No 'v'. Invalid.
  //           z(5,5)->y(6,5)? (6,5)='z'. No.
  //           z(8,7)->y(8,6)->x(8,5)->w(8,4)->v(8,3)->u(8,2). All left. Valid!
  // "nmlkji": n(11,11)->m(11,10)->l(11,9)->k(11,8)->j(11,7)->i(11,6). All left.
  // Valid.
  std::vector<std::string> words = {"abcdef", "mnoy",   "mno",
                                    "zan",    "zyxwvu", "nmlkji"};
  EXPECT_EQ(sorted(find_words<TypeParam>(board, words)),
            sorted({"abcdef", "mno", "zyxwvu", "nmlkji"}));
}

TYPED_TEST(WordSearchTest, NoCellReuse) {
  // "aba" requires going a->b->a, but the board only has one 'a'
  // adjacent to 'b' — can't reuse the same 'a' cell
  std::vector<std::vector<char>> board = {{'a', 'b', 'c'}};
  std::vector<std::string> words = {"aba"};
  EXPECT_EQ(find_words<TypeParam>(board, words), std::vector<std::string>{});
}
