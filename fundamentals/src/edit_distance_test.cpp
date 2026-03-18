#include "edit_distance.hpp"

#include <gtest/gtest.h>

TEST(EditDistance, HorseToRos) { EXPECT_EQ(minDistance("horse", "ros"), 3); }

TEST(EditDistance, IntentionToExecution) {
  EXPECT_EQ(minDistance("intention", "execution"), 5);
}

TEST(EditDistance, BothEmpty) { EXPECT_EQ(minDistance("", ""), 0); }

TEST(EditDistance, FirstEmpty) { EXPECT_EQ(minDistance("", "abc"), 3); }

TEST(EditDistance, SecondEmpty) { EXPECT_EQ(minDistance("abc", ""), 3); }

TEST(EditDistance, Identical) { EXPECT_EQ(minDistance("abc", "abc"), 0); }

TEST(EditDistance, SingleReplace) { EXPECT_EQ(minDistance("a", "b"), 1); }

TEST(EditDistance, SingleInsert) { EXPECT_EQ(minDistance("a", "ab"), 1); }

TEST(EditDistance, SingleDelete) { EXPECT_EQ(minDistance("ab", "a"), 1); }

TEST(EditDistance, CompleteRewrite) {
  // No characters in common — must replace all (or delete + insert)
  EXPECT_EQ(minDistance("abc", "xyz"), 3);
}

TEST(EditDistance, PrefixMatch) {
  // "abc" → "abcdef" requires 3 inserts
  EXPECT_EQ(minDistance("abc", "abcdef"), 3);
}

TEST(EditDistance, SuffixMatch) {
  // "def" → "abcdef" requires 3 inserts
  EXPECT_EQ(minDistance("def", "abcdef"), 3);
}

TEST(EditDistance, Symmetric) {
  // Edit distance is symmetric
  EXPECT_EQ(minDistance("kitten", "sitting"), minDistance("sitting", "kitten"));
}

TEST(EditDistance, KittenToSitting) {
  // kitten → sitten (replace k→s)
  // sitten → sittin (replace e→i)
  // sittin → sitting (insert g)
  EXPECT_EQ(minDistance("kitten", "sitting"), 3);
}

TEST(EditDistance, RepeatedChars) {
  // "aaa" → "aaaa" — one insert
  EXPECT_EQ(minDistance("aaa", "aaaa"), 1);
}

TEST(EditDistance, Anagram) {
  // "abc" → "bca" — not free just because same chars
  // abc → bca: replace a→b, keep b? No.
  // abc → bbc (replace a→b) → bcc (replace b→c) → bca (replace c→a) = 3?
  // abc → bac (replace a→b, replace b→a) = swap = 2 ops, then bac → bca = 1
  // more = 3? Actually: abc → bac (2 replaces is wrong, that's just 1 swap but
  // swaps aren't an op) abc → bbc (replace a→b) → bbc → bcc (replace b→c) → bca
  // (replace c→a) = 3 Or: abc → bca directly? a→b, b→c, c→a = 3 replaces? No:
  // pos0: a→b, pos1: b→c, pos2: c→a = 3 replaces. But maybe cheaper:
  // abc → abca (insert a) → bca (delete first a, delete b)... that's 3 ops too.
  // Minimum is 2: abc → abc→bca? Let me think...
  // abc → bac (swap a,b = replace a→b at pos0, replace b→a at pos1 = 2 ops) →
  // bca (replace a→c at pos2, replace c→a... no that's more) Actually just
  // trust DP: known answer is 2.
  EXPECT_EQ(minDistance("abc", "bca"), 2);
}

TEST(EditDistance, LongerExample) {
  // "saturday" → "sunday"
  // Known edit distance = 3
  EXPECT_EQ(minDistance("saturday", "sunday"), 3);
}
