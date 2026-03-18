#pragma once

#include "min_heap.hpp"
#include <algorithm>
#include <deque>
#include <functional>
#include <unordered_map>
#include <vector>

// Network Delay Time (LeetCode 743)
//
// You are given a network of n nodes, labeled from 1 to n. You are also given
// times, a list of travel times as directed edges where
// times[i] = (u_i, v_i, w_i) and u_i is the source node, v_i is the target
// node, and w_i is the time it takes for a signal to travel from source to
// target.
//
// We will send a signal from a given node k. Return the minimum time it takes
// for all n nodes to receive the signal. If it is impossible for all n nodes
// to receive the signal, return -1.
//
// Example 1:
//   times = [[2,1,1],[2,3,1],[3,4,1]], n = 4, k = 2
//   Output: 2
//   (Signal reaches 1 and 3 at t=1, then 4 at t=2)
//
// Example 2:
//   times = [[1,2,1]], n = 2, k = 1
//   Output: 1
//
// Example 3:
//   times = [[1,2,1]], n = 2, k = 2
//   Output: -1
//   (Node 1 is unreachable from node 2)
//
// Constraints:
//   - 1 <= k <= n <= 100
//   - 1 <= times.length <= 6000
//   - times[i].length == 3
//   - 1 <= u_i, v_i <= n
//   - u_i != v_i
//   - 0 <= w_i <= 100
//   - All the pairs (u_i, v_i) are unique

namespace {
struct Edge {
  int source;
  int dest;

  auto operator<=>(const Edge &) const = default;
};
} // namespace

namespace std {
template <> struct hash<Edge> {
  size_t operator()(const Edge &edge) const {
    // n guarenteed to be under 256, combine this way
    const int combined = (edge.source << 8) | edge.dest;
    return std::hash<int>()(combined);
  }
};
} // namespace std

// SPFA (Shortest Path Faster Algorithm): BFS with re-enqueue on improvement.
// Correct but may visit nodes multiple times. O(V * E) worst case.
inline int networkDelayTimeSPFA(std::vector<std::vector<int>> &times, int n,
                                int k) {

  // We should build an adjacency list, keeping the weights in mind
  // Should keep a vector of minimum time it takes to reach that node starting
  // at k
  // Initialize to -1, except for k (should be 0)
  // BFS the nodes, keeping the minimum it takes to reach node x from y,
  // starting from k

  std::unordered_map<int, std::vector<int>> node_to_neighbors;
  std::unordered_map<Edge, int> weights;

  for (const auto &edge_time : times) {
    const auto source_node = edge_time[0];
    const auto dest_node = edge_time[1];
    const auto weight = edge_time[2];
    node_to_neighbors[source_node].push_back(dest_node);
    weights[{source_node, dest_node}] = weight;
  }

  std::vector<int> minimum_time_from_k(static_cast<size_t>(n), -1);
  const auto k_index = static_cast<size_t>(k - 1);
  minimum_time_from_k[k_index] = 0;

  struct SearchState {
    int next_node;
    int prev_node;
    int prev_node_time;
  };

  std::deque<SearchState> neighbor_queue;

  for (const auto neighbor : node_to_neighbors[k]) {
    neighbor_queue.push_back(
        {.next_node = neighbor, .prev_node = k, .prev_node_time = 0});
  }

  while (!neighbor_queue.empty()) {
    const auto state = neighbor_queue.front();
    neighbor_queue.pop_front();
    const auto next_node_index = static_cast<size_t>(state.next_node - 1);
    const auto new_possible_time =
        state.prev_node_time + weights.at({state.prev_node, state.next_node});
    if (minimum_time_from_k[next_node_index] == -1 ||
        minimum_time_from_k[next_node_index] > new_possible_time) {
      minimum_time_from_k[next_node_index] = new_possible_time;
      // Continue BFS from here
      for (const auto neighbor : node_to_neighbors[state.next_node]) {
        neighbor_queue.push_back({.next_node = neighbor,
                                  .prev_node = state.next_node,
                                  .prev_node_time = new_possible_time});
      }
    }
  }

  const auto min_time = *std::ranges::min_element(minimum_time_from_k.begin(),
                                                  minimum_time_from_k.end());
  if (min_time == -1) {
    return -1;
  }

  return *std::ranges::max_element(minimum_time_from_k.begin(),
                                   minimum_time_from_k.end());
}

// Dijkstra: always expand the globally cheapest unvisited node using a
// min-heap. Each node settled exactly once. O((V + E) log V).
inline int networkDelayTimeDijkstra(std::vector<std::vector<int>> &times, int n,
                                    int k) {

  std::unordered_map<int, std::vector<std::pair<int, int>>> node_to_neighbors;

  for (const auto &edge_time : times) {
    const auto source_node = edge_time[0];
    const auto dest_node = edge_time[1];
    const auto weight = edge_time[2];
    node_to_neighbors[source_node].emplace_back(dest_node, weight);
  }

  std::vector<int> minimum_time_from_k(static_cast<size_t>(n), -1);
  const auto k_index = static_cast<size_t>(k - 1);
  minimum_time_from_k[k_index] = 0;

  // Now want to use min_heap to keep track of globally shortest path we've
  // explored so far
  MinHeap<std::pair<int, int>> search_frontier;
  for (const auto &[next_node, weight] : node_to_neighbors[k]) {
    search_frontier.push({weight, next_node});
    minimum_time_from_k[next_node - 1] = weight;
  }

  while (!search_frontier.empty()) {
    auto [time_so_far, node] = search_frontier.pop();
    for (const auto &[next_node, additional_time] : node_to_neighbors[node]) {
      auto new_possible_time = time_so_far + additional_time;
      if (minimum_time_from_k[next_node - 1] == -1 ||
          minimum_time_from_k[next_node - 1] > new_possible_time) {
        minimum_time_from_k[next_node - 1] = new_possible_time;
        search_frontier.push({new_possible_time, next_node});
      }
    }
  }

  const auto min_time = *std::ranges::min_element(minimum_time_from_k.begin(),
                                                  minimum_time_from_k.end());
  if (min_time == -1) {
    return -1;
  }

  return *std::ranges::max_element(minimum_time_from_k.begin(),
                                   minimum_time_from_k.end());
}
