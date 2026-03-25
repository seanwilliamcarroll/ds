#include "hash_map_bench_helpers.hpp"

#include "chaining_hash_map.hpp"
#include "chaining_pool_hash_map.hpp"
#include "linear_probing_hash_map.hpp"
#include "linear_probing_hash_map_merged.hpp"
#include "robin_hood_hash_map.hpp"
#include "robin_hood_stored_dist_hash_map.hpp"

// A/B optimization pair benchmarks for blog post 2.
//
// Each pair compares a baseline implementation against a single optimization.
// Thin inheriting structs give each side a unique type name in Google
// Benchmark output. This is necessary because `using` aliases are transparent
// to the compiler — two aliases for the same type produce identical benchmark
// names, making pairs indistinguishable in the output. Inheriting structs
// create distinct types with readable names.
//
// Filter by pair with --benchmark_filter:
//   "LinearProbingHashMap_"  → AoS or prefetch pairs
//   "Merged"                 → AoS merge pair only
//   "Prefetch"               → prefetching pair only
//   "StoredDist"             → stored probe distance pair only
//   "ChainingHashMap_"       → pool allocator pair only
//   "RobinHoodHashMap_Bool"  → vector<bool> vs uint8_t pair only

// --- Pair 1: AoS merge (SoA layout vs merged struct) ---
struct LinearProbingHashMap_SoA : LinearProbingHashMap<0.75> {};
struct LinearProbingHashMap_MergedStruct
    : LinearProbingMergedStructHashMap<0.75> {};

// --- Pair 2: Stored probe distance (recomputed vs stored) ---
struct RobinHoodHashMap_RecomputedDist : RobinHoodHashMap<0.75> {};
struct RobinHoodHashMap_StoredDist : RobinHoodStoredDistHashMap<0.75> {};

// --- Pair 3: Prefetching (off vs on) ---
struct LinearProbingHashMap_NoPrefetch
    : LinearProbingHashMap<0.75, false, false> {};
struct LinearProbingHashMap_Prefetch : LinearProbingHashMap<0.75, false, true> {
};

// --- Pair 4: vector<bool> vs uint8_t state ---
struct RobinHoodHashMap_BoolState : RobinHoodHashMap<0.75, true> {};
struct RobinHoodHashMap_Uint8State : RobinHoodHashMap<0.75, false> {};

// --- Pair 5: Pool allocator (per-node unique_ptr vs pool) ---
struct ChainingHashMap_UniquePtr : ChainingHashMap<0.75> {};
struct ChainingHashMap_PoolAllocator : ChainingPoolHashMap<0.75> {};

// --- Registration ---
// 4 scenarios x 3 patterns x 2 sides per pair, N up to 65536.

// clang-format off

#define REGISTER_PAIR(A, B)                                                    \
  BENCHMARK(BM_Insert<A, KeyPattern::SEQUENTIAL>)->Range(1 << 8, 1 << 16);    \
  BENCHMARK(BM_Insert<B, KeyPattern::SEQUENTIAL>)->Range(1 << 8, 1 << 16);    \
  BENCHMARK(BM_Insert<A, KeyPattern::UNIFORM>)->Range(1 << 8, 1 << 16);       \
  BENCHMARK(BM_Insert<B, KeyPattern::UNIFORM>)->Range(1 << 8, 1 << 16);       \
  BENCHMARK(BM_Insert<A, KeyPattern::NORMAL>)->Range(1 << 8, 1 << 16);        \
  BENCHMARK(BM_Insert<B, KeyPattern::NORMAL>)->Range(1 << 8, 1 << 16);        \
  BENCHMARK(BM_FindHit<A, KeyPattern::SEQUENTIAL>)->Range(1 << 8, 1 << 16);   \
  BENCHMARK(BM_FindHit<B, KeyPattern::SEQUENTIAL>)->Range(1 << 8, 1 << 16);   \
  BENCHMARK(BM_FindHit<A, KeyPattern::UNIFORM>)->Range(1 << 8, 1 << 16);      \
  BENCHMARK(BM_FindHit<B, KeyPattern::UNIFORM>)->Range(1 << 8, 1 << 16);      \
  BENCHMARK(BM_FindHit<A, KeyPattern::NORMAL>)->Range(1 << 8, 1 << 16);       \
  BENCHMARK(BM_FindHit<B, KeyPattern::NORMAL>)->Range(1 << 8, 1 << 16);       \
  BENCHMARK(BM_FindMiss<A, KeyPattern::SEQUENTIAL>)->Range(1 << 8, 1 << 16);  \
  BENCHMARK(BM_FindMiss<B, KeyPattern::SEQUENTIAL>)->Range(1 << 8, 1 << 16);  \
  BENCHMARK(BM_FindMiss<A, KeyPattern::UNIFORM>)->Range(1 << 8, 1 << 16);     \
  BENCHMARK(BM_FindMiss<B, KeyPattern::UNIFORM>)->Range(1 << 8, 1 << 16);     \
  BENCHMARK(BM_FindMiss<A, KeyPattern::NORMAL>)->Range(1 << 8, 1 << 16);      \
  BENCHMARK(BM_FindMiss<B, KeyPattern::NORMAL>)->Range(1 << 8, 1 << 16);      \
  BENCHMARK(BM_EraseAndFind<A, KeyPattern::SEQUENTIAL>)->Range(1 << 8, 1 << 16); \
  BENCHMARK(BM_EraseAndFind<B, KeyPattern::SEQUENTIAL>)->Range(1 << 8, 1 << 16); \
  BENCHMARK(BM_EraseAndFind<A, KeyPattern::UNIFORM>)->Range(1 << 8, 1 << 16); \
  BENCHMARK(BM_EraseAndFind<B, KeyPattern::UNIFORM>)->Range(1 << 8, 1 << 16); \
  BENCHMARK(BM_EraseAndFind<A, KeyPattern::NORMAL>)->Range(1 << 8, 1 << 16);  \
  BENCHMARK(BM_EraseAndFind<B, KeyPattern::NORMAL>)->Range(1 << 8, 1 << 16)

REGISTER_PAIR(LinearProbingHashMap_SoA, LinearProbingHashMap_MergedStruct);
REGISTER_PAIR(RobinHoodHashMap_RecomputedDist, RobinHoodHashMap_StoredDist);
REGISTER_PAIR(LinearProbingHashMap_NoPrefetch, LinearProbingHashMap_Prefetch);
REGISTER_PAIR(RobinHoodHashMap_BoolState, RobinHoodHashMap_Uint8State);
REGISTER_PAIR(ChainingHashMap_UniquePtr, ChainingHashMap_PoolAllocator);

#undef REGISTER_PAIR

// Large-scale AoS comparison: 64K to 4M entries.
// The AoS regression grows with scale as the inflated working set thrashes L2.
#define REGISTER_LARGE(BM, Pattern)                                            \
  BENCHMARK(BM<LinearProbingHashMap_SoA, Pattern>)->Range(1 << 16, 1 << 22);  \
  BENCHMARK(BM<LinearProbingHashMap_MergedStruct, Pattern>)->Range(1 << 16, 1 << 22)

REGISTER_LARGE(BM_Insert, KeyPattern::SEQUENTIAL);
REGISTER_LARGE(BM_FindHit, KeyPattern::SEQUENTIAL);
REGISTER_LARGE(BM_FindHit, KeyPattern::UNIFORM);

#undef REGISTER_LARGE

// clang-format on

BENCHMARK_MAIN();
