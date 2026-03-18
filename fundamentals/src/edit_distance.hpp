#pragma once

#include <string>

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

inline int minDistance(const std::string &word1, const std::string &word2) {
  return -1; // TODO
}
