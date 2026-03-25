#pragma once

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

enum class KeyPattern { SEQUENTIAL, UNIFORM, NORMAL };

inline std::vector<int> make_sequential_keys(int64_t n) {
  std::vector<int> keys(static_cast<size_t>(n));
  std::iota(keys.begin(), keys.end(), 0);
  return keys;
}

inline std::vector<int> make_uniform_keys(int64_t n) {
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

inline std::vector<int> make_normal_keys(int64_t n) {
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

inline std::vector<int> make_keys(KeyPattern pattern, int64_t n) {
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
// Sequential: miss keys are [N, 2N). With identity hash and a table of size
// 2*next_pow2(N), these map to the empty half — fast misses via immediate
// empty-slot detection.
//
// Uniform: prime offset from each hit key. Scattered occupied slots mean the
// offset reliably lands in empty regions.
//
// Normal: independently generated normal keys centered at N*2 (hit keys center
// at N/2). With identity hash the slot is determined by normal_part alone, and
// the two centers are ~12 stddevs apart, so miss keys hash to a completely
// different region of the table. A simple offset doesn't work here because
// offset mod table_size varies with table size — for some sizes it lands right
// in the hit cluster, for others it doesn't, producing wildly non-monotonic
// benchmark results.
inline std::vector<int> make_miss_keys(KeyPattern pattern,
                                       const std::vector<int> &hit_keys) {
  auto n = static_cast<int64_t>(hit_keys.size());

  if (pattern == KeyPattern::SEQUENTIAL) {
    std::vector<int> miss(hit_keys.size());
    std::iota(miss.begin(), miss.end(), static_cast<int>(n));
    return miss;
  }

  if (pattern == KeyPattern::NORMAL) {
    // Same construction as make_normal_keys but centered at 3N/2 instead of
    // N/2, with a different seed. The SLOT_STRIDE trick ensures normal_part
    // alone determines the slot. With table_size ≈ 2N, the miss center is N
    // slots (50% of the table) from the hit center — on the opposite side.
    // The 95% ranges ([N/4, 3N/4] hits vs [5N/4, 7N/4] misses) don't overlap.
    constexpr int SLOT_STRIDE = 1 << 20;
    constexpr int MAX_X = 2000;
    std::mt19937 rng(777);
    auto mean = static_cast<double>(n) * 1.5;
    auto stddev = static_cast<double>(n) / 8.0;
    std::normal_distribution<double> dist(mean, stddev);
    std::uniform_int_distribution<int> x_dist(0, MAX_X - 1);

    std::unordered_set<int> seen(hit_keys.begin(), hit_keys.end());
    std::vector<int> miss;
    miss.reserve(static_cast<size_t>(n));
    while (static_cast<int64_t>(miss.size()) < n) {
      auto normal_part = static_cast<int>(std::round(dist(rng)));
      int key = normal_part + x_dist(rng) * SLOT_STRIDE;
      if (seen.insert(key).second) {
        miss.push_back(key);
      }
    }
    return miss;
  }

  // Uniform: simple prime offset works fine since occupied slots are scattered.
  constexpr int PRIME_OFFSET = 1000003;
  std::vector<int> miss(hit_keys.size());
  for (size_t i = 0; i < hit_keys.size(); ++i) {
    miss[i] = hit_keys[i] + PRIME_OFFSET;
  }
  return miss;
}

// --- Parameterized benchmark scenarios ---
// Each scenario is templated on Map type and key pattern.

// 1. Insert: insert N keys into an empty map
template <typename T, KeyPattern P> void BM_Insert(benchmark::State &state) {
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
template <typename T, KeyPattern P> void BM_FindHit(benchmark::State &state) {
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
//    distribution.
template <typename T, KeyPattern P> void BM_FindMiss(benchmark::State &state) {
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
//    order), then find the survivors.
template <typename T, KeyPattern P>
void BM_EraseAndFind(benchmark::State &state) {
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
template <typename T, KeyPattern P>
void BM_EraseChurn(benchmark::State &state) {
  auto n = state.range(0);
  auto half = n / 2;

  // 20N keys: first N/2 for pre-fill, rest for churn.
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
