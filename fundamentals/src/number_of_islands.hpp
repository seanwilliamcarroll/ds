#pragma once

#include <cstddef>
#include <vector>

// Number of Islands (LeetCode 200)
//
// Given an m x n 2D binary grid which represents a map of '1's (land) and
// '0's (water), return the number of islands.
//
// An island is surrounded by water and is formed by connecting adjacent land
// cells horizontally or vertically. You may assume all four edges of the grid
// are surrounded by water.
//
// Example 1:
//   grid = {{'1','1','1','1','0'},
//           {'1','1','0','1','0'},
//           {'1','1','0','0','0'},
//           {'0','0','0','0','0'}}
//   Answer: 1
//
// Example 2:
//   grid = {{'1','1','0','0','0'},
//           {'1','1','0','0','0'},
//           {'0','0','1','0','0'},
//           {'0','0','0','1','1'}}
//   Answer: 3
//
// Constraints:
//   - m == grid.size()
//   - n == grid[i].size()
//   - 1 <= m, n <= 300
//   - grid[i][j] is '0' or '1'

inline int numIslands(std::vector<std::vector<char>> &grid) {

  constexpr char LAND = '1';
  constexpr char WATER = '0';

  int num_islands = 0;

  for (size_t row_index = 0; row_index < grid.size(); ++row_index) {
    for (size_t col_index = 0; col_index < grid[row_index].size();
         ++col_index) {

      if (grid[row_index][col_index] != LAND) {
        continue;
      }
      ++num_islands;
      grid[row_index][col_index] = WATER;

      // Find adjacent land tiles and fill them in

      using Position = std::pair<size_t, size_t>;

      std::vector<Position> possible_land_tiles;

      auto add_possible_tiles = [&possible_land_tiles,
                                 &grid](const size_t possible_row_index,
                                        const size_t possible_col_index) {
        if (possible_row_index > 0 &&
            grid[possible_row_index - 1][possible_col_index] == LAND) {
          possible_land_tiles.emplace_back(possible_row_index - 1,
                                           possible_col_index);
        }
        if (possible_col_index > 0 &&
            grid[possible_row_index][possible_col_index - 1] == LAND) {
          possible_land_tiles.emplace_back(possible_row_index,
                                           possible_col_index - 1);
        }
        if (possible_row_index < grid.size() - 1 &&
            grid[possible_row_index + 1][possible_col_index] == LAND) {
          possible_land_tiles.emplace_back(possible_row_index + 1,
                                           possible_col_index);
        }
        if (possible_col_index < grid[possible_row_index].size() - 1 &&
            grid[possible_row_index][possible_col_index + 1] == LAND) {
          possible_land_tiles.emplace_back(possible_row_index,
                                           possible_col_index + 1);
        }
      };

      add_possible_tiles(row_index, col_index);

      while (!possible_land_tiles.empty()) {
        const auto [island_row_index, island_col_index] =
            possible_land_tiles.back();
        possible_land_tiles.pop_back();

        if (grid[island_row_index][island_col_index] != LAND) {
          continue;
        }
        grid[island_row_index][island_col_index] = WATER;
        add_possible_tiles(island_row_index, island_col_index);
      }
    }
  }

  return num_islands;
}
