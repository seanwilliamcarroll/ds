#include "longest_common_subsequence.hpp"

#include <gtest/gtest.h>

TEST(LCS, BasicMatch) {
  EXPECT_EQ(longestCommonSubsequence("abcde", "ace"), 3);
}

TEST(LCS, IdenticalStrings) {
  EXPECT_EQ(longestCommonSubsequence("abc", "abc"), 3);
}

TEST(LCS, NoCommon) { EXPECT_EQ(longestCommonSubsequence("abc", "def"), 0); }

TEST(LCS, SingleChar) { EXPECT_EQ(longestCommonSubsequence("a", "a"), 1); }

TEST(LCS, SingleCharNoMatch) {
  EXPECT_EQ(longestCommonSubsequence("a", "b"), 0);
}

TEST(LCS, OneEmpty) {
  // Constraint says length >= 1, but good to handle edge behavior
  EXPECT_EQ(longestCommonSubsequence("abc", ""), 0);
  EXPECT_EQ(longestCommonSubsequence("", "abc"), 0);
}

TEST(LCS, SubsequenceNotSubstring) {
  // "aec" is not a substring of "abcde" but "ace" is a subsequence
  // LCS of "abcde" and "aec" is "ac" (length 2) — the 'e' in "aec" comes
  // before 'c', but in "abcde" 'e' comes after 'c', so "aec" can't be matched
  // as a full subsequence. Best is "a" + "c" = 2, or "a" + "e" = 2.
  EXPECT_EQ(longestCommonSubsequence("abcde", "aec"), 2);
}

TEST(LCS, RepeatedCharacters) {
  EXPECT_EQ(longestCommonSubsequence("aaa", "aa"), 2);
}

TEST(LCS, InterleavedPattern) {
  // text1: "axbycz", text2: "abc" → LCS is "abc" (length 3)
  EXPECT_EQ(longestCommonSubsequence("axbycz", "abc"), 3);
}

TEST(LCS, LongerExample) {
  // Classic example: "ABCBDAB" and "BDCAB" → LCS is "BCAB" (length 4)
  EXPECT_EQ(longestCommonSubsequence("ABCBDAB", "BDCAB"), 4);
}

TEST(LCS, Palindromic) {
  // "abcba" and "abcba" — identical, LCS = 5
  EXPECT_EQ(longestCommonSubsequence("abcba", "abcba"), 5);
}

TEST(LCS, DifferentLengths) {
  // "ab" and "azb" → LCS is "ab" (length 2)
  EXPECT_EQ(longestCommonSubsequence("ab", "azb"), 2);
}

TEST(LCS, AllSameChar) { EXPECT_EQ(longestCommonSubsequence("aaaa", "aa"), 2); }
