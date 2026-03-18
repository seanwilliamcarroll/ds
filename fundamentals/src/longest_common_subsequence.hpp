#pragma once

#include <cstddef>
#include <string>
#include <vector>

// Longest Common Subsequence (LeetCode 1143)
//
// Given two strings, return the length of their longest common subsequence.
// A subsequence is a sequence that can be derived from the string by deleting
// some (or no) characters without changing the order of the remaining ones.
//
// Example 1:
//   text1 = "abcde", text2 = "ace"
//   Answer: 3 ("ace")
//
// Example 2:
//   text1 = "abc", text2 = "abc"
//   Answer: 3 ("abc")
//
// Example 3:
//   text1 = "abc", text2 = "def"
//   Answer: 0 (no common subsequence)
//
// Constraints:
//   - 1 <= text1.length, text2.length <= 1000
//   - text1 and text2 consist of only lowercase English characters

inline int longestCommonSubsequence(const std::string &text1,
                                    const std::string &text2) {

  std::vector<std::vector<size_t>> lcs_so_far(
      text1.size() + 1, std::vector<size_t>(text2.size() + 1, 0));

  for (size_t index_1 = 1; index_1 < text1.size() + 1; ++index_1) {
    for (size_t index_2 = 1; index_2 < text2.size() + 1; ++index_2) {
      if (text1[index_1 - 1] == text2[index_2 - 1]) {
        lcs_so_far[index_1][index_2] = lcs_so_far[index_1 - 1][index_2 - 1] + 1;
      } else {
        lcs_so_far[index_1][index_2] = std::max(
            lcs_so_far[index_1][index_2 - 1], lcs_so_far[index_1 - 1][index_2]);
      }
    }
  }

  return static_cast<int>(lcs_so_far.back().back());
}
