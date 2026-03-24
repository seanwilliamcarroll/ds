#include "chaining_hash_map.hpp"
#include "linear_probing_hash_map.hpp"
#include "robin_hood_hash_map.hpp"
#include "std_unordered_map_adapter.hpp"

#include <algorithm>
#include <benchmark/benchmark.h>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <random>
#include <unordered_set>
#include <vector>

// --- Key generators ---
// Each returns a vector of N unique keys with a specific distribution.
// Fixed seeds for reproducibility.

static std::vector<int> make_sequential_keys(int64_t n) {
  std::vector<int> keys(static_cast<size_t>(n));
  std::iota(keys.begin(), keys.end(), 0);
  return keys;
}

static std::vector<int> make_uniform_keys(int64_t n) {
  // Unique keys drawn uniformly from [0, 10N). Shuffle-and-take guarantees
  // uniqueness without rejection sampling. The 10x range keeps keys sparse
  // so identity hash scatters them across the table.
  auto range = static_cast<size_t>(n * 10);
  std::vector<int> pool(range);
  std::iota(pool.begin(), pool.end(), 0);
  std::mt19937 rng(42);
  std::shuffle(pool.begin(), pool.end(), rng);
  pool.resize(static_cast<size_t>(n));
  return pool;
}

static std::vector<int> make_normal_keys(int64_t n) {
  // Keys: normal_part + X * SLOT_STRIDE, where:
  //   normal_part ~ N(N/2, N/8), rounded to int — determines the hash slot
  //   X ~ Uniform[0, MAX_X) — spreads keys apart to guarantee uniqueness
  //
  // With identity hash and power-of-two tables, slot = key & (num_buckets-1).
  // SLOT_STRIDE = 1<<20 is a multiple of any table size <= 2^20 (~1M slots),
  // so the X term vanishes in the slot computation. All keys with the same
  // normal_part map to the same slot. ~95% of keys cluster within ±N/4 of
  // center.
  //
  // Overflow guard: max key ≈ MAX_X * (1<<20) + N. With MAX_X = 2000,
  // that's ~2.1 billion, safely under INT_MAX (2^31 - 1 ≈ 2.15 billion).
  constexpr int SLOT_STRIDE = 1 << 20;
  constexpr int MAX_X = 2000;
  std::mt19937 rng(99);
  auto mean = static_cast<double>(n) / 2.0;
  auto stddev = static_cast<double>(n) / 8.0;
  std::normal_distribution<double> dist(mean, stddev);
  std::uniform_int_distribution<int> x_dist(0, MAX_X - 1);

  std::unordered_set<int> seen;
  seen.reserve(static_cast<size_t>(n));
  std::vector<int> keys;
  keys.reserve(static_cast<size_t>(n));
  while (static_cast<int64_t>(keys.size()) < n) {
    auto normal_part = static_cast<int>(std::round(dist(rng)));
    int key = normal_part + x_dist(rng) * SLOT_STRIDE;
    if (seen.insert(key).second) {
      keys.push_back(key);
    }
  }
  return keys;
}

// --- Parameterized scenarios ---
// Each scenario is templated on Map type and key pattern.
// Lookup order matches the key distribution: sequential keys are looked up
// in order (cache-friendly), uniform/normal keys in insertion order
// (scattered). This way the access pattern reflects the distribution, not an
// artificial shuffle.

enum class KeyPattern { SEQUENTIAL, UNIFORM, NORMAL };

static std::vector<int> make_keys(KeyPattern pattern, int64_t n) {
  switch (pattern) {
  case KeyPattern::SEQUENTIAL:
    return make_sequential_keys(n);
  case KeyPattern::UNIFORM:
    return make_uniform_keys(n);
  case KeyPattern::NORMAL:
    return make_normal_keys(n);
  }
  __builtin_unreachable();
}

// Generate miss keys: disjoint from hit keys, same distribution shape.
//
// For sequential keys: miss keys are [N, 2N). With identity hash and a table
// of size 2*next_pow2(N), these map to the empty half of the table — fast
// misses via immediate empty-slot detection. Using any fixed offset risks
// landing inside the contiguous occupied block [0, N), which causes O(N)
// probes for LP.
//
// For uniform/normal: uses a large prime offset. The occupied slots are
// scattered (not contiguous), so the offset shifts miss keys into nearby
// but mostly-empty regions. A prime avoids aliasing with power-of-two
// table sizes.
static std::vector<int> make_miss_keys(KeyPattern pattern,
                                       const std::vector<int> &hit_keys) {
  auto n = static_cast<int64_t>(hit_keys.size());
  std::vector<int> miss(hit_keys.size());

  if (pattern == KeyPattern::SEQUENTIAL) {
    std::iota(miss.begin(), miss.end(), static_cast<int>(n));
  } else {
    constexpr int PRIME_OFFSET = 1000003;
    for (size_t i = 0; i < hit_keys.size(); ++i) {
      miss[i] = hit_keys[i] + PRIME_OFFSET;
    }
  }
  return miss;
}

// 1. Insert: insert N keys into an empty map
template <typename T, KeyPattern P>
static void BM_Insert(benchmark::State &state) {
  auto n = state.range(0);
  auto keys = make_keys(P, n);
  for (auto _ : state) {
    T map;
    for (int64_t i = 0; i < n; ++i) {
      map.insert(keys[static_cast<size_t>(i)], static_cast<int>(i));
    }
    benchmark::DoNotOptimize(map);
  }
}

// 2. FindHit: pre-fill N keys, then find all of them in insertion order
template <typename T, KeyPattern P>
static void BM_FindHit(benchmark::State &state) {
  auto n = state.range(0);
  auto keys = make_keys(P, n);
  T map;
  for (int64_t i = 0; i < n; ++i) {
    map.insert(keys[static_cast<size_t>(i)], static_cast<int>(i));
  }

  for (auto _ : state) {
    for (int64_t i = 0; i < n; ++i) {
      benchmark::DoNotOptimize(map.find(keys[static_cast<size_t>(i)]));
    }
  }
}

// 3. FindMiss: pre-fill N keys, then find N disjoint keys from the same
//    distribution. Miss keys use a prime offset so they land in similar
//    table regions (same clustering) without aliasing with power-of-two
//    table sizes.
template <typename T, KeyPattern P>
static void BM_FindMiss(benchmark::State &state) {
  auto n = state.range(0);
  auto keys = make_keys(P, n);
  T map;
  for (int64_t i = 0; i < n; ++i) {
    map.insert(keys[static_cast<size_t>(i)], static_cast<int>(i));
  }

  auto miss_keys = make_miss_keys(P, keys);

  for (auto _ : state) {
    for (int64_t i = 0; i < n; ++i) {
      benchmark::DoNotOptimize(map.find(miss_keys[static_cast<size_t>(i)]));
    }
  }
}

// 4. EraseAndFind: pre-fill N keys, erase half (every other in insertion
// order),
//    then find the survivors.
template <typename T, KeyPattern P>
static void BM_EraseAndFind(benchmark::State &state) {
  auto n = state.range(0);
  auto keys = make_keys(P, n);

  for (auto _ : state) {
    state.PauseTiming();
    T map;
    for (int64_t i = 0; i < n; ++i) {
      map.insert(keys[static_cast<size_t>(i)], static_cast<int>(i));
    }
    // Erase every other key (by insertion order)
    for (int64_t i = 0; i < n; i += 2) {
      map.erase(keys[static_cast<size_t>(i)]);
    }
    state.ResumeTiming();

    // Find survivors (odd-indexed keys)
    for (int64_t i = 1; i < n; i += 2) {
      benchmark::DoNotOptimize(map.find(keys[static_cast<size_t>(i)]));
    }
  }
}

// 5. EraseChurn: pre-fill N/2 keys, then repeatedly insert one + erase one.
//    Generates a large key pool (20N) so the buffer never wraps during a
//    benchmark run. With N/2 pre-fill and 19.5N churn keys available, even
//    long runs won't exhaust the pool.
template <typename T, KeyPattern P>
static void BM_EraseChurn(benchmark::State &state) {
  auto n = state.range(0);
  auto half = n / 2;

  // 20N keys: first N/2 for pre-fill, rest for churn. At ~1ns/iter,
  // Google Benchmark runs ~100M iterations in 100ms. 19.5N = ~1.3M for
  // N=65536, so the pool won't wrap for sub-microsecond operations.
  constexpr int64_t POOL_MULTIPLIER = 20;
  auto keys = make_keys(P, n * POOL_MULTIPLIER);
  T map;
  for (int64_t i = 0; i < half; ++i) {
    map.insert(keys[static_cast<size_t>(i)], static_cast<int>(i));
  }

  int64_t insert_pos = half;
  int64_t erase_pos = 0;
  for (auto _ : state) {
    map.insert(keys[static_cast<size_t>(insert_pos)],
               static_cast<int>(insert_pos));
    map.erase(keys[static_cast<size_t>(erase_pos)]);
    ++insert_pos;
    ++erase_pos;
  }
}

// --- Registration ---
// 5 scenarios x 3 patterns x 4 implementations at load 0.75, N=65536.

// clang-format off

#define REGISTER_PATTERN(BM, Pattern)                                          \
  BENCHMARK(BM<ChainingHashMap<0.75>, Pattern>)->Range(1 << 8, 1 << 16);      \
  BENCHMARK(BM<LinearProbingHashMap<0.75>, Pattern>)->Range(1 << 8, 1 << 16);  \
  BENCHMARK(BM<RobinHoodHashMap<0.75>, Pattern>)->Range(1 << 8, 1 << 16);     \
  BENCHMARK(BM<StdUnorderedMapAdapter<0.75>, Pattern>)->Range(1 << 8, 1 << 16)

REGISTER_PATTERN(BM_Insert, KeyPattern::SEQUENTIAL);
REGISTER_PATTERN(BM_Insert, KeyPattern::UNIFORM);
REGISTER_PATTERN(BM_Insert, KeyPattern::NORMAL);

REGISTER_PATTERN(BM_FindHit, KeyPattern::SEQUENTIAL);
REGISTER_PATTERN(BM_FindHit, KeyPattern::UNIFORM);
REGISTER_PATTERN(BM_FindHit, KeyPattern::NORMAL);

REGISTER_PATTERN(BM_FindMiss, KeyPattern::SEQUENTIAL);
REGISTER_PATTERN(BM_FindMiss, KeyPattern::UNIFORM);
REGISTER_PATTERN(BM_FindMiss, KeyPattern::NORMAL);

REGISTER_PATTERN(BM_EraseAndFind, KeyPattern::SEQUENTIAL);
REGISTER_PATTERN(BM_EraseAndFind, KeyPattern::UNIFORM);
REGISTER_PATTERN(BM_EraseAndFind, KeyPattern::NORMAL);

REGISTER_PATTERN(BM_EraseChurn, KeyPattern::SEQUENTIAL);
REGISTER_PATTERN(BM_EraseChurn, KeyPattern::UNIFORM);
REGISTER_PATTERN(BM_EraseChurn, KeyPattern::NORMAL);

#undef REGISTER_PATTERN

// clang-format on

BENCHMARK_MAIN();
