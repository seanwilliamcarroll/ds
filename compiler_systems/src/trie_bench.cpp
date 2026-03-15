#include "data_oriented_index_arena_trie.hpp"
#include "deque_arena_trie.hpp"
#include "index_arena_trie.hpp"
#include "ptr_trie.hpp"

#include <benchmark/benchmark.h>
#include <string>
#include <vector>

// --- Helpers ---

// Generate words with shared prefixes (dense trie)
static std::vector<std::string> generateDenseWords(int64_t count) {
  std::vector<std::string> words;
  words.reserve(static_cast<size_t>(count));
  for (int64_t i = 0; i < count; ++i) {
    // Creates words like "aaa", "aab", "aac", ... sharing long prefixes
    std::string word;
    int64_t val = i;
    for (int64_t d = 0; d < 6; ++d) {
      word += 'a' + (val % 10);
      val /= 10;
    }
    words.push_back(word);
  }
  return words;
}

// Generate words with little prefix sharing (sparse trie)
static std::vector<std::string> generateSparseWords(int64_t count) {
  std::vector<std::string> words;
  words.reserve(static_cast<size_t>(count));
  for (int64_t i = 0; i < count; ++i) {
    std::string word;
    int64_t val = i;
    for (int64_t d = 0; d < 6; ++d) {
      word += 'a' + (val % 26);
      val /= 26;
    }
    words.push_back(word);
  }
  return words;
}

using IndexArenaSentinel = IndexArenaTrie<true>;
using IndexArenaZeroNull = IndexArenaTrie<false>;

// --- Insert benchmarks ---

template <typename T> static void BM_InsertDense(benchmark::State &state) {
  int64_t n = state.range(0);
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
  int64_t n = state.range(0);
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
  int64_t n = state.range(0);
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
  int64_t n = state.range(0);
  auto words = generateDenseWords(n);
  T trie;
  for (const auto &w : words) {
    trie.insert(w);
  }

  for (auto _ : state) {
    for (int64_t i = 0; i < n; ++i) {
      benchmark::DoNotOptimize(trie.search("zzzzzzz"));
    }
  }
}

// --- Register IndexArenaTrie (sentinel) benchmarks ---

BENCHMARK(BM_InsertDense<IndexArenaSentinel>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_InsertSparse<IndexArenaSentinel>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_SearchHitDense<IndexArenaSentinel>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_SearchMiss<IndexArenaSentinel>)->Range(1 << 8, 1 << 16);

// --- Register IndexArenaTrie (zero-as-null) benchmarks ---

BENCHMARK(BM_InsertDense<IndexArenaZeroNull>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_InsertSparse<IndexArenaZeroNull>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_SearchHitDense<IndexArenaZeroNull>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_SearchMiss<IndexArenaZeroNull>)->Range(1 << 8, 1 << 16);

// --- Register DataOrientedIndexArenaTrie benchmarks ---

BENCHMARK(BM_InsertDense<DataOrientedIndexArenaTrie>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_InsertSparse<DataOrientedIndexArenaTrie>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_SearchHitDense<DataOrientedIndexArenaTrie>)
    ->Range(1 << 8, 1 << 16);
BENCHMARK(BM_SearchMiss<DataOrientedIndexArenaTrie>)->Range(1 << 8, 1 << 16);

// --- Register DequeArenaTrie benchmarks ---

BENCHMARK(BM_InsertDense<DequeArenaTrie>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_InsertSparse<DequeArenaTrie>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_SearchHitDense<DequeArenaTrie>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_SearchMiss<DequeArenaTrie>)->Range(1 << 8, 1 << 16);

// --- Register PtrTrie benchmarks ---

BENCHMARK(BM_InsertDense<PtrTrie>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_InsertSparse<PtrTrie>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_SearchHitDense<PtrTrie>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_SearchMiss<PtrTrie>)->Range(1 << 8, 1 << 16);

// --- getWordsWithPrefix benchmarks ---

template <typename T>
static void BM_GetWordsWithPrefix(benchmark::State &state) {
  int64_t n = state.range(0);
  auto words = generateDenseWords(n);
  T trie;
  for (const auto &w : words) {
    trie.insert(w);
  }

  for (auto _ : state) {
    benchmark::DoNotOptimize(trie.getWordsWithPrefix("aaa"));
  }
}

BENCHMARK(BM_GetWordsWithPrefix<IndexArenaSentinel>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_GetWordsWithPrefix<IndexArenaZeroNull>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_GetWordsWithPrefix<DequeArenaTrie>)->Range(1 << 8, 1 << 16);
BENCHMARK(BM_GetWordsWithPrefix<DataOrientedIndexArenaTrie>)
    ->Range(1 << 8, 1 << 16);
BENCHMARK(BM_GetWordsWithPrefix<PtrTrie>)->Range(1 << 8, 1 << 16);

BENCHMARK_MAIN();
