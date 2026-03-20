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

class RobinHoodHashMap {

  static constexpr double MAX_LOAD = 0.75;

  struct Slot {
    int key;
    int value;
  };

public:
  RobinHoodHashMap() : occupied(num_slots), slots(num_slots) {}
  ~RobinHoodHashMap() = default;

  RobinHoodHashMap(const RobinHoodHashMap &) = delete;
  RobinHoodHashMap &operator=(const RobinHoodHashMap &) = delete;
  RobinHoodHashMap(RobinHoodHashMap &&) = delete;
  RobinHoodHashMap &operator=(RobinHoodHashMap &&) = delete;

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
    (void)key;
    (void)value;
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
    (void)key;
    return std::nullopt;
  }

  bool erase(int key) {
    // TODO: Backshift deletion
    // 1. Find the slot holding key (same early termination as find)
    // 2. Mark it empty, then look at the next slot:
    //    a. If empty or at its home position (probe distance 0), stop
    //    b. Otherwise, shift it back into the gap and repeat
    // 3. Decrement size
    (void)key;
    return false;
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
  }

  size_t num_entries = 0;
  size_t num_slots = 16;
  std::vector<bool> occupied;
  std::vector<Slot> slots;
};
