#ifdef TRACK_MEMORY
#include "alloc_counter.hpp"
#endif

#include "chaining_hash_map.hpp"
#include "chaining_hash_map_v2.hpp"
#include "linear_probing_hash_map.hpp"
#include "robin_hood_hash_map.hpp"
#include "robin_hood_hash_map_v2.hpp"
#include "std_unordered_map_adapter.hpp"

#include <benchmark/benchmark.h>
#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <random>
#include <vector>

// When compiled with -DTRACK_MEMORY, reports heap bytes via custom counters.
// The alloc_counter overrides global new/delete, adding a small per-allocation
// overhead (atomic increment). Use hash_map_mem_bench for memory numbers and
// hash_map_bench for clean timing numbers.
#ifdef TRACK_MEMORY
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define MEMORY_RESET() alloc_counter::reset()
#define MEMORY_REPORT(st, entries)                                             \
  do {                                                                         \
    auto _bytes = alloc_counter::current();                                    \
    (st).counters["bytes"] = static_cast<double>(_bytes);                      \
    (st).counters["bytes_per_entry"] =                                         \
        static_cast<double>(_bytes) / static_cast<double>(entries);            \
  } while (0)
// NOLINTEND(cppcoreguidelines-macro-usage)
#else
#define MEMORY_RESET() (void)0
#define MEMORY_REPORT(st, entries) (void)0
#endif

// --- Scenarios ---

// 1. Insert: insert N sequential keys into an empty map
template <typename T> static void BM_Insert(benchmark::State &state) {
  auto n = state.range(0);
  for (auto _ : state) {
    MEMORY_RESET();
    T map;
    for (int64_t i = 0; i < n; ++i) {
      map.insert(static_cast<int>(i), static_cast<int>(i));
    }
    MEMORY_REPORT(state, n);
    benchmark::DoNotOptimize(map);
  }
}

// 2. Find hit: pre-fill N keys, then find all of them
// Memory: reported once before the timed loop (steady-state after fill)
template <typename T> static void BM_FindHit(benchmark::State &state) {
  auto n = state.range(0);
  MEMORY_RESET();
  T map;
  for (int64_t i = 0; i < n; ++i) {
    map.insert(static_cast<int>(i), static_cast<int>(i));
  }
  MEMORY_REPORT(state, n);

  for (auto _ : state) {
    for (int64_t i = 0; i < n; ++i) {
      benchmark::DoNotOptimize(map.find(static_cast<int>(i)));
    }
  }
}

// 3. Find miss: pre-fill N keys [0, N), then find N keys that don't exist
// Memory: same as find hit (steady-state after fill)
template <typename T> static void BM_FindMiss(benchmark::State &state) {
  auto n = state.range(0);
  MEMORY_RESET();
  T map;
  for (int64_t i = 0; i < n; ++i) {
    map.insert(static_cast<int>(i), static_cast<int>(i));
  }
  MEMORY_REPORT(state, n);

  for (auto _ : state) {
    for (int64_t i = n; i < 2 * n; ++i) {
      benchmark::DoNotOptimize(map.find(static_cast<int>(i)));
    }
  }
}

// 4. Erase + find survivors: pre-fill N keys, erase even keys, find odd keys
// Memory: after insert + erase (N/2 survivors, but table may retain full size)
template <typename T> static void BM_EraseAndFind(benchmark::State &state) {
  auto n = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();
    MEMORY_RESET();
    T map;
    for (int64_t i = 0; i < n; ++i) {
      map.insert(static_cast<int>(i), static_cast<int>(i));
    }
    for (int64_t i = 0; i < n; i += 2) {
      map.erase(static_cast<int>(i));
    }
    MEMORY_REPORT(state, n / 2);
    state.ResumeTiming();

    for (int64_t i = 1; i < n; i += 2) {
      benchmark::DoNotOptimize(map.find(static_cast<int>(i)));
    }
  }
}

// 5. Erase-heavy churn: pre-fill N/2 keys, then repeatedly insert+erase
//    Measures combined insert+erase throughput under churn
// Memory: steady-state after pre-fill (churn maintains ~N/2 entries)
template <typename T> static void BM_EraseChurn(benchmark::State &state) {
  auto n = state.range(0);
  auto half = n / 2;

  MEMORY_RESET();
  T map;
  for (int64_t i = 0; i < half; ++i) {
    map.insert(static_cast<int>(i), static_cast<int>(i));
  }
  MEMORY_REPORT(state, half);

  int next_key = static_cast<int>(half);
  for (auto _ : state) {
    // Insert a new key, erase an old one
    map.insert(next_key, next_key);
    map.erase(next_key - static_cast<int>(half));
    ++next_key;
  }
}

// --- Key generation helpers ---
// Fixed seed for reproducibility across runs.

static std::vector<int> make_random_keys(int64_t n) {
  // Shuffle [0, 10N) and take the first N. Guarantees uniqueness without
  // needing a set, and the large range means keys are sparse/scattered.
  std::vector<int> keys(static_cast<size_t>(n * 10));
  std::iota(keys.begin(), keys.end(), 0);
  std::mt19937 rng(42);
  std::shuffle(keys.begin(), keys.end(), rng);
  keys.resize(static_cast<size_t>(n));
  return keys;
}

static std::vector<int> make_strided_keys(int64_t n, int stride) {
  std::vector<int> keys;
  keys.reserve(static_cast<size_t>(n));
  for (int64_t i = 0; i < n; ++i) {
    keys.push_back(static_cast<int>(i) * stride);
  }
  return keys;
}

// --- Random key scenarios ---
// Tests hash quality and cache behavior with non-sequential access.
// Identity hash maps random keys to scattered slots — fine for distribution,
// but random access patterns break the sequential-scan cache advantage that
// helps open addressing with sequential keys.
//
// Note: keys are read from a pre-built vector, adding one L1-hot sequential
// read per operation vs the counter in the original benchmarks. This cost is
// identical across all implementations so relative comparisons are valid.

template <typename T> static void BM_Insert_Random(benchmark::State &state) {
  auto n = state.range(0);
  auto keys = make_random_keys(n);
  for (auto _ : state) {
    T map;
    for (int64_t i = 0; i < n; ++i) {
      map.insert(keys[static_cast<size_t>(i)], static_cast<int>(i));
    }
    benchmark::DoNotOptimize(map);
  }
}

template <typename T> static void BM_FindHit_Random(benchmark::State &state) {
  auto n = state.range(0);
  auto keys = make_random_keys(n);
  T map;
  for (int64_t i = 0; i < n; ++i) {
    map.insert(keys[static_cast<size_t>(i)], static_cast<int>(i));
  }

  // Shuffle lookup order so we're not finding in insertion order
  auto lookup_keys = keys;
  std::mt19937 rng(123);
  std::shuffle(lookup_keys.begin(), lookup_keys.end(), rng);

  for (auto _ : state) {
    for (int64_t i = 0; i < n; ++i) {
      benchmark::DoNotOptimize(map.find(lookup_keys[static_cast<size_t>(i)]));
    }
  }
}

// --- Strided key scenarios ---
// Pathological for identity hash with power-of-two tables. Keys that are
// multiples of 16 all map to slot 0 (since hash(k) & (16-1) = 0 when k is
// a multiple of 16). This creates massive clustering for open addressing
// and long chains for chaining. A good hash function would scatter these.
//
// Note: stride of 16 was chosen to match common initial table sizes. As the
// table grows (32, 64, ...) the collision pattern shifts but clustering
// remains severe — e.g. with 32 slots, all keys land on slots 0 or 16.

template <typename T> static void BM_Insert_Strided(benchmark::State &state) {
  auto n = state.range(0);
  auto keys = make_strided_keys(n, 16);
  for (auto _ : state) {
    T map;
    for (int64_t i = 0; i < n; ++i) {
      map.insert(keys[static_cast<size_t>(i)], static_cast<int>(i));
    }
    benchmark::DoNotOptimize(map);
  }
}

template <typename T> static void BM_FindHit_Strided(benchmark::State &state) {
  auto n = state.range(0);
  auto keys = make_strided_keys(n, 16);
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

// --- Large scale scenarios ---
// Extends to 1<<22 (~4M entries) to push tables well past L2 cache.
// At ~9 bytes/slot with 0.75 load, 1<<22 entries needs ~5.6M slots ≈ 50MB.
// On Apple Silicon, P-core L2 is 16MB — so 1<<20 (~12.6MB) barely fits
// while 1<<22 is fully cache-cold. This is where prefetching would help —
// the next probe position is predictable but almost certainly a cache miss.
//
// Three variants: sequential keys (best-case cache pattern), random keys
// (realistic), and random lookups (worst-case — random key + random order).

template <typename T> static void BM_Insert_Large(benchmark::State &state) {
  auto n = state.range(0);
  for (auto _ : state) {
    T map;
    for (int64_t i = 0; i < n; ++i) {
      map.insert(static_cast<int>(i), static_cast<int>(i));
    }
    benchmark::DoNotOptimize(map);
  }
}

template <typename T> static void BM_FindHit_Large(benchmark::State &state) {
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

template <typename T>
static void BM_FindHit_Random_Large(benchmark::State &state) {
  auto n = state.range(0);
  auto keys = make_random_keys(n);
  T map;
  for (int64_t i = 0; i < n; ++i) {
    map.insert(keys[static_cast<size_t>(i)], static_cast<int>(i));
  }

  auto lookup_keys = keys;
  std::mt19937 rng(123);
  std::shuffle(lookup_keys.begin(), lookup_keys.end(), rng);

  for (auto _ : state) {
    for (int64_t i = 0; i < n; ++i) {
      benchmark::DoNotOptimize(map.find(lookup_keys[static_cast<size_t>(i)]));
    }
  }
}

// --- Registration ---

// clang-format off

// Original scenarios: 5 scenarios x 6 impls x 3 load factors
#define REGISTER_ALL(BM, Load)                                                 \
  BENCHMARK(BM<ChainingHashMap<Load>>)->Range(1 << 8, 1 << 16);               \
  BENCHMARK(BM<ChainingHashMapV2<Load>>)->Range(1 << 8, 1 << 16);             \
  BENCHMARK(BM<LinearProbingHashMap<Load>>)->Range(1 << 8, 1 << 16);          \
  /* BENCHMARK(BM<LinearProbingHashMap<Load, true>>)->Range(1 << 8, 1 << 16); */ \
  /* BENCHMARK(BM<LinearProbingHashMap<Load, false, true>>)->Range(1 << 8, 1 << 16); */ \
  BENCHMARK(BM<RobinHoodHashMap<Load>>)->Range(1 << 8, 1 << 16);             \
  BENCHMARK(BM<RobinHoodHashMap<Load, true>>)->Range(1 << 8, 1 << 16);      \
  BENCHMARK(BM<RobinHoodHashMapV2<Load>>)->Range(1 << 8, 1 << 16);           \
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

#undef REGISTER_ALL

// Hash quality scenarios: random + strided keys at load 0.75 only.
// Load factor is not the variable here — key distribution is.
#define REGISTER_HASH(BM)                                                      \
  BENCHMARK(BM<ChainingHashMap<0.75>>)->Range(1 << 8, 1 << 16);               \
  BENCHMARK(BM<ChainingHashMapV2<0.75>>)->Range(1 << 8, 1 << 16);             \
  BENCHMARK(BM<LinearProbingHashMap<0.75>>)->Range(1 << 8, 1 << 16);          \
  /* BENCHMARK(BM<LinearProbingHashMap<0.75, true>>)->Range(1 << 8, 1 << 16); */ \
  /* BENCHMARK(BM<LinearProbingHashMap<0.75, false, true>>)->Range(1 << 8, 1 << 16); */ \
  BENCHMARK(BM<RobinHoodHashMap<0.75>>)->Range(1 << 8, 1 << 16);             \
  BENCHMARK(BM<RobinHoodHashMap<0.75, true>>)->Range(1 << 8, 1 << 16);      \
  BENCHMARK(BM<RobinHoodHashMapV2<0.75>>)->Range(1 << 8, 1 << 16);           \
  BENCHMARK(BM<StdUnorderedMapAdapter<0.75>>)->Range(1 << 8, 1 << 16)

REGISTER_HASH(BM_Insert_Random);
REGISTER_HASH(BM_FindHit_Random);
REGISTER_HASH(BM_Insert_Strided);
REGISTER_HASH(BM_FindHit_Strided);

#undef REGISTER_HASH

// Large scale scenarios: 1<<16 to 1<<22, load 0.75 only.
// Targets cache boundary behavior and prefetch opportunities.
// On Apple Silicon (P-core L2 = 16MB), 1<<20 (~12.6MB) barely fits;
// 1<<22 (~50MB) is fully cache-cold.
#define REGISTER_LARGE(BM)                                                     \
  BENCHMARK(BM<ChainingHashMap<0.75>>)->Range(1 << 16, 1 << 22);              \
  BENCHMARK(BM<ChainingHashMapV2<0.75>>)->Range(1 << 16, 1 << 22);            \
  BENCHMARK(BM<LinearProbingHashMap<0.75>>)->Range(1 << 16, 1 << 22);         \
  /* BENCHMARK(BM<LinearProbingHashMap<0.75, true>>)->Range(1 << 16, 1 << 22); */ \
  /* BENCHMARK(BM<LinearProbingHashMap<0.75, false, true>>)->Range(1 << 16, 1 << 22); */ \
  BENCHMARK(BM<RobinHoodHashMap<0.75>>)->Range(1 << 16, 1 << 22);            \
  BENCHMARK(BM<RobinHoodHashMap<0.75, true>>)->Range(1 << 16, 1 << 22);     \
  BENCHMARK(BM<RobinHoodHashMapV2<0.75>>)->Range(1 << 16, 1 << 22);          \
  BENCHMARK(BM<StdUnorderedMapAdapter<0.75>>)->Range(1 << 16, 1 << 22)

REGISTER_LARGE(BM_Insert_Large);
REGISTER_LARGE(BM_FindHit_Large);
REGISTER_LARGE(BM_FindHit_Random_Large);

#undef REGISTER_LARGE

// clang-format on

BENCHMARK_MAIN();
