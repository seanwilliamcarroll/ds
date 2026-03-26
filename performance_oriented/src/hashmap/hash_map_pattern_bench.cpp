#include "hash_map_bench_helpers.hpp"

#include "chaining_hash_map.hpp"
#include "linear_probing_hash_map.hpp"
#include "robin_hood_hash_map.hpp"
#include "std_unordered_map_adapter.hpp"

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
