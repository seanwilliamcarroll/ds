#pragma once

#include <cstddef>
#include <limits>
#include <string>
#include <vector>

// Edit Distance (LeetCode 72)
//
// Given two strings word1 and word2, return the minimum number of operations
// required to convert word1 into word2. You have three operations:
//   - Insert a character
//   - Delete a character
//   - Replace a character
//
// Example 1:
//   word1 = "horse", word2 = "ros"
//   Answer: 3
//     horse → rorse (replace 'h' with 'r')
//     rorse → rose  (delete 'r')
//     rose  → ros   (delete 'e')
//
// Example 2:
//   word1 = "intention", word2 = "execution"
//   Answer: 5
//     intention → inention  (delete 't')
//     inention  → enention  (replace 'i' with 'e')
//     enention  → exention  (replace 'n' with 'x')
//     exention  → exection  (replace 'n' with 'c')
//     exection  → execution (insert 'u')
//
// Constraints:
//   - 0 <= word1.length, word2.length <= 500
//   - word1 and word2 consist of lowercase English letters

inline int min_distance(const std::string &word1, const std::string &word2) {

  std::vector<std::vector<size_t>> min_distance_so_far(
      word1.size() + 1, std::vector<size_t>(word2.size() + 1, 0));

  // Need to initialize the base case properly
  // This means we'd delete all index chars in word1 to get empty string
  for (size_t index = 1; index < word1.size() + 1; ++index) {
    min_distance_so_far[index][0] = index;
  }

  // This means we'd delete all index chars in word2 to get empty string
  for (size_t index = 1; index < word2.size() + 1; ++index) {
    min_distance_so_far[0][index] = index;
  }

  // At any point comparing, if a match, ours is (i-1,j-i)
  // If not, min of 1 +
  //     (i, j-1) // delete
  //     (i-1, j-1) // replace
  //     (i-1, j) // insert?

  for (size_t index_1 = 1; index_1 < word1.size() + 1; ++index_1) {
    for (size_t index_2 = 1; index_2 < word2.size() + 1; ++index_2) {
      if (word1[index_1 - 1] == word2[index_2 - 1]) {
        min_distance_so_far[index_1][index_2] =
            min_distance_so_far[index_1 - 1][index_2 - 1];
      } else {
        min_distance_so_far[index_1][index_2] =
            1 + std::min({
                    min_distance_so_far[index_1][index_2 - 1],
                    min_distance_so_far[index_1 - 1][index_2 - 1],
                    min_distance_so_far[index_1 - 1][index_2],
                });
      }
    }
  }

  return static_cast<int>(min_distance_so_far.back().back());
}
