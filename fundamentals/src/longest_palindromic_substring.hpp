#pragma once

#include <string>
#include <vector>

// Longest Palindromic Substring (LeetCode 5)
//
// Given a string s, return the longest substring that is a palindrome.
// If there are multiple answers of the same length, return any one of them.
//
// Example 1:
//   s = "babad"
//   Answer: "bab" (or "aba" — both valid)
//
// Example 2:
//   s = "cbbd"
//   Answer: "bb"
//
// Example 3:
//   s = "a"
//   Answer: "a"
//
// Example 4:
//   s = "ac"
//   Answer: "a" (or "c")
//
// Constraints:
//   - 1 <= s.length <= 1000
//   - s consists of only digits and English letters

inline std::string longestPalindrome(const std::string &s) {
  std::string longest_palindrome{s[0]};
  // Come back to the actual palindrome returned
  if (s.size() == 1) {
    return longest_palindrome;
  }

  std::vector<std::vector<bool>> is_substring_palindrome(
      s.size() + 1, std::vector<bool>(s.size() + 1, false));

  // Base cases
  for (size_t index = 0; index < s.size(); ++index) {
    // Single character is always a palindrome
    is_substring_palindrome[index][index] = true;

    // Length 2 while we're here
    if (index < s.size() - 2 + 1) {
      auto left_index = index;
      auto right_index = index + 2 - 1;
      if (s[left_index] == s[right_index]) {
        is_substring_palindrome[left_index][right_index] = true;
        longest_palindrome = s.substr(index, 2);
      }
    }
  }

  for (size_t length = 3; length <= s.size(); ++length) {
    for (size_t index = 0; index < s.size() - length + 1; ++index) {
      auto left_index = index;
      auto right_index = index + length - 1;
      auto left_end = s[left_index];
      auto right_end = s[right_index];
      if (left_end != right_end) {
        continue;
      }
      // They match, need to check if the two between match
      if (!is_substring_palindrome[left_index + 1][right_index - 1]) {
        continue;
      }
      is_substring_palindrome[left_index][right_index] = true;
      if (length <= longest_palindrome.size()) {
        continue;
      }
      longest_palindrome = s.substr(left_index, length);
    }
  }

  return longest_palindrome;
}
