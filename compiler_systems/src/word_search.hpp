#pragma once

#include "trie.hpp"
#include <cstddef>
#include <string>
#include <unordered_set>
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
  // Load all the words into a Trie for fast lookup

  // For each position on the board, check if the that character is a prefix
  // if prefix, start seach in 4 directions, keeping track of directions
  // previously seen
  // for each letter, add to existing prefix so far, do search
  // if search succeeds, stop and add word to found words
  // else, check if the prefix exists in the trie
  // if so continue on with it

  std::unordered_set<std::string> found_words;

  ArenaTrie trie;
  for (const auto &word : words) {
    trie.insert(word);
  }

  using Position = std::pair<size_t, size_t>;

  const std::vector<std::vector<bool>> initial_seen_positions(
      board.size(), std::vector<bool>(board.front().size(), false));

  // Want to do DFS across the board
  struct IntermediateResult {
    Position last_position;
    std::string prefix;
    std::vector<std::vector<bool>> seen_positions;

    IntermediateResult(size_t row_index, size_t col_index, std::string prefix,
                       std::vector<std::vector<bool>> seen_positions)
        : last_position{row_index, col_index}, prefix(std::move(prefix)),
          seen_positions(std::move(seen_positions)) {
      saw(row_index, col_index);
    }

    void saw(size_t row_index, size_t col_index) {
      seen_positions[row_index][col_index] = true;
    }

    bool seen(size_t row_index, size_t col_index) const {
      return seen_positions[row_index][col_index];
    }

    std::vector<Position> get_possible_next_positions() const {
      std::vector<Position> output;

      auto [row_index, col_index] = last_position;

      if (row_index > 0 && !seen(row_index - 1, col_index)) {
        output.push_back({row_index - 1, col_index});
      }

      if (col_index > 0 && !seen(row_index, col_index - 1)) {
        output.push_back({row_index, col_index - 1});
      }

      if (row_index < seen_positions.size() - 1 &&
          !seen(row_index + 1, col_index)) {
        output.push_back({row_index + 1, col_index});
      }

      if (col_index < seen_positions.back().size() - 1 &&
          !seen(row_index, col_index + 1)) {
        output.push_back({row_index, col_index + 1});
      }

      return output;
    }
  };

  std::vector<IntermediateResult> attempts;
  {
    size_t row_index = 0;
    for (const auto &row : board) {
      size_t col_index = 0;
      for (const auto character : row) {
        std::string first_letter{character};
        if (trie.search(first_letter)) {
          found_words.insert(first_letter);
        }
        if (trie.startsWith(first_letter)) {
          attempts.push_back(IntermediateResult(row_index, col_index,
                                                std::move(first_letter),
                                                initial_seen_positions));
        }
        ++col_index;
      }
      ++row_index;
    }
  }
  // DFS queue initialized

  while (!attempts.empty()) {
    auto next_attempt = std::move(attempts.back());
    attempts.pop_back();
    for (const auto &[row_index, col_index] :
         next_attempt.get_possible_next_positions()) {
      std::string new_prefix =
          next_attempt.prefix + board[row_index][col_index];
      if (trie.search(new_prefix)) {
        found_words.insert(new_prefix);
      }
      if (trie.startsWith(new_prefix)) {
        attempts.push_back(IntermediateResult(row_index, col_index,
                                              std::move(new_prefix),
                                              next_attempt.seen_positions));
      }
    }
  }

  return std::vector<std::string>(found_words.begin(), found_words.end());
}
