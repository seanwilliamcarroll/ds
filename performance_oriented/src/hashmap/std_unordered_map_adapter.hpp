#pragma once

#include <cstddef>
#include <optional>
#include <unordered_map>

// Thin wrapper around std::unordered_map<int, int> to match the same
// insert/find/erase/size/empty interface as the custom implementations.

template <double MaxLoad = 0.75> class StdUnorderedMapAdapter {

public:
  StdUnorderedMapAdapter() {
    map_.max_load_factor(static_cast<float>(MaxLoad));
  }

  void insert(int key, int value) { map_.insert_or_assign(key, value); }

  [[nodiscard]]
  std::optional<int> find(int key) const {
    auto it = map_.find(key);
    if (it == map_.end()) {
      return std::nullopt;
    }
    return {it->second};
  }

  bool erase(int key) { return map_.erase(key) > 0; }

  [[nodiscard]]
  size_t size() const {
    return map_.size();
  }

  [[nodiscard]]
  bool empty() const {
    return map_.empty();
  }

private:
  std::unordered_map<int, int> map_;
};
