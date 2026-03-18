#include "network_delay_time.hpp"

#include <benchmark/benchmark.h>
#include <random>
#include <vector>

// Dense graph: all n*(n-1) directed edges with random weights [1, 100].
// At n=100 this is ~10k edges — stresses re-enqueue behaviour in SPFA.
static std::vector<std::vector<int>> makeDenseGraph(int n, std::mt19937 &rng) {
  std::uniform_int_distribution<int> weight(1, 100);
  std::vector<std::vector<int>> edges;
  edges.reserve(static_cast<size_t>(n * (n - 1)));
  for (int u = 1; u <= n; ++u) {
    for (int v = 1; v <= n; ++v) {
      if (u != v) {
        edges.push_back({u, v, weight(rng)});
      }
    }
  }
  return edges;
}

// Sparse graph: a simple chain 1->2->...->n with random weights [1, 100].
// n-1 edges total. Dijkstra settles each node exactly once; SPFA must walk
// the full chain before the optimal path is known.
static std::vector<std::vector<int>> makeSparseGraph(int n, std::mt19937 &rng) {
  std::uniform_int_distribution<int> weight(1, 100);
  std::vector<std::vector<int>> edges;
  edges.reserve(static_cast<size_t>(n - 1));
  for (int i = 1; i < n; ++i) {
    edges.push_back({i, i + 1, weight(rng)});
  }
  return edges;
}

// ── Dense benchmarks
// ──────────────────────────────────────────────────────────

static void BM_SPFA_Dense(benchmark::State &state) {
  int n = static_cast<int>(state.range(0));
  std::mt19937 rng(42);
  for (auto _ : state) {
    state.PauseTiming();
    auto edges = makeDenseGraph(n, rng);
    state.ResumeTiming();
    benchmark::DoNotOptimize(networkDelayTimeSPFA(edges, n, 1));
  }
}

static void BM_Dijkstra_Dense(benchmark::State &state) {
  int n = static_cast<int>(state.range(0));
  std::mt19937 rng(42);
  for (auto _ : state) {
    state.PauseTiming();
    auto edges = makeDenseGraph(n, rng);
    state.ResumeTiming();
    benchmark::DoNotOptimize(networkDelayTimeDijkstra(edges, n, 1));
  }
}

BENCHMARK(BM_SPFA_Dense)->Arg(10)->Arg(25)->Arg(50)->Arg(100);
BENCHMARK(BM_Dijkstra_Dense)->Arg(10)->Arg(25)->Arg(50)->Arg(100);

// ── Sparse benchmarks
// ─────────────────────────────────────────────────────────

static void BM_SPFA_Sparse(benchmark::State &state) {
  int n = static_cast<int>(state.range(0));
  std::mt19937 rng(42);
  for (auto _ : state) {
    state.PauseTiming();
    auto edges = makeSparseGraph(n, rng);
    state.ResumeTiming();
    benchmark::DoNotOptimize(networkDelayTimeSPFA(edges, n, 1));
  }
}

static void BM_Dijkstra_Sparse(benchmark::State &state) {
  int n = static_cast<int>(state.range(0));
  std::mt19937 rng(42);
  for (auto _ : state) {
    state.PauseTiming();
    auto edges = makeSparseGraph(n, rng);
    state.ResumeTiming();
    benchmark::DoNotOptimize(networkDelayTimeDijkstra(edges, n, 1));
  }
}

BENCHMARK(BM_SPFA_Sparse)->Arg(10)->Arg(50)->Arg(100)->Arg(500)->Arg(1000);
BENCHMARK(BM_Dijkstra_Sparse)->Arg(10)->Arg(50)->Arg(100)->Arg(500)->Arg(1000);

BENCHMARK_MAIN();
