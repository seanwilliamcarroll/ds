#include "union_find.hpp"

#include <benchmark/benchmark.h>

// Four variants to compare
using UF_None = UnionFind<false, false>;
using UF_RankOnly = UnionFind<true, false>;
using UF_CompressionOnly = UnionFind<false, true>;
using UF_Both = UnionFind<true, true>;

// Adversarial case: build a chain that creates maximum depth,
// then find the deepest node.
template <typename UF>
static void BM_FindAfterAdversarialChain(benchmark::State &state) {
  int n = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();
    UF uf(n);
    for (int i = 1; i < n; ++i) {
      uf.unite(i, i - 1);
    }
    state.ResumeTiming();

    benchmark::DoNotOptimize(uf.find(0));
  }
}

BENCHMARK(BM_FindAfterAdversarialChain<UF_None>)->Range(1 << 10, 1 << 18);
BENCHMARK(BM_FindAfterAdversarialChain<UF_RankOnly>)->Range(1 << 10, 1 << 18);
BENCHMARK(BM_FindAfterAdversarialChain<UF_CompressionOnly>)
    ->Range(1 << 10, 1 << 18);
BENCHMARK(BM_FindAfterAdversarialChain<UF_Both>)->Range(1 << 10, 1 << 18);

// Repeated finds after chain construction.
// Shows the effect of path compression on subsequent lookups.
template <typename UF>
static void BM_RepeatedFindAfterChain(benchmark::State &state) {
  int n = state.range(0);
  UF uf(n);
  for (int i = 1; i < n; ++i) {
    uf.unite(i, i - 1);
  }

  // First find triggers compression (if enabled)
  uf.find(0);

  for (auto _ : state) {
    benchmark::DoNotOptimize(uf.find(0));
  }
}

BENCHMARK(BM_RepeatedFindAfterChain<UF_None>)->Range(1 << 10, 1 << 18);
BENCHMARK(BM_RepeatedFindAfterChain<UF_RankOnly>)->Range(1 << 10, 1 << 18);
BENCHMARK(BM_RepeatedFindAfterChain<UF_CompressionOnly>)
    ->Range(1 << 10, 1 << 18);
BENCHMARK(BM_RepeatedFindAfterChain<UF_Both>)->Range(1 << 10, 1 << 18);

// Random unites then random finds — typical workload.
template <typename UF>
static void BM_RandomUnitesAndFinds(benchmark::State &state) {
  int n = state.range(0);

  for (auto _ : state) {
    UF uf(n);
    for (int i = 0; i < n; ++i) {
      uf.unite(i % 97, i % 53);
    }
    for (int i = 0; i < n; ++i) {
      benchmark::DoNotOptimize(uf.find(i % n));
    }
  }
}

BENCHMARK(BM_RandomUnitesAndFinds<UF_None>)->Range(1 << 10, 1 << 18);
BENCHMARK(BM_RandomUnitesAndFinds<UF_RankOnly>)->Range(1 << 10, 1 << 18);
BENCHMARK(BM_RandomUnitesAndFinds<UF_CompressionOnly>)->Range(1 << 10, 1 << 18);
BENCHMARK(BM_RandomUnitesAndFinds<UF_Both>)->Range(1 << 10, 1 << 18);

BENCHMARK_MAIN();
