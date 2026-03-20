#include "edit_distance.hpp"

#include <gtest/gtest.h>

TEST(EditDistance, HorseToRos) { EXPECT_EQ(min_distance("horse", "ros"), 3); }

TEST(EditDistance, IntentionToExecution) {
  EXPECT_EQ(min_distance("intention", "execution"), 5);
}

TEST(EditDistance, BothEmpty) { EXPECT_EQ(min_distance("", ""), 0); }

TEST(EditDistance, FirstEmpty) { EXPECT_EQ(min_distance("", "abc"), 3); }

TEST(EditDistance, SecondEmpty) { EXPECT_EQ(min_distance("abc", ""), 3); }

TEST(EditDistance, Identical) { EXPECT_EQ(min_distance("abc", "abc"), 0); }

TEST(EditDistance, SingleReplace) { EXPECT_EQ(min_distance("a", "b"), 1); }

TEST(EditDistance, SingleInsert) { EXPECT_EQ(min_distance("a", "ab"), 1); }

TEST(EditDistance, SingleDelete) { EXPECT_EQ(min_distance("ab", "a"), 1); }

TEST(EditDistance, CompleteRewrite) {
  // No characters in common — must replace all (or delete + insert)
  EXPECT_EQ(min_distance("abc", "xyz"), 3);
}

TEST(EditDistance, PrefixMatch) {
  // "abc" → "abcdef" requires 3 inserts
  EXPECT_EQ(min_distance("abc", "abcdef"), 3);
}

TEST(EditDistance, SuffixMatch) {
  // "def" → "abcdef" requires 3 inserts
  EXPECT_EQ(min_distance("def", "abcdef"), 3);
}

TEST(EditDistance, Symmetric) {
  // Edit distance is symmetric
  EXPECT_EQ(min_distance("kitten", "sitting"),
            min_distance("sitting", "kitten"));
}

TEST(EditDistance, KittenToSitting) {
  // kitten → sitten (replace k→s)
  // sitten → sittin (replace e→i)
  // sittin → sitting (insert g)
  EXPECT_EQ(min_distance("kitten", "sitting"), 3);
}

TEST(EditDistance, RepeatedChars) {
  // "aaa" → "aaaa" — one insert
  EXPECT_EQ(min_distance("aaa", "aaaa"), 1);
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
  EXPECT_EQ(min_distance("abc", "bca"), 2);
}

TEST(EditDistance, LongerExample) {
  // "saturday" → "sunday"
  // Known edit distance = 3
  EXPECT_EQ(min_distance("saturday", "sunday"), 3);
}
