#pragma once

#include <string>
#include <vector>

// Word Search II (LeetCode 212)
//
// Given an m x n board of characters and a list of target words, return all
// words that can be found on the board.
//
// A word is "found" if it can be constructed from a path of horizontally or
// vertically adjacent cells (up, down, left, right), where each cell is used
// at most once per word.
//
// The same cell may be reused across different words, just not within a single
// word's path.
//
// Constraints:
//   - 1 <= m, n <= 12
//   - 1 <= words.length <= 3 * 10^4
//   - 1 <= words[i].length <= 10
//   - board[i][j] is a lowercase English letter
//   - words[i] is a lowercase English letter
//   - All words[i] are unique (but duplicates should be handled gracefully)
//
// Example:
//   board = {{'o','a','a','n'},
//            {'e','t','a','e'},
//            {'i','h','k','r'},
//            {'i','f','l','v'}}
//   words = {"oath", "pea", "eat", "rain"}
//   result = {"eat", "oath"}
//
//   "oath" is found: (0,0)->(1,0)->(2,0)->... wait, no:
//     o(0,0) -> a(0,1)? no, oath...
//     o(0,0) is 'o', need 'a' adjacent: (0,1)='a', then 't': (1,1)='t',
//     then 'h': (2,1)='h'. Path: (0,0)->(0,1)->(1,1)->(2,1). Valid.
//   "eat" is found: e(1,0)->a(0,0)? no, 'a' at (0,1) is adjacent to e?
//     e(1,3)->'a'(0,3)? no that's 'n'. e(1,3)->a(1,2)->t(1,1). Valid.
//   "pea" — no 'p' on the board.
//   "rain" — r(2,3)->a(1,2)->i(?)-> no 'i' adjacent to 'a' at (1,2).

inline std::vector<std::string>
findWords(std::vector<std::vector<char>> &board,
          const std::vector<std::string> &words) {
  return {};
}
