// Dumps generated keys and their slot mappings to CSV for visualization.
//
// Usage: ./key_distribution_dump > keys.csv
//        python scripts/plot_key_distribution.py keys.csv

#include "hash_map_bench_helpers.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string_view>

// Compute table size the same way our hash maps do:
// start at 16, double until load <= 0.75.
size_t table_size_for(int64_t n) {
  size_t slots = 16;
  while (static_cast<double>(n) / static_cast<double>(slots) > 0.75) {
    slots *= 2;
  }
  return slots;
}

size_t slot_for(int key, size_t table_size) {
  return std::hash<int>()(key) & (table_size - 1);
}

void dump_keys(std::string_view pattern_name, KeyPattern pattern, int64_t n) {
  auto hit_keys = make_keys(pattern, n);
  auto miss_keys = make_miss_keys(pattern, hit_keys);
  auto table_sz = table_size_for(n);

  for (const auto &key : hit_keys) {
    std::cout << pattern_name << "," << n << "," << table_sz << ",hit," << key
              << "," << slot_for(key, table_sz) << "\n";
  }
  for (const auto &key : miss_keys) {
    std::cout << pattern_name << "," << n << "," << table_sz << ",miss," << key
              << "," << slot_for(key, table_sz) << "\n";
  }
}

int main() {
  std::cout << "pattern,n,table_size,type,key,slot\n";

  constexpr std::array<int64_t, 5> NS = {256, 512, 4096, 32768, 65536};

  for (auto n : NS) {
    dump_keys("sequential", KeyPattern::SEQUENTIAL, n);
    dump_keys("uniform", KeyPattern::UNIFORM, n);
    dump_keys("normal", KeyPattern::NORMAL, n);
  }
}
