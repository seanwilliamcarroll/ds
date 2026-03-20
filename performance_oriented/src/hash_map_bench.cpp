#include "chaining_hash_map.hpp"
#include "linear_probing_hash_map.hpp"
#include "robin_hood_hash_map.hpp"
#include "std_unordered_map_adapter.hpp"

#include <benchmark/benchmark.h>
#include <cstdint>

// --- Scenarios ---

// 1. Insert: insert N sequential keys into an empty map
template <typename T> static void BM_Insert(benchmark::State &state) {
  auto n = state.range(0);
  for (auto _ : state) {
    T map;
    for (int64_t i = 0; i < n; ++i) {
      map.insert(static_cast<int>(i), static_cast<int>(i));
    }
    benchmark::DoNotOptimize(map);
  }
}

// 2. Find hit: pre-fill N keys, then find all of them
template <typename T> static void BM_FindHit(benchmark::State &state) {
  auto n = state.range(0);
  T map;
  for (int64_t i = 0; i < n; ++i) {
    map.insert(static_cast<int>(i), static_cast<int>(i));
  }

  for (auto _ : state) {
    for (int64_t i = 0; i < n; ++i) {
      benchmark::DoNotOptimize(map.find(static_cast<int>(i)));
    }
  }
}

// 3. Find miss: pre-fill N keys [0, N), then find N keys that don't exist
template <typename T> static void BM_FindMiss(benchmark::State &state) {
  auto n = state.range(0);
  T map;
  for (int64_t i = 0; i < n; ++i) {
    map.insert(static_cast<int>(i), static_cast<int>(i));
  }

  for (auto _ : state) {
    for (int64_t i = n; i < 2 * n; ++i) {
      benchmark::DoNotOptimize(map.find(static_cast<int>(i)));
    }
  }
}

// 4. Erase + find survivors: pre-fill N keys, erase even keys, find odd keys
template <typename T> static void BM_EraseAndFind(benchmark::State &state) {
  auto n = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();
    T map;
    for (int64_t i = 0; i < n; ++i) {
      map.insert(static_cast<int>(i), static_cast<int>(i));
    }
    for (int64_t i = 0; i < n; i += 2) {
      map.erase(static_cast<int>(i));
    }
    state.ResumeTiming();

    for (int64_t i = 1; i < n; i += 2) {
      benchmark::DoNotOptimize(map.find(static_cast<int>(i)));
    }
  }
}

// 5. Erase-heavy churn: pre-fill N/2 keys, then repeatedly insert+erase
//    Measures combined insert+erase throughput under churn
template <typename T> static void BM_EraseChurn(benchmark::State &state) {
  auto n = state.range(0);
  auto half = n / 2;

  T map;
  for (int64_t i = 0; i < half; ++i) {
    map.insert(static_cast<int>(i), static_cast<int>(i));
  }

  int next_key = static_cast<int>(half);
  for (auto _ : state) {
    // Insert a new key, erase an old one
    map.insert(next_key, next_key);
    map.erase(next_key - static_cast<int>(half));
    ++next_key;
  }
}

// --- Registration ---
// 5 scenarios x 4 implementations x 3 load factors = 60 benchmarks
// The 0.9 load factor runs of FindHit/FindMiss correspond to the
// "high load find" scenario in the hypotheses.

// clang-format off
#define REGISTER_ALL(BM, Load)                                                 \
  BENCHMARK(BM<ChainingHashMap<Load>>)->Range(1 << 8, 1 << 16);               \
  BENCHMARK(BM<LinearProbingHashMap<Load>>)->Range(1 << 8, 1 << 16);          \
  BENCHMARK(BM<RobinHoodHashMap<Load>>)->Range(1 << 8, 1 << 16);             \
  BENCHMARK(BM<StdUnorderedMapAdapter<Load>>)->Range(1 << 8, 1 << 16)

REGISTER_ALL(BM_Insert, 0.5);
REGISTER_ALL(BM_Insert, 0.75);
REGISTER_ALL(BM_Insert, 0.9);

REGISTER_ALL(BM_FindHit, 0.5);
REGISTER_ALL(BM_FindHit, 0.75);
REGISTER_ALL(BM_FindHit, 0.9);

REGISTER_ALL(BM_FindMiss, 0.5);
REGISTER_ALL(BM_FindMiss, 0.75);
REGISTER_ALL(BM_FindMiss, 0.9);

REGISTER_ALL(BM_EraseAndFind, 0.5);
REGISTER_ALL(BM_EraseAndFind, 0.75);
REGISTER_ALL(BM_EraseAndFind, 0.9);

REGISTER_ALL(BM_EraseChurn, 0.5);
REGISTER_ALL(BM_EraseChurn, 0.75);
REGISTER_ALL(BM_EraseChurn, 0.9);
// clang-format on

#undef REGISTER_ALL

BENCHMARK_MAIN();
