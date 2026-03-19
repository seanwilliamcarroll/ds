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

TEST(LongestPalindrome, EntireStringEven) {
  // "abccba" reversed = "abccba" ✓ length 6
  EXPECT_EQ(longestPalindrome("abccba"), "abccba");
}

TEST(LongestPalindrome, EntireStringOddLonger) {
  // "abacaba" reversed = "abacaba" ✓ length 7
  EXPECT_EQ(longestPalindrome("abacaba"), "abacaba");
}

TEST(LongestPalindrome, EvenEntireStringLong) {
  // "abbaabba" reversed = "abbaabba" ✓ length 8
  EXPECT_EQ(longestPalindrome("abbaabba"), "abbaabba");
}

TEST(LongestPalindrome, ThreeCharPalindrome) {
  EXPECT_EQ(longestPalindrome("aba"), "aba");
}

TEST(LongestPalindrome, DeeplyNested) {
  // "qwertyabcdefedcbapoiuy"
  // "abcdefedcba" at indices 6-16: a-b-c-d-e-f-e-d-c-b-a
  // reversed = a-b-c-d-e-f-e-d-c-b-a ✓ length 11
  EXPECT_EQ(longestPalindrome("qwertyabcdefedcbapoiuy"), "abcdefedcba");
}

TEST(LongestPalindrome, TwoEqualLengthPalindromes) {
  // "abcba" at 0-4: a-b-c-b-a ✓ length 5
  // "xyzyx" at 5-9: x-y-z-y-x ✓ length 5
  // Nothing spans both (s[4]='a', s[5]='x' don't help extend)
  expectPalindrome("abcbaxyzyxw", longestPalindrome("abcbaxyzyxw"), 5);
}

TEST(LongestPalindrome, EvenBetweenOdd) {
  // "xabbay": "abba" at 1-4, a-b-b-a reversed a-b-b-a ✓ length 4
  EXPECT_EQ(longestPalindrome("xabbay"), "abba");
}

TEST(LongestPalindrome, PickLongestAmongMultiple) {
  // "abba" at 3-6, length 4
  // "racecar" at 10-16: r-a-c-e-c-a-r ✓ length 7
  EXPECT_EQ(longestPalindrome("xyzabbauvwracecarjkl"), "racecar");
}

TEST(LongestPalindrome, AlternatingChars) {
  // "ababababab" (length 10)
  // Odd-length palindromes centered at each position:
  //   Centered at 0: "ababababa" (0-8) length 9 ✓
  //     a-b-a-b-a-b-a-b-a reversed = a-b-a-b-a-b-a-b-a ✓
  //   Centered at 1: "babababab" (1-9) length 9 ✓
  // Whole string (length 10): "ababababab" reversed = "bababababa" ✗
  expectPalindrome("ababababab", longestPalindrome("ababababab"), 9);
}

TEST(LongestPalindrome, NoRepeatedChars) {
  // "abcabcabc" — check all centers, no adjacent pairs match,
  // no 3-char palindrome (neighbors never equal around any center)
  expectPalindrome("abcabcabc", longestPalindrome("abcabcabc"), 1);
}

TEST(LongestPalindrome, MultipleShortEvenPalindromes) {
  // "aabbcc": "aa" at 0-1, "bb" at 2-3, "cc" at 4-5, all length 2
  // No length-3+ palindrome: "aab" no, "abb" no, "bbc" no, "bcc" no
  expectPalindrome("aabbcc", longestPalindrome("aabbcc"), 2);
}

TEST(LongestPalindrome, OverlappingExpansions) {
  // "aabaa": a-a-b-a-a reversed = a-a-b-a-a ✓ length 5 (entire string)
  EXPECT_EQ(longestPalindrome("aabaa"), "aabaa");
}

TEST(LongestPalindrome, FirstLastMatchButNotPalindrome) {
  // "abcda": first and last are 'a', but "abcda" reversed = "adcba" ✗
  // Longest is just length 1
  expectPalindrome("abcda", longestPalindrome("abcda"), 1);
}

TEST(LongestPalindrome, NearlyPalindrome) {
  // "abcbd": "bcb" at 1-3, b-c-b reversed b-c-b ✓ length 3
  EXPECT_EQ(longestPalindrome("abcbd"), "bcb");
}
