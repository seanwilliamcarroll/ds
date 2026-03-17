#pragma once

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
                      std::vector<std::vector<int>> &prerequisites) {}

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
findOrder(int numCourses, std::vector<std::vector<int>> &prerequisites) {}
