#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

// Linear Probing Hash Map (int → int)
//
// Implement a hash map using open addressing with linear probing.
//
// Operations:
//   insert(key, value) — insert or update the value for a key
//   find(key)          — return the value if present, std::nullopt otherwise
//   erase(key)         — remove the key if present, return true if removed
//   size()             — number of key-value pairs currently stored
//   empty()            — whether the map has no entries
//
// Behavior:
//   - All entries live in a single flat array of slots
//   - Each slot has three states: empty, occupied, or tombstone (deleted)
//   - On collision, probe linearly: slot, slot+1, slot+2, ... (wrapping)
//   - Erase marks slots as tombstone (not empty) to preserve probe chains
//   - Insert can reuse tombstone slots
//   - Find probes through tombstones but stops at empty slots
//   - Rehash (double slot count, reinsert all occupied entries) when
//     load factor (size / slot_count) exceeds a threshold (e.g., 0.75)
//   - Use std::hash<int> for hashing
//   - Power-of-two slot count, map to slot via: hash(key) & (slot_count - 1)
//
// Constraints:
//   - No use of std::unordered_map, std::map, or other standard associative
//   containers

class LinearProbingHashMap {
public:
  LinearProbingHashMap();
  ~LinearProbingHashMap() = default;

  LinearProbingHashMap(const LinearProbingHashMap &) = delete;
  LinearProbingHashMap &operator=(const LinearProbingHashMap &) = delete;
  LinearProbingHashMap(LinearProbingHashMap &&) = delete;
  LinearProbingHashMap &operator=(LinearProbingHashMap &&) = delete;

  void insert(int key, int value);
  [[nodiscard]] std::optional<int> find(int key) const;
  bool erase(int key);

  [[nodiscard]] size_t size() const;
  [[nodiscard]] bool empty() const;

private:
};
