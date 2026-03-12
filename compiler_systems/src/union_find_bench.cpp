#include "union_find.hpp"

#include <benchmark/benchmark.h>

// Adversarial case: build a chain where each unite attaches the
// accumulated tree under a new single node, creating maximum depth.
// Then repeatedly find the deepest node.
static void BM_FindAfterAdversarialChain(benchmark::State &state) {
  int n = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();
    UnionFind uf(n);
    // Build worst-case chain: unite(1,0), unite(2,1), unite(3,2), ...
    // Each time the larger tree gets reparented under a fresh node.
    for (int i = 1; i < n; ++i) {
      uf.unite(i, i - 1);
    }
    state.ResumeTiming();

    // Find on the node that's deepest in the chain
    benchmark::DoNotOptimize(uf.find(0));
  }
}
BENCHMARK(BM_FindAfterAdversarialChain)->Range(1 << 10, 1 << 20);

// Measure repeated finds after chain construction.
// Path compression means the first find is expensive but subsequent
// ones should be nearly free.
static void BM_RepeatedFindAfterChain(benchmark::State &state) {
  int n = state.range(0);
  UnionFind uf(n);
  for (int i = 1; i < n; ++i) {
    uf.unite(i, i - 1);
  }

  // First find compresses the path
  uf.find(0);

  for (auto _ : state) {
    benchmark::DoNotOptimize(uf.find(0));
  }
}
BENCHMARK(BM_RepeatedFindAfterChain)->Range(1 << 10, 1 << 20);

// Random unites then random finds — the "typical" workload.
static void BM_RandomUnitesAndFinds(benchmark::State &state) {
  int n = state.range(0);

  for (auto _ : state) {
    UnionFind uf(n);
    for (int i = 0; i < n; ++i) {
      uf.unite(i % 97, i % 53);
    }
    for (int i = 0; i < n; ++i) {
      benchmark::DoNotOptimize(uf.find(i % n));
    }
  }
}
BENCHMARK(BM_RandomUnitesAndFinds)->Range(1 << 10, 1 << 20);

BENCHMARK_MAIN();
