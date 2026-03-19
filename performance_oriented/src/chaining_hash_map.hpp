#pragma once

#include <cstddef>
#include <functional>
#include <optional>

// Chaining Hash Map (int → int)
//
// Implement a hash map using separate chaining (linked list per bucket).
//
// Operations:
//   insert(key, value) — insert or update the value for a key
//   find(key)          — return the value if present, std::nullopt otherwise
//   erase(key)         — remove the key if present, return true if removed
//   size()             — number of key-value pairs currently stored
//   empty()            — whether the map has no entries
//
// Behavior:
//   - Start with a small number of buckets (e.g., 8)
//   - Rehash (double bucket count, redistribute all entries) when
//     load factor (size / bucket_count) exceeds a threshold (e.g., 0.75)
//   - Use std::hash<int> for hashing
//   - Map to a bucket via: hash(key) % bucket_count
//     (power-of-two sizing with bitmasking is a future optimization)
//
// Constraints:
//   - No use of std::unordered_map, std::map, or other standard associative
//   containers
//   - You must manage your own linked list nodes (heap-allocated)

class ChainingHashMap {
public:
  ChainingHashMap();
  ~ChainingHashMap();

  ChainingHashMap(const ChainingHashMap &) = delete;
  ChainingHashMap &operator=(const ChainingHashMap &) = delete;
  ChainingHashMap(ChainingHashMap &&) = delete;
  ChainingHashMap &operator=(ChainingHashMap &&) = delete;

  void insert(int key, int value);
  std::optional<int> find(int key) const;
  bool erase(int key);

  size_t size() const;
  bool empty() const;

private:
};
