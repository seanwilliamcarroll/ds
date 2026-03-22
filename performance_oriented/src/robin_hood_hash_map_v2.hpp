#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

// Robin Hood Hash Map (int → int)
//
// Open addressing with Robin Hood hashing and backshift deletion.
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
//   - Each slot is either empty or occupied (no tombstones!)
//   - On collision, probe linearly: slot, slot+1, slot+2, ... (wrapping)
//   - Robin Hood insertion: if the inserting element has probed farther than
//     the occupant, swap them and continue inserting the displaced element
//   - Early termination on find: if occupant's probe distance < our current
//     probe distance, the key cannot be in the table
//   - Backshift deletion: shift displaced elements backward to fill the gap,
//     stop when hitting an empty slot or an element at its home position
//   - Rehash (double slot count, reinsert all occupied entries) when
//     load factor (size / slot_count) exceeds a threshold (e.g., 0.75)
//   - Use std::hash<int> for hashing
//   - Power-of-two slot count, map to slot via: hash(key) & (slot_count - 1)
//
// Constraints:
//   - No use of std::unordered_map, std::map, or other standard associative
//   containers

template <double MaxLoad = 0.75> class RobinHoodHashMapV2 {

  static constexpr double MAX_LOAD = MaxLoad;

  struct Slot {
    int key;
    int value;
  };

public:
  RobinHoodHashMapV2() : occupied(num_slots), slots(num_slots) {}
  ~RobinHoodHashMapV2() = default;

  RobinHoodHashMapV2(const RobinHoodHashMapV2 &) = delete;
  RobinHoodHashMapV2 &operator=(const RobinHoodHashMapV2 &) = delete;
  RobinHoodHashMapV2(RobinHoodHashMapV2 &&) = delete;
  RobinHoodHashMapV2 &operator=(RobinHoodHashMapV2 &&) = delete;

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
    size_t inserter_probe_distance = 0;

    while (occupied[slot_index]) {
      auto &occupant_slot = slots[slot_index];
      // Is it occupied by our key?
      if (occupant_slot.key == key) {
        occupant_slot.value = value;
        return;
      }
      // What's its probe_distance?
      auto occupant_home_slot_index = get_home(occupant_slot.key);
      auto occupant_probe_distance =
          probe_distance(occupant_home_slot_index, slot_index);
      if (occupant_probe_distance < inserter_probe_distance) {
        // Replace and reinsert?
        std::swap(key, occupant_slot.key);
        std::swap(value, occupant_slot.value);
        home_slot_index = occupant_home_slot_index;
        inserter_probe_distance = occupant_probe_distance;
      }
      // Occupant was poorer or as poor as me
      // Continue to next slot
      slot_index = (slot_index + 1) & (num_slots - 1);
      ++inserter_probe_distance;
    }
    // Found an empty slot
    occupied[slot_index] = true;
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
    size_t finder_probe_distance = 0;

    while (occupied[slot_index]) {
      const auto &occupant = slots[slot_index];
      if (occupant.key == key) {
        return {occupant.value};
      }
      // Check if we're done already
      auto occupant_home_slot_index = get_home(occupant.key);
      auto occupant_probe_distance =
          probe_distance(occupant_home_slot_index, slot_index);
      if (occupant_probe_distance < finder_probe_distance) {
        // We've stumbled into the next section, our key can't be here
        return std::nullopt;
      }

      // On to next slot
      slot_index = (slot_index + 1) & (num_slots - 1);
      ++finder_probe_distance;
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
    size_t eraser_probe_distance = 0;

    while (occupied[slot_index]) {
      const auto &occupant = slots[slot_index];
      if (occupant.key == key) {
        break;
      }
      // Check if we're done already
      auto occupant_home_slot_index = get_home(occupant.key);
      auto occupant_probe_distance =
          probe_distance(occupant_home_slot_index, slot_index);
      if (occupant_probe_distance < eraser_probe_distance) {
        // We've stumbled into the next section, our key can't be here
        return false;
      }

      // On to next slot
      slot_index = (slot_index + 1) & (num_slots - 1);
      ++eraser_probe_distance;
    }
    if (!occupied[slot_index]) {
      // Didn't find the slot
      return false;
    }

    // We know that the slot to erase is slot_index now
    occupied[slot_index] = false;
    auto previous_slot_index = slot_index;
    slot_index = (slot_index + 1) & (num_slots - 1);
    --num_entries;
    while (occupied[slot_index]) {
      auto &occupant = slots[slot_index];
      auto occupant_home_slot_index = get_home(occupant.key);
      if (occupant_home_slot_index == slot_index) {
        return true;
      }
      // Need to shift it back one
      std::swap(occupant, slots[previous_slot_index]);
      std::swap(occupied[slot_index], occupied[previous_slot_index]);

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
    std::vector<bool> new_occupied(num_slots, false);
    std::vector<Slot> new_slots(num_slots);

    for (size_t slot_index = 0; slot_index < occupied.size(); ++slot_index) {
      if (!occupied[slot_index]) {
        continue;
      }
      auto &old_occupant = slots[slot_index];
      // Need to insert into new
      auto new_home_slot_index = get_home(old_occupant.key);
      auto new_slot_index = new_home_slot_index;
      size_t inserter_probe_distance = 0;

      while (new_occupied[new_slot_index]) {
        auto &new_occupant = new_slots[new_slot_index];
        // What's its probe_distance?
        auto occupant_home_slot_index = get_home(new_occupant.key);
        auto occupant_probe_distance =
            probe_distance(occupant_home_slot_index, new_slot_index);
        if (occupant_probe_distance < inserter_probe_distance) {
          // Replace and reinsert?
          std::swap(old_occupant, new_occupant);
          new_home_slot_index = occupant_home_slot_index;
          inserter_probe_distance = occupant_probe_distance;
        }
        // Occupant was poorer or as poor as me
        // Continue to next slot
        new_slot_index = (new_slot_index + 1) & (num_slots - 1);
        ++inserter_probe_distance;
      }
      // Found an empty slot
      new_occupied[new_slot_index] = true;
      new_slots[new_slot_index] = old_occupant;
    }

    occupied = std::move(new_occupied);
    slots = std::move(new_slots);
  }

  size_t num_entries = 0;
  size_t num_slots = 16;
  std::vector<bool> occupied;
  std::vector<Slot> slots;
};
