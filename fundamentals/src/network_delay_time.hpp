#pragma once

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

inline int networkDelayTime(std::vector<std::vector<int>> &times, int n,
                            int k) {}
