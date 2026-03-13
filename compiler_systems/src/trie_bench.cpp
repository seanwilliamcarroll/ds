#include "trie.hpp"

#include <benchmark/benchmark.h>
#include <string>
#include <vector>

// --- Helpers ---

// Generate words with shared prefixes (dense trie)
static std::vector<std::string> generateDenseWords(int count) {
  std::vector<std::string> words;
  words.reserve(count);
  for (int i = 0; i < count; ++i) {
    // Creates words like "aaa", "aab", "aac", ... sharing long prefixes
    std::string word;
    int val = i;
    for (int d = 0; d < 6; ++d) {
      word += 'a' + (val % 10);
      val /= 10;
    }
    words.push_back(word);
  }
  return words;
}

// Generate words with little prefix sharing (sparse trie)
static std::vector<std::string> generateSparseWords(int count) {
  std::vector<std::string> words;
  words.reserve(count);
  for (int i = 0; i < count; ++i) {
    std::string word;
    int val = i;
    for (int d = 0; d < 6; ++d) {
      word += 'a' + (val % 26);
      val /= 26;
    }
    words.push_back(word);
  }
  return words;
}

// --- Insert benchmarks ---

template <typename T> static void BM_InsertDense(benchmark::State &state) {
  int n = state.range(0);
  auto words = generateDenseWords(n);
  for (auto _ : state) {
    T trie;
    for (const auto &w : words) {
      trie.insert(w);
    }
    benchmark::DoNotOptimize(trie);
  }
}

template <typename T> static void BM_InsertSparse(benchmark::State &state) {
  int n = state.range(0);
  auto words = generateSparseWords(n);
  for (auto _ : state) {
    T trie;
    for (const auto &w : words) {
      trie.insert(w);
    }
    benchmark::DoNotOptimize(trie);
  }
}

// --- Search benchmarks ---

template <typename T> static void BM_SearchHitDense(benchmark::State &state) {
  int n = state.range(0);
  auto words = generateDenseWords(n);
  T trie;
  for (const auto &w : words) {
    trie.insert(w);
  }

  for (auto _ : state) {
    for (const auto &w : words) {
      benchmark::DoNotOptimize(trie.search(w));
    }
  }
}

template <typename T> static void BM_SearchMiss(benchmark::State &state) {
  int n = state.range(0);
  auto words = generateDenseWords(n);
  T trie;
  for (const auto &w : words) {
    trie.insert(w);
  }

  for (auto _ : state) {
    for (int i = 0; i < n; ++i) {
      benchmark::DoNotOptimize(trie.search("zzzzzzz"));
    }
  }
}

// --- Register Arena benchmarks ---

BENCHMARK(BM_InsertDense<ArenaTrie>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_InsertSparse<ArenaTrie>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_SearchHitDense<ArenaTrie>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_SearchMiss<ArenaTrie>)->Range(1 << 8, 1 << 16);

// --- Register PtrTrie benchmarks ---

BENCHMARK(BM_InsertDense<PtrTrie>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_InsertSparse<PtrTrie>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_SearchHitDense<PtrTrie>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_SearchMiss<PtrTrie>)->Range(1 << 8, 1 << 16);

// --- getWordsWithPrefix benchmarks ---

template <typename T>
static void BM_GetWordsWithPrefix(benchmark::State &state) {
  int n = state.range(0);
  auto words = generateDenseWords(n);
  T trie;
  for (const auto &w : words) {
    trie.insert(w);
  }

  for (auto _ : state) {
    benchmark::DoNotOptimize(trie.getWordsWithPrefix("aaa"));
  }
}

BENCHMARK(BM_GetWordsWithPrefix<ArenaTrie>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_GetWordsWithPrefix<PtrTrie>)->Range(1 << 8, 1 << 16);

BENCHMARK_MAIN();
