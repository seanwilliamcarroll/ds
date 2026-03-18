#include "network_delay_time.hpp"

#include <vector>

#include <gtest/gtest.h>

TEST(NetworkDelayTime, LeetCodeExample1) {
  // Signal from 2: reaches 1 and 3 at t=1, then 4 at t=2
  std::vector<std::vector<int>> times = {{2, 1, 1}, {2, 3, 1}, {3, 4, 1}};
  EXPECT_EQ(networkDelayTime(times, 4, 2), 2);
}

TEST(NetworkDelayTime, LeetCodeExample2) {
  std::vector<std::vector<int>> times = {{1, 2, 1}};
  EXPECT_EQ(networkDelayTime(times, 2, 1), 1);
}

TEST(NetworkDelayTime, LeetCodeExample3_Unreachable) {
  // Node 1 unreachable from node 2
  std::vector<std::vector<int>> times = {{1, 2, 1}};
  EXPECT_EQ(networkDelayTime(times, 2, 2), -1);
}

TEST(NetworkDelayTime, SingleNode) {
  std::vector<std::vector<int>> times = {};
  EXPECT_EQ(networkDelayTime(times, 1, 1), 0);
}

TEST(NetworkDelayTime, DirectPathVsRelayPath) {
  // Direct path 1->3 costs 10; relay 1->2->3 costs 3
  // Answer is max arrival time = 3 (both nodes reachable)
  std::vector<std::vector<int>> times = {{1, 2, 1}, {2, 3, 2}, {1, 3, 10}};
  EXPECT_EQ(networkDelayTime(times, 3, 1), 3);
}

TEST(NetworkDelayTime, ShortestPathNotFastest) {
  // 1->3 direct (weight 100) vs 1->2->3 (weight 1+1=2)
  // Dijkstra must pick cheapest, not fewest hops
  std::vector<std::vector<int>> times = {{1, 2, 1}, {2, 3, 1}, {1, 3, 100}};
  EXPECT_EQ(networkDelayTime(times, 3, 1), 2);
}

TEST(NetworkDelayTime, LinearChain) {
  // 1->2->3->4->5, each edge weight 1
  std::vector<std::vector<int>> times = {
      {1, 2, 1}, {2, 3, 1}, {3, 4, 1}, {4, 5, 1}};
  EXPECT_EQ(networkDelayTime(times, 5, 1), 4);
}

TEST(NetworkDelayTime, StarTopology) {
  // Hub node 1 sends to all others with varying weights
  std::vector<std::vector<int>> times = {
      {1, 2, 1}, {1, 3, 5}, {1, 4, 2}, {1, 5, 3}};
  EXPECT_EQ(networkDelayTime(times, 5, 1), 5);
}

TEST(NetworkDelayTime, PartiallyConnected) {
  // Node 4 has no incoming edges from source's component
  std::vector<std::vector<int>> times = {{1, 2, 1}, {2, 3, 1}};
  EXPECT_EQ(networkDelayTime(times, 4, 1), -1);
}

TEST(NetworkDelayTime, ZeroWeightEdges) {
  std::vector<std::vector<int>> times = {{1, 2, 0}, {2, 3, 0}, {3, 4, 0}};
  EXPECT_EQ(networkDelayTime(times, 4, 1), 0);
}

TEST(NetworkDelayTime, SourceIsLastToReceive) {
  // All paths are long except the last node
  // 1->2 (weight 10), 1->3 (weight 1), 3->2 (weight 1)
  // Node 2 arrival: min(10, 1+1) = 2; answer = 2
  std::vector<std::vector<int>> times = {{1, 2, 10}, {1, 3, 1}, {3, 2, 1}};
  EXPECT_EQ(networkDelayTime(times, 3, 1), 2);
}
