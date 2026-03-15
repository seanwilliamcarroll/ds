#pragma once

#include "index_arena_trie.hpp"
#include <bitset>
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

static const size_t MAX_ROWS = 12;
static const size_t MAX_COLS = 12;

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

  IndexArenaTrie<false> trie;
  for (const auto &word : words) {
    trie.insert(word);
  }

  using Position = std::pair<size_t, size_t>;

  // Want to do DFS across the board
  struct IntermediateResult {
    Position last_position;
    std::string prefix;
    IndexArenaTrie<false>::Cursor cursor;
    std::bitset<MAX_ROWS * MAX_COLS> seen_positions;

    IntermediateResult(size_t num_cols, size_t row_index, size_t col_index,
                       std::string prefix, IndexArenaTrie<false>::Cursor cursor,
                       std::bitset<MAX_ROWS * MAX_COLS> seen_positions)
        : last_position{row_index, col_index}, prefix(std::move(prefix)),
          cursor(std::move(cursor)), seen_positions(seen_positions) {
      this->seen_positions[num_cols * row_index + col_index] = true;
    }

    std::vector<Position> get_possible_next_positions(size_t num_rows,
                                                      size_t num_cols) const {
      std::vector<Position> output;

      auto [row_index, col_index] = last_position;

      auto was_seen = [num_cols, this](size_t row, size_t col) {
        return seen_positions[row * num_cols + col];
      };

      if (row_index > 0 && !was_seen(row_index - 1, col_index)) {
        output.push_back({row_index - 1, col_index});
      }

      if (col_index > 0 && !was_seen(row_index, col_index - 1)) {
        output.push_back({row_index, col_index - 1});
      }

      if (row_index < num_rows - 1 && !was_seen(row_index + 1, col_index)) {
        output.push_back({row_index + 1, col_index});
      }

      if (col_index < num_cols - 1 && !was_seen(row_index, col_index + 1)) {
        output.push_back({row_index, col_index + 1});
      }

      return output;
    }
  };

  std::bitset<MAX_ROWS * MAX_COLS> initial_seen_positions;

  std::vector<IntermediateResult> attempts;
  {
    size_t row_index = 0;
    for (const auto &row : board) {
      size_t col_index = 0;
      for (const auto character : row) {
        std::string first_letter{character};
        auto root_cursor = trie.cursor();
        auto cursor = root_cursor.advance(character);
        if (cursor.is_word()) {
          found_words.insert(first_letter);
        }
        if (cursor.is_valid()) {
          attempts.push_back(
              IntermediateResult(board.back().size(), row_index, col_index,
                                 std::move(first_letter), std::move(cursor),
                                 initial_seen_positions));
        }
        ++col_index;
      }
      ++row_index;
    }
  }
  // DFS queue initialized

  while (!attempts.empty()) {
    // Pop next attempt off the stack
    auto next_attempt = std::move(attempts.back());
    attempts.pop_back();

    // Check possible next positions
    for (const auto &[row_index, col_index] :
         next_attempt.get_possible_next_positions(board.size(),
                                                  board.back().size())) {
      // Try to advance cursor
      auto new_cursor =
          next_attempt.cursor.advance(board[row_index][col_index]);
      if (!new_cursor.is_valid()) {
        continue;
      }
      auto new_prefix = next_attempt.prefix + board[row_index][col_index];
      if (new_cursor.is_word()) {
        found_words.insert(new_prefix);
      }
      attempts.push_back(IntermediateResult(
          board.back().size(), row_index, col_index, std::move(new_prefix),
          std::move(new_cursor), next_attempt.seen_positions));
    }
  }

  return std::vector<std::string>(found_words.begin(), found_words.end());
}
