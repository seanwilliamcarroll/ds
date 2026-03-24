#pragma once

#include <algorithm>
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

struct LinkedListNodeV2 {
  LinkedListNodeV2() = default;

  LinkedListNodeV2(int key, int value) : key(key), value(value) {}

  int key = 0;
  int value = 0;
  LinkedListNodeV2 *next = nullptr;
};

struct LinkedListV2 {

  struct Iterator {
    Iterator(LinkedListNodeV2 *node) : node(node) {}

    LinkedListNodeV2 &operator*() const { return *node; }
    Iterator &operator++() {
      node = node->next;
      return *this;
    }
    bool operator!=(const Iterator &other) const { return node != other.node; }

    LinkedListNodeV2 *node;
  };

  [[nodiscard]]
  Iterator begin() const {
    return {head};
  }

  [[nodiscard]]
  static Iterator end() {
    return {nullptr};
  }

  [[nodiscard]]
  std::optional<int> find(int key) const {
    if (head == nullptr) {
      return std::nullopt;
    }
    auto *current_node = head;
    while (current_node != nullptr) {
      if (current_node->key == key) {
        return {current_node->value};
      }
      current_node = current_node->next;
    }
    return std::nullopt;
  }

  // Return true on successful insert, false on replacement
  // Tells us whether we used this new node or not
  bool insert(int key, int value, LinkedListNodeV2 *new_node) {
    if (head == nullptr) {
      *new_node = LinkedListNodeV2(key, value);
      head = new_node;
      tail = head;
      length = 1;
      return true;
    }
    auto *current_pointer = head;
    while (current_pointer != nullptr) {
      if (current_pointer->key == key) {
        current_pointer->value = value;
        return false;
      }
      current_pointer = current_pointer->next;
    }
    *new_node = LinkedListNodeV2(key, value);
    force_insert(new_node);
    return true;
  }

  LinkedListNodeV2 *erase(int key) {
    auto *current_pointer = head;
    auto *previous_pointer = head;
    LinkedListNodeV2 *old_node = nullptr;
    while (current_pointer != nullptr) {
      if (current_pointer->key == key) {
        if (current_pointer == head && current_pointer == tail) {
          old_node = head;
          head = nullptr;
          tail = nullptr;
        } else if (current_pointer == head) {
          old_node = head;
          head = head->next;
        } else if (current_pointer == tail) {
          old_node = tail;
          tail = previous_pointer;
          tail->next = nullptr;
        } else {
          old_node = current_pointer;
          previous_pointer->next = current_pointer->next;
        }
        --length;
        return old_node;
      }
      previous_pointer = current_pointer;
      current_pointer = previous_pointer->next;
    }
    return nullptr;
  }

  [[nodiscard]]
  bool empty() const {
    return length == 0;
  }

  [[nodiscard]]
  size_t size() const {
    return length;
  }

  void force_insert(LinkedListNodeV2 *new_node) {
    if (head == nullptr) {
      head = new_node;
      tail = head;
      length = 1;
      return;
    }
    tail->next = new_node;
    tail = tail->next;
    ++length;
  }

private:
  LinkedListNodeV2 *head = nullptr;
  LinkedListNodeV2 *tail = nullptr;
  size_t length = 0;
};

template <double MaxLoad = 0.75> class ChainingHashMapV2 {
  static constexpr double MAX_LOAD_PERCENTAGE = MaxLoad;

  static constexpr size_t NULL_INDEX = 0;

public:
  ChainingHashMapV2()
      : buckets(num_buckets), allocated_nodes(num_buckets),
        free_list_head(&allocated_nodes.front()) {
    auto *current_pointer = free_list_head;
    for (auto iter = (allocated_nodes.begin() + 1);
         iter != allocated_nodes.end(); ++iter) {
      auto &node = *iter;
      current_pointer->next = &node;
      current_pointer = current_pointer->next;
    }
  }

  ~ChainingHashMapV2() = default;

  ChainingHashMapV2(const ChainingHashMapV2 &) = delete;
  ChainingHashMapV2 &operator=(const ChainingHashMapV2 &) = delete;
  ChainingHashMapV2(ChainingHashMapV2 &&) = delete;
  ChainingHashMapV2 &operator=(ChainingHashMapV2 &&) = delete;

  void insert(int key, int value) {
    auto bucket_index = get_bucket_index(key);
    auto *new_node = free_list_head;
    auto *free_list_next = free_list_head->next;
    if (buckets[bucket_index].insert(key, value, new_node)) {
      ++num_entries;
      free_list_head = free_list_next;
    }

    // Need to rehash?
    if (should_resize()) {
      grow();
    }
  }

  [[nodiscard]]
  std::optional<int> find(int key) const {
    auto bucket_index = get_bucket_index(key);
    return buckets[bucket_index].find(key);
  }

  bool erase(int key) {
    auto bucket_index = get_bucket_index(key);
    auto *erased_node = buckets[bucket_index].erase(key);
    if (erased_node != nullptr) {
      --num_entries;
      erased_node->next = free_list_head;
      free_list_head = erased_node;
      return true;
    }
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
  void grow() {
    num_buckets *= 2;
    std::vector<LinkedListV2> new_buckets(num_buckets);
    std::vector<LinkedListNodeV2> new_allocated_nodes(num_buckets);
    free_list_head = &new_allocated_nodes.front();
    auto *current_node = free_list_head;
    for (auto iter = (new_allocated_nodes.begin() + 1);
         iter != new_allocated_nodes.end(); ++iter) {
      auto &node = *iter;
      current_node->next = &node;
      current_node = current_node->next;
    }

    // Iterate over our linked list and pushing (key, value) into new linked
    // list based on new hash
    for (const auto &old_bucket : buckets) {
      for (const auto &node : old_bucket) {
        auto new_bucket_index = get_bucket_index(node.key);
        auto *new_node = free_list_head;
        free_list_head = free_list_head->next;
        *new_node = LinkedListNodeV2(node.key, node.value);
        new_buckets[new_bucket_index].force_insert(new_node);
      }
    }

    buckets = std::move(new_buckets);
    allocated_nodes = std::move(new_allocated_nodes);
  }

  [[nodiscard]]
  size_t get_bucket_index(int key) const {
    return std::hash<int>()(key) % num_buckets;
  }

  [[nodiscard]]
  bool should_resize() const {
    auto load_percentage =
        static_cast<double>(num_entries) / static_cast<double>(num_buckets);
    return load_percentage > MAX_LOAD_PERCENTAGE;
  }

  size_t num_buckets = 16;
  size_t num_entries = 0;
  std::vector<LinkedListV2> buckets;

  std::vector<LinkedListNodeV2> allocated_nodes;
  LinkedListNodeV2 *free_list_head;
};
