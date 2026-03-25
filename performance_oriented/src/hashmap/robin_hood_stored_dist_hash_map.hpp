#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <optional>
#include <vector>

// Robin Hood Hash Map — stored probe distance variant (int → int)
//
// Identical algorithm to RobinHoodHashMap, but stores the probe distance in
// the uint8_t state byte (0 = empty, 1+ = probe distance + 1) instead of
// recomputing it via get_home() at each probe step. This exists to measure
// the performance impact of stored vs recomputed probe distance.
//
// See robin_hood_hash_map.hpp for the recomputed version and full
// documentation.

template <double MaxLoad = 0.75> class RobinHoodStoredDistHashMap {

  static constexpr double MAX_LOAD = MaxLoad;

  struct Slot {
    int key;
    int value;
  };

public:
  RobinHoodStoredDistHashMap()
      : stored_probe_distance(num_slots, 0U), slots(num_slots) {}
  ~RobinHoodStoredDistHashMap() = default;

  RobinHoodStoredDistHashMap(const RobinHoodStoredDistHashMap &) = delete;
  RobinHoodStoredDistHashMap &
  operator=(const RobinHoodStoredDistHashMap &) = delete;
  RobinHoodStoredDistHashMap(RobinHoodStoredDistHashMap &&) = delete;
  RobinHoodStoredDistHashMap &operator=(RobinHoodStoredDistHashMap &&) = delete;

  void insert(int key, int value) {
    // TODO: Robin Hood insertion
    // 1. Compute home slot for key
    // 2. Probe forward. At each occupied slot:
    //    a. If it holds our key, update value and return
    //    b. Compute occupant's probe distance vs ours
    //    c. If ours is greater, swap (key, value) with occupant,
    //       continue inserting the displaced element
    // 3. Place into empty slot
    // 4. Increment size, check load factor, grow if needed
    auto home_slot_index = get_home(key);
    auto slot_index = home_slot_index;
    uint8_t inserter_probe_distance = 0;

    while (stored_probe_distance[slot_index] != 0U) {
      auto &occupant_slot = slots[slot_index];
      // Is it occupied by our key?
      if (occupant_slot.key == key) {
        occupant_slot.value = value;
        return;
      }
      // What's its probe_distance?
      uint8_t occupant_probe_distance = stored_probe_distance[slot_index] - 1U;
      if (occupant_probe_distance < inserter_probe_distance) {
        // Replace and reinsert?
        std::swap(key, occupant_slot.key);
        std::swap(value, occupant_slot.value);
        stored_probe_distance[slot_index] = inserter_probe_distance + 1U;
        inserter_probe_distance = occupant_probe_distance;
      }
      // Occupant was poorer or as poor as me
      // Continue to next slot
      slot_index = (slot_index + 1) & (num_slots - 1);
      ++inserter_probe_distance;
      if (inserter_probe_distance == 0) {
        std::abort(); // probe distance overflowed uint8_t
      }
    }
    // Found an empty slot
    stored_probe_distance[slot_index] = inserter_probe_distance + 1U;
    slots[slot_index] = {.key = key, .value = value};
    ++num_entries;

    if (should_resize()) {
      grow();
    }
  }

  [[nodiscard]]
  std::optional<int> find(int key) const {
    // TODO: Find with early termination
    // 1. Compute home slot for key
    // 2. Probe forward. At each occupied slot:
    //    a. If it holds our key, return value
    //    b. If occupant's probe distance < our current probe distance,
    //       key is not in the table (early termination)
    // 3. If empty slot, key is not in the table
    auto slot_index = get_home(key);
    uint8_t finder_probe_distance = 0;

    while (stored_probe_distance[slot_index] != 0U) {
      const auto &occupant = slots[slot_index];
      if (occupant.key == key) {
        return {occupant.value};
      }
      // Check if we're done already
      uint8_t occupant_probe_distance = stored_probe_distance[slot_index] - 1U;
      if (occupant_probe_distance < finder_probe_distance) {
        // We've stumbled into the next section, our key can't be here
        return std::nullopt;
      }

      // On to next slot
      slot_index = (slot_index + 1) & (num_slots - 1);
      ++finder_probe_distance;
      if (finder_probe_distance == 0) {
        std::abort(); // probe distance overflowed uint8_t
      }
    }
    // Found empty slot, didn't find it
    return std::nullopt;
  }

  bool erase(int key) {
    // TODO: Backshift deletion
    // 1. Find the slot holding key (same early termination as find)
    // 2. Mark it empty, then look at the next slot:
    //    a. If empty or at its home position (probe distance 0), stop
    //    b. Otherwise, shift it back into the gap and repeat
    // 3. Decrement size
    auto slot_index = get_home(key);
    uint8_t eraser_probe_distance = 0;

    while (stored_probe_distance[slot_index] != 0U) {
      const auto &occupant = slots[slot_index];
      if (occupant.key == key) {
        break;
      }
      // Check if we're done already
      uint8_t occupant_probe_distance = stored_probe_distance[slot_index] - 1U;
      if (occupant_probe_distance < eraser_probe_distance) {
        // We've stumbled into the next section, our key can't be here
        return false;
      }

      // On to next slot
      slot_index = (slot_index + 1) & (num_slots - 1);
      ++eraser_probe_distance;
      if (eraser_probe_distance == 0) {
        std::abort(); // probe distance overflowed uint8_t
      }
    }
    if (stored_probe_distance[slot_index] == 0U) {
      // Didn't find the slot
      return false;
    }

    // We know that the slot to erase is slot_index now
    stored_probe_distance[slot_index] = 0U;
    auto previous_slot_index = slot_index;
    slot_index = (slot_index + 1) & (num_slots - 1);
    --num_entries;
    while (stored_probe_distance[slot_index] != 0U) {
      auto &occupant = slots[slot_index];
      uint8_t occupant_probe_distance = stored_probe_distance[slot_index] - 1U;
      if (occupant_probe_distance == 0U) {
        return true;
      }
      // Need to shift it back one
      std::swap(occupant, slots[previous_slot_index]);
      std::swap(stored_probe_distance[slot_index],
                stored_probe_distance[previous_slot_index]);

      previous_slot_index = slot_index;
      slot_index = (slot_index + 1) & (num_slots - 1);
    }

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
  size_t probe_distance(size_t home, size_t current) const {
    // How far current is from home, accounting for wraparound
    return (current - home) & (num_slots - 1);
  }

  [[nodiscard]]
  size_t get_home(int key) const {
    return std::hash<int>()(key) & (num_slots - 1);
  }

  [[nodiscard]]
  bool should_resize() const {
    // No tombstones — load factor is simply entries / slots
    auto current_load =
        static_cast<double>(num_entries) / static_cast<double>(num_slots);
    return current_load > MAX_LOAD;
  }

  void grow() {
    // TODO: double num_slots, reinsert all occupied entries
    num_slots *= 2;
    std::vector<uint8_t> new_stored_probe_distance(num_slots, 0U);
    std::vector<Slot> new_slots(num_slots);

    for (size_t slot_index = 0; slot_index < stored_probe_distance.size();
         ++slot_index) {
      if (stored_probe_distance[slot_index] == 0U) {
        continue;
      }
      auto &old_occupant = slots[slot_index];
      // Need to insert into new
      auto new_home_slot_index = get_home(old_occupant.key);
      auto new_slot_index = new_home_slot_index;
      uint8_t inserter_probe_distance = 0;

      while (new_stored_probe_distance[new_slot_index] != 0U) {
        auto &new_occupant = new_slots[new_slot_index];
        // What's its probe_distance?
        uint8_t occupant_probe_distance =
            new_stored_probe_distance[new_slot_index] - 1U;
        if (occupant_probe_distance < inserter_probe_distance) {
          // Replace and reinsert?
          std::swap(old_occupant, new_occupant);
          new_stored_probe_distance[new_slot_index] =
              inserter_probe_distance + 1U;
          inserter_probe_distance = occupant_probe_distance;
        }
        // Occupant was poorer or as poor as me
        // Continue to next slot
        new_slot_index = (new_slot_index + 1) & (num_slots - 1);
        ++inserter_probe_distance;
        if (inserter_probe_distance == 0) {
          std::abort(); // probe distance overflowed uint8_t
        }
      }
      // Found an empty slot
      new_stored_probe_distance[new_slot_index] = inserter_probe_distance + 1U;
      new_slots[new_slot_index] = old_occupant;
    }

    stored_probe_distance = std::move(new_stored_probe_distance);
    slots = std::move(new_slots);
  }

  size_t num_entries = 0;
  size_t num_slots = 16;
  std::vector<uint8_t> stored_probe_distance;
  std::vector<Slot> slots;
};
