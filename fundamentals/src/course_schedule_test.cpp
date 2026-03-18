#include "course_schedule.hpp"

#include <algorithm>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

// ── canFinish ────────────────────────────────────────────────────────────────

TEST(CourseSchedule, CanFinish_NoCycles) {
  // 0 -> 1
  std::vector<std::vector<int>> prereqs = {{1, 0}};
  EXPECT_TRUE(canFinish(2, prereqs));
}

TEST(CourseSchedule, CanFinish_SimpleCycle) {
  // 0 -> 1 -> 0
  std::vector<std::vector<int>> prereqs = {{1, 0}, {0, 1}};
  EXPECT_FALSE(canFinish(2, prereqs));
}

TEST(CourseSchedule, CanFinish_NoPrerequisites) {
  std::vector<std::vector<int>> prereqs = {};
  EXPECT_TRUE(canFinish(5, prereqs));
}

TEST(CourseSchedule, CanFinish_SingleCourse) {
  std::vector<std::vector<int>> prereqs = {};
  EXPECT_TRUE(canFinish(1, prereqs));
}

TEST(CourseSchedule, CanFinish_LinearChain) {
  // 0 -> 1 -> 2 -> 3
  std::vector<std::vector<int>> prereqs = {{1, 0}, {2, 1}, {3, 2}};
  EXPECT_TRUE(canFinish(4, prereqs));
}

TEST(CourseSchedule, CanFinish_CycleInLargerGraph) {
  // 0 -> 1 -> 2 -> 1 (cycle between 1 and 2)
  std::vector<std::vector<int>> prereqs = {{1, 0}, {2, 1}, {1, 2}};
  EXPECT_FALSE(canFinish(3, prereqs));
}

TEST(CourseSchedule, CanFinish_DiamondNoCycle) {
  //     0
  //    / \
  //   1   2
  //    \ /
  //     3
  std::vector<std::vector<int>> prereqs = {{1, 0}, {2, 0}, {3, 1}, {3, 2}};
  EXPECT_TRUE(canFinish(4, prereqs));
}

TEST(CourseSchedule, CanFinish_DisconnectedNoCycle) {
  // Two independent chains: 0->1, 2->3
  std::vector<std::vector<int>> prereqs = {{1, 0}, {3, 2}};
  EXPECT_TRUE(canFinish(4, prereqs));
}

TEST(CourseSchedule, CanFinish_ConvergingPaths) {
  // Two paths both leading to node 1: 0->1, 0->2->1 (no cycle)
  std::vector<std::vector<int>> prereqs = {{0, 1}, {0, 2}, {2, 1}};
  EXPECT_TRUE(canFinish(3, prereqs));
}

TEST(CourseSchedule, CanFinish_ConvergingPathsWithChild) {
  // Same as above but node 1 has a child (3): 0->1->3, 0->2->1 (no cycle)
  std::vector<std::vector<int>> prereqs = {{0, 1}, {0, 2}, {2, 1}, {1, 3}};
  EXPECT_TRUE(canFinish(4, prereqs));
}

// ── findOrder ────────────────────────────────────────────────────────────────

// Verify that the returned order is a valid topological sort:
// - contains every course exactly once
// - all prerequisites appear before the course that requires them
static void verifyOrder(int numCourses,
                        const std::vector<std::vector<int>> &prereqs,
                        const std::vector<int> &order) {
  ASSERT_EQ(static_cast<int>(order.size()), numCourses);

  // Every course present exactly once
  std::vector<int> sorted = order;
  std::ranges::sort(sorted);
  for (int i = 0; i < numCourses; ++i) {
    EXPECT_EQ(sorted[static_cast<size_t>(i)], i);
  }

  // Build position map
  std::unordered_map<int, int> pos;
  for (int i = 0; i < static_cast<int>(order.size()); ++i) {
    pos[order[static_cast<size_t>(i)]] = i;
  }

  // Each prerequisite appears before the course that needs it
  for (const auto &p : prereqs) {
    int course = p[0];
    int prereq = p[1];
    EXPECT_LT(pos[prereq], pos[course])
        << "prereq " << prereq << " should come before course " << course;
  }
}

TEST(CourseSchedule, FindOrder_NoCourses) {
  std::vector<std::vector<int>> prereqs = {};
  auto order = findOrder(0, prereqs);
  EXPECT_TRUE(order.empty());
}

TEST(CourseSchedule, FindOrder_SingleCourse) {
  std::vector<std::vector<int>> prereqs = {};
  auto order = findOrder(1, prereqs);
  ASSERT_EQ(order.size(), 1U);
  EXPECT_EQ(order[0], 0);
}

TEST(CourseSchedule, FindOrder_LeetCodeExample1) {
  // [1,0] -> take 0 before 1
  std::vector<std::vector<int>> prereqs = {{1, 0}};
  auto order = findOrder(2, prereqs);
  verifyOrder(2, prereqs, order);
}

TEST(CourseSchedule, FindOrder_LeetCodeExample2) {
  // [[1,0],[2,0],[3,1],[3,2]]
  std::vector<std::vector<int>> prereqs = {{1, 0}, {2, 0}, {3, 1}, {3, 2}};
  auto order = findOrder(4, prereqs);
  verifyOrder(4, prereqs, order);
}

TEST(CourseSchedule, FindOrder_HasCycle) {
  std::vector<std::vector<int>> prereqs = {{1, 0}, {0, 1}};
  auto order = findOrder(2, prereqs);
  EXPECT_TRUE(order.empty());
}

TEST(CourseSchedule, FindOrder_NoPrerequisites) {
  std::vector<std::vector<int>> prereqs = {};
  auto order = findOrder(4, prereqs);
  verifyOrder(4, prereqs, order);
}

TEST(CourseSchedule, FindOrder_LinearChain) {
  // 3 requires 2 requires 1 requires 0
  std::vector<std::vector<int>> prereqs = {{1, 0}, {2, 1}, {3, 2}};
  auto order = findOrder(4, prereqs);
  verifyOrder(4, prereqs, order);
}

TEST(CourseSchedule, FindOrder_CycleInLargerGraph) {
  std::vector<std::vector<int>> prereqs = {{1, 0}, {2, 1}, {1, 2}};
  auto order = findOrder(3, prereqs);
  EXPECT_TRUE(order.empty());
}
