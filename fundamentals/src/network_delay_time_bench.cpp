#include "network_delay_time.hpp"

#include <algorithm>
#include <benchmark/benchmark.h>
#include <random>
#include <vector>

// Builds a graph with exactly m directed edges on n nodes, guaranteed
// reachable from node 1.
//
// Strategy:
//   1. Start with the chain 1->2->...->n (n-1 edges) so node 1 reaches all.
//   2. Collect all remaining possible directed edges (n*(n-1) - (n-1) total).
//   3. Shuffle them and take the first (m - (n-1)) to hit the target.
//
// density_pct [0..100] controls how many extra edges are added on top of
// the chain, as a percentage of the (n-1)^2 extra edges available:
//   extra = density_pct% * (n-1)^2
//   m     = (n-1) + extra
static std::vector<std::vector<int>> makeGraph(int n, int density_pct,
                                               std::mt19937 &rng) {
  std::uniform_int_distribution<int> weight(1, 100);

  // Chain edges (guaranteed connectivity)
  std::vector<std::vector<int>> edges;
  edges.reserve(static_cast<size_t>(n - 1));
  for (int i = 1; i < n; ++i) {
    edges.push_back({i, i + 1, weight(rng)});
  }

  if (density_pct == 0) {
    return edges;
  }

  // All possible non-chain directed edges
  std::vector<std::pair<int, int>> extra_candidates;
  extra_candidates.reserve(static_cast<size_t>((n - 1) * (n - 1)));
  for (int u = 1; u <= n; ++u) {
    for (int v = 1; v <= n; ++v) {
      if (u != v && v != u + 1) { // skip chain edges already added
        extra_candidates.emplace_back(u, v);
      }
    }
  }

  std::shuffle(extra_candidates.begin(), extra_candidates.end(), rng);

  const auto max_extra = static_cast<int>(extra_candidates.size());
  const int n_extra = density_pct * max_extra / 100;
  for (int i = 0; i < n_extra; ++i) {
    auto [u, v] = extra_candidates[static_cast<size_t>(i)];
    edges.push_back({u, v, weight(rng)});
  }

  return edges;
}

// ── SPFA benchmark
// ────────────────────────────────────────────────────────────

static void BM_SPFA(benchmark::State &state) {
  int n = static_cast<int>(state.range(0));
  int density_pct = static_cast<int>(state.range(1));
  std::mt19937 rng(42);
  for (auto _ : state) {
    state.PauseTiming();
    auto edges = makeGraph(n, density_pct, rng);
    state.ResumeTiming();
    benchmark::DoNotOptimize(network_delay_time_spfa(edges, n, 1));
  }
}

// ── Dijkstra benchmark
// ────────────────────────────────────────────────────────

static void BM_Dijkstra(benchmark::State &state) {
  int n = static_cast<int>(state.range(0));
  int density_pct = static_cast<int>(state.range(1));
  std::mt19937 rng(42);
  for (auto _ : state) {
    state.PauseTiming();
    auto edges = makeGraph(n, density_pct, rng);
    state.ResumeTiming();
    benchmark::DoNotOptimize(network_delay_time_dijkstra(edges, n, 1));
  }
}

// Cartesian product:
//   n           ∈ {10, 25, 50, 100, 200, 500}
//   density_pct ∈ {0, 5, 10, 15, 20, 25, 50, 75, 100}
BENCHMARK(BM_SPFA)->ArgsProduct({
    {10, 25, 50, 100, 200, 500},
    {0, 5, 10, 15, 20, 25, 50, 75, 100},
});
BENCHMARK(BM_Dijkstra)
    ->ArgsProduct({
        {10, 25, 50, 100, 200, 500},
        {0, 5, 10, 15, 20, 25, 50, 75, 100},
    });

BENCHMARK_MAIN();
