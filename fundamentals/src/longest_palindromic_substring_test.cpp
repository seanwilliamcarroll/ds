#include "longest_palindromic_substring.hpp"

#include <algorithm>
#include <gtest/gtest.h>

// Helper: check that a result is a palindrome of the expected length
// and is a substring of the input. Needed because multiple valid answers exist.
static void expectPalindrome(const std::string &input,
                             const std::string &result, size_t expected_len) {
  EXPECT_EQ(result.size(), expected_len)
      << "input: \"" << input << "\", got: \"" << result << "\"";
  // Is it a palindrome?
  std::string rev = result;
  std::reverse(rev.begin(), rev.end());
  EXPECT_EQ(result, rev) << "not a palindrome: \"" << result << "\"";
  // Is it a substring of the input?
  EXPECT_NE(input.find(result), std::string::npos)
      << "\"" << result << "\" is not a substring of \"" << input << "\"";
}

TEST(LongestPalindrome, OddLength) {
  expectPalindrome("babad", longestPalindrome("babad"), 3);
}

TEST(LongestPalindrome, EvenLength) {
  EXPECT_EQ(longestPalindrome("cbbd"), "bb");
}

TEST(LongestPalindrome, SingleChar) { EXPECT_EQ(longestPalindrome("a"), "a"); }

TEST(LongestPalindrome, TwoDistinct) {
  expectPalindrome("ac", longestPalindrome("ac"), 1);
}

TEST(LongestPalindrome, TwoSame) { EXPECT_EQ(longestPalindrome("aa"), "aa"); }

TEST(LongestPalindrome, EntireString) {
  EXPECT_EQ(longestPalindrome("racecar"), "racecar");
}

TEST(LongestPalindrome, AllSameChar) {
  EXPECT_EQ(longestPalindrome("aaaa"), "aaaa");
}

TEST(LongestPalindrome, PalindromeAtStart) {
  expectPalindrome("abacxyz", longestPalindrome("abacxyz"), 3);
}

TEST(LongestPalindrome, PalindromeAtEnd) {
  EXPECT_EQ(longestPalindrome("xyzabcba"), "abcba");
}

TEST(LongestPalindrome, PalindromeInMiddle) {
  // "xabacbabcba" — longest palindromes are "cbabc" (4-8) or "abcba" (6-10)
  expectPalindrome("xabacbabcba", longestPalindrome("xabacbabcba"), 5);
}

TEST(LongestPalindrome, NoPalindromeOverTwo) {
  // All distinct characters — every single char is a palindrome of length 1
  expectPalindrome("abcdef", longestPalindrome("abcdef"), 1);
}

TEST(LongestPalindrome, LongerEvenPalindrome) {
  EXPECT_EQ(longestPalindrome("xabccbayz"), "abccba");
}

TEST(LongestPalindrome, NestedPalindromes) {
  // "abaaba" contains "aba", "baab", "abaaba"
  EXPECT_EQ(longestPalindrome("abaaba"), "abaaba");
}

TEST(LongestPalindrome, DigitsAndLetters) {
  EXPECT_EQ(longestPalindrome("a1221b"), "1221");
}
