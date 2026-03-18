#pragma once

#include <unordered_map>
#include <vector>

// Course Schedule (LeetCode 207)
//
// There are a total of numCourses courses you have to take, labeled from 0 to
// numCourses - 1. You are given an array prerequisites where
// prerequisites[i] = [a_i, b_i] indicates that you must take course b_i first
// if you want to take course a_i.
//
// For example, the pair [0, 1] indicates that to take course 0 you have to
// first take course 1.
//
// Return true if you can finish all courses. Otherwise, return false.
//
// Example 1:
//   numCourses = 2, prerequisites = [[1,0]]
//   Output: true
//   (Take course 0 then course 1.)
//
// Example 2:
//   numCourses = 2, prerequisites = [[1,0],[0,1]]
//   Output: false
//   (Courses 0 and 1 are prerequisites of each other — impossible.)
//
// Constraints:
//   - 1 <= numCourses <= 2000
//   - 0 <= prerequisites.length <= 5000
//   - prerequisites[i].length == 2
//   - 0 <= a_i, b_i < numCourses
//   - All the pairs prerequisites[i] are unique

inline bool canFinish(int numCourses,
                      std::vector<std::vector<int>> &prerequisites) {

  std::unordered_map<int, std::vector<int>> node_to_children;

  enum StackState : uint8_t { EXPLORE, FINISH_EXPLORING };

  // Have a list of edges with numCourses nodes
  for (const auto &prerequisite : prerequisites) {
    const auto intro_course = prerequisite[0];
    const auto advanced_course = prerequisite[1];

    // An advanced course depends on taking an intro first
    node_to_children[intro_course].push_back(advanced_course);
  }

  // Do DFS keeping track of the current path and the fully explored values
  std::vector<bool> fully_explored(static_cast<size_t>(numCourses), false);

  for (int course_index = 0; course_index < numCourses; ++course_index) {
    if (fully_explored[static_cast<size_t>(course_index)]) {
      continue;
    }
    // Do DFS
    std::vector<bool> currently_searching(fully_explored.size(), false);
    std::vector<std::pair<StackState, int>> current_path{
        {StackState::EXPLORE, course_index}};
    while (!current_path.empty()) {
      const auto [state, current_node] = std::move(current_path.back());
      current_path.pop_back();

      switch (state) {
      case StackState::EXPLORE: {
        if (fully_explored[static_cast<size_t>(current_node)]) {
          // We know all the children are good
          continue;
        }
        if (currently_searching[static_cast<size_t>(current_node)]) {
          // Already seen this node on this path, cycle detected
          return false;
        }
        current_path.emplace_back(StackState::FINISH_EXPLORING, current_node);
        currently_searching[static_cast<size_t>(current_node)] = true;
        bool children_to_explore = false;
        // We need to search the children of this node
        for (const auto child : node_to_children[current_node]) {
          if (fully_explored[static_cast<size_t>(child)]) {
            // Don't need to search it again
            continue;
          }
          children_to_explore = true;
          current_path.emplace_back(StackState::EXPLORE, child);
        }
        if (!children_to_explore) {
          fully_explored[static_cast<size_t>(current_node)] = true;
        }
        break;
      }
      case StackState::FINISH_EXPLORING: {
        currently_searching[static_cast<size_t>(current_node)] = false;
        fully_explored[static_cast<size_t>(current_node)] = true;
        break;
      }
      }
    }
  }

  return true;
}

// Course Schedule II (LeetCode 210)
//
// There are a total of numCourses courses you have to take, labeled from 0 to
// numCourses - 1. You are given an array prerequisites where
// prerequisites[i] = [a_i, b_i] indicates that you must take course b_i first
// if you want to take course a_i.
//
// Return the ordering of courses you should take to finish all courses. If
// there are many valid answers, return any of them. If it is impossible to
// finish all courses, return an empty array.
//
// Example 1:
//   numCourses = 2, prerequisites = [[1,0]]
//   Output: [0,1]
//
// Example 2:
//   numCourses = 4, prerequisites = [[1,0],[2,0],[3,1],[3,2]]
//   Output: [0,1,2,3] or [0,2,1,3]
//
// Example 3:
//   numCourses = 1, prerequisites = []
//   Output: [0]
//
// Constraints:
//   - 1 <= numCourses <= 2000
//   - 0 <= prerequisites.length <= numCourses * (numCourses - 1)
//   - prerequisites[i].length == 2
//   - 0 <= a_i, b_i < numCourses
//   - a_i != b_i
//   - All the pairs [a_i, b_i] are distinct

inline std::vector<int>
findOrder(int numCourses, std::vector<std::vector<int>> &prerequisites) {
  return {};
}
