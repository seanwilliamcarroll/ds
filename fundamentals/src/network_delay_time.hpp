#pragma once

#include "min_heap.hpp"
#include <algorithm>
#include <deque>
#include <functional>
#include <limits>
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

  std::unordered_map<int, std::vector<std::pair<int, int>>> node_to_neighbors;

  for (const auto &edge_time : times) {
    const auto source_node = edge_time[0];
    const auto dest_node = edge_time[1];
    const auto weight = edge_time[2];
    node_to_neighbors[source_node].emplace_back(dest_node, weight);
  }

  std::vector<int> minimum_time_from_k(static_cast<size_t>(n),
                                       std::numeric_limits<int>::max());

  struct SearchState {
    int next_node;
    int cost_to_reach;
  };

  std::deque<SearchState> neighbor_queue{{.next_node = k, .cost_to_reach = 0}};

  while (!neighbor_queue.empty()) {
    const auto state = neighbor_queue.front();
    neighbor_queue.pop_front();
    const auto next_node_index = static_cast<size_t>(state.next_node - 1);
    if (minimum_time_from_k[next_node_index] > state.cost_to_reach) {
      minimum_time_from_k[next_node_index] = state.cost_to_reach;
      // Continue BFS from here
      for (const auto &[neighbor, weight] :
           node_to_neighbors[state.next_node]) {
        neighbor_queue.push_back(
            {.next_node = neighbor,
             .cost_to_reach = state.cost_to_reach + weight});
      }
    }
  }

  auto max_element = *std::ranges::max_element(minimum_time_from_k.begin(),
                                               minimum_time_from_k.end());
  if (max_element == std::numeric_limits<int>::max()) {
    return -1;
  }
  return max_element;
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

  std::vector<int> minimum_time_from_k(static_cast<size_t>(n),
                                       std::numeric_limits<int>::max());
  const auto k_index = static_cast<size_t>(k - 1);
  minimum_time_from_k[k_index] = 0;

  // Now want to use min_heap to keep track of globally shortest path we've
  // explored so far
  MinHeap<std::pair<int, int>> search_frontier;
  search_frontier.push({0, k});

  while (!search_frontier.empty()) {
    auto [time_so_far, node] = search_frontier.pop();
    auto node_index = static_cast<size_t>(node - 1);
    if (time_so_far > minimum_time_from_k[node_index]) {
      // This path is no good, skip it
      continue;
    }
    for (const auto &[next_node, additional_time] : node_to_neighbors[node]) {
      auto new_possible_time = time_so_far + additional_time;
      auto next_node_index = static_cast<size_t>(next_node - 1);
      if (minimum_time_from_k[next_node_index] > new_possible_time) {
        minimum_time_from_k[next_node_index] = new_possible_time;
        search_frontier.push({new_possible_time, next_node});
      }
    }
  }

  auto max_time = *std::ranges::max_element(minimum_time_from_k.begin(),
                                            minimum_time_from_k.end());
  if (max_time == std::numeric_limits<int>::max()) {
    return -1;
  }
  return max_time;
}
