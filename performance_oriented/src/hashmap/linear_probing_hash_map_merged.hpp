#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

// Linear Probing Hash Map — AoS (merged struct) layout variant (int → int)
//
// Identical algorithm to LinearProbingHashMap, but with a merged struct per
// slot (AoS) instead of separate state and key-value arrays (SoA). Each slot
// is { key, value, state } — 12 bytes with padding. This exists to measure
// the performance impact of AoS vs SoA layout on linear probing.
//
// See linear_probing_hash_map.hpp for the SoA version and full documentation.

template <double MaxLoad = 0.75> class LinearProbingMergedStructHashMap {

  static constexpr double MAX_LOAD = MaxLoad;

  enum class SlotState : uint8_t {
    EMPTY,
    TOMBSTONE,
    OCCUPIED,
  };

  struct Slot {
    int key{};
    int value{};
    SlotState state{};
  };

public:
  LinearProbingMergedStructHashMap() : slots(num_slots) {}
  ~LinearProbingMergedStructHashMap() = default;

  LinearProbingMergedStructHashMap(const LinearProbingMergedStructHashMap &) =
      delete;
  LinearProbingMergedStructHashMap &
  operator=(const LinearProbingMergedStructHashMap &) = delete;
  LinearProbingMergedStructHashMap(LinearProbingMergedStructHashMap &&) =
      delete;
  LinearProbingMergedStructHashMap &
  operator=(LinearProbingMergedStructHashMap &&) = delete;

  void insert(int key, int value) {
    auto bucket_index = get_bucket_index(key);
    auto first_tombstone = num_slots;

    while (!is_empty(bucket_index)) {
      auto next_bucket_index = (bucket_index + 1) & (num_slots - 1);
      if (is_tombstone(bucket_index)) {
        if (first_tombstone == num_slots) {
          first_tombstone = bucket_index;
        }
      } else if (slots[bucket_index].key == key) {
        slots[bucket_index].value = value;
        return;
      }
      bucket_index = next_bucket_index;
    }
    if (first_tombstone < num_slots) {
      bucket_index = first_tombstone;
      --num_tombstones;
    }
    // State should be empty
    slots[bucket_index] = {
        .key = key, .value = value, .state = SlotState::OCCUPIED};
    ++num_entries;

    if (should_resize()) {
      grow();
    }
  }

  [[nodiscard]]
  std::optional<int> find(int key) const {
    auto bucket_index = get_bucket_index(key);
    auto original_bucket_index = bucket_index;

    while (is_occupied_but_no_match(bucket_index, key) ||
           is_tombstone(bucket_index)) {
      bucket_index = (bucket_index + 1) & (num_slots - 1);
      if (bucket_index == original_bucket_index) {
        return std::nullopt;
      }
    }
    if (is_empty(bucket_index)) {
      return std::nullopt;
    }
    return {slots[bucket_index].value};
  }

  bool erase(int key) {
    auto bucket_index = get_bucket_index(key);

    while (is_occupied_but_no_match(bucket_index, key) ||
           is_tombstone(bucket_index)) {
      bucket_index = (bucket_index + 1) & (num_slots - 1);
    }
    if (is_empty(bucket_index)) {
      return false;
    }
    --num_entries;
    slots[bucket_index].state = SlotState::TOMBSTONE;
    ++num_tombstones;
    return true;
  }

  [[nodiscard]]
  size_t size() const {
    return num_entries;
  }

  [[nodiscard]]
  bool empty() const {
    return size() == 0;
  }

private:
  [[nodiscard]]
  bool is_occupied(size_t index) const {
    return slots[index].state == SlotState::OCCUPIED;
  }

  [[nodiscard]]
  bool is_occupied_but_no_match(size_t index, int key) const {
    return is_occupied(index) && slots[index].key != key;
  }

  [[nodiscard]]
  bool is_empty(size_t index) const {
    return slots[index].state == SlotState::EMPTY;
  }

  [[nodiscard]]
  bool is_tombstone(size_t index) const {
    return slots[index].state == SlotState::TOMBSTONE;
  }

  [[nodiscard]]
  bool should_resize() const {
    // Tombstones count toward load because insert probes until it finds an
    // empty slot — if occupied + tombstones fills the table, the probe loops
    // forever even though actual entry count is low.
    //
    // Downside: heavy erase churn can trigger a grow when few real entries
    // exist (e.g. 12 tombstones + 1 entry → resize to 32 slots with 1/32
    // occupancy). The production fix is two thresholds: grow when
    // num_entries / num_slots exceeds a limit, rehash in-place (same
    // capacity, drop tombstones) when (num_entries + num_tombstones) /
    // num_slots exceeds a higher limit. Abseil's flat_hash_map does this.
    auto current_load = static_cast<double>(num_entries + num_tombstones) /
                        static_cast<double>(num_slots);
    return current_load > MAX_LOAD;
  }

  void grow() {
    num_slots *= 2;

    std::vector<Slot> new_slots(num_slots);

    for (size_t old_bucket_index = 0; old_bucket_index < slots.size();
         ++old_bucket_index) {
      if (slots[old_bucket_index].state != SlotState::OCCUPIED) {
        continue;
      }
      auto new_bucket_index = get_bucket_index(slots[old_bucket_index].key);
      while (new_slots[new_bucket_index].state == SlotState::OCCUPIED) {
        new_bucket_index = (new_bucket_index + 1) & (num_slots - 1);
      }
      new_slots[new_bucket_index] = slots[old_bucket_index];
    }
    slots = std::move(new_slots);
    num_tombstones = 0;
  }

  [[nodiscard]]
  size_t get_hash(int key) const {
    return std::hash<int>()(key);
  }

  [[nodiscard]]
  size_t get_bucket_index(int key) const {
    return get_hash(key) & (num_slots - 1);
  }

  size_t num_entries = 0;
  size_t num_tombstones = 0;
  size_t num_slots = 16;
  std::vector<Slot> slots;
};
