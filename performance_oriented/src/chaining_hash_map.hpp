#pragma once

#include <cstddef>
#include <functional>
#include <memory>
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

struct LinkedListNode {

  LinkedListNode(int key, int value) : key(key), value(value), next(nullptr) {}

  int key;
  int value;
  std::unique_ptr<LinkedListNode> next;
};

struct LinkedList {

  struct Iterator {
    Iterator(LinkedListNode *node) : node(node) {}

    LinkedListNode &operator*() const { return *node; }
    Iterator &operator++() {
      node = node->next.get();
      return *this;
    }
    bool operator!=(const Iterator &other) const { return node != other.node; }

    LinkedListNode *node;
  };

  [[nodiscard]]
  Iterator begin() const {
    return {head.get()};
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
    auto *current_node = head.get();
    while (current_node != nullptr) {
      if (current_node->key == key) {
        return {current_node->value};
      }
      current_node = current_node->next.get();
    }
    return std::nullopt;
  }

  // Return true on successful insert, false on replacement?
  bool insert(int key, int value) {
    if (head == nullptr) {
      auto new_node = std::make_unique<LinkedListNode>(key, value);
      head = std::move(new_node);
      tail = head.get();
      length = 1;
      return true;
    }
    auto *current_pointer = head.get();
    while (current_pointer != nullptr) {
      if (current_pointer->key == key) {
        current_pointer->value = value;
        return false;
      }
      current_pointer = current_pointer->next.get();
    }
    force_insert(key, value);
    return true;
  }

  bool erase(int key) {
    auto *current_pointer = head.get();
    auto *previous_pointer = head.get();
    while (current_pointer != nullptr) {
      if (current_pointer->key == key) {
        if (current_pointer == head.get() && current_pointer == tail) {
          head = nullptr;
          tail = nullptr;
        } else if (current_pointer == head.get()) {
          head = std::move(head->next);
          return true;
        } else if (current_pointer == tail) {
          tail = previous_pointer;
          tail->next = nullptr;
        } else {
          previous_pointer->next = std::move(current_pointer->next);
        }
        --length;
        return true;
      }
      previous_pointer = current_pointer;
      current_pointer = previous_pointer->next.get();
    }
    return false;
  }

  [[nodiscard]]
  bool empty() const {
    return length == 0;
  }

  [[nodiscard]]
  size_t size() const {
    return length;
  }

  void force_insert(int key, int value) {
    if (head == nullptr) {
      auto new_node = std::make_unique<LinkedListNode>(key, value);
      head = std::move(new_node);
      tail = head.get();
      length = 1;
      return;
    }
    auto new_node = std::make_unique<LinkedListNode>(key, value);
    tail->next = std::move(new_node);
    tail = tail->next.get();
    ++length;
  }

private:
  std::unique_ptr<LinkedListNode> head = nullptr;
  LinkedListNode *tail = nullptr;
  size_t length = 0;
};

class ChainingHashMap {
  static constexpr double MAX_LOAD_PERCENTAGE = 0.75;

public:
  ChainingHashMap() : buckets(num_buckets) {}

  ~ChainingHashMap() = default;

  ChainingHashMap(const ChainingHashMap &) = delete;
  ChainingHashMap &operator=(const ChainingHashMap &) = delete;
  ChainingHashMap(ChainingHashMap &&) = delete;
  ChainingHashMap &operator=(ChainingHashMap &&) = delete;

  void insert(int key, int value) {
    auto bucket_index = get_bucket_index(key);
    if (buckets[bucket_index].insert(key, value)) {
      ++num_entries;
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
    auto did_erase = buckets[bucket_index].erase(key);
    if (did_erase) {
      --num_entries;
    }
    return did_erase;
  }

  [[nodiscard]]
  size_t size() const {
    return num_entries;
  }
  [[nodiscard]]
  bool empty() const {
    return num_entries == 0;
  }

private:
  void grow() {
    num_buckets *= 2;
    std::vector<LinkedList> new_buckets(num_buckets);
    // Iterate over our linked list and pushing (key, value) into new linked
    // list based on new hash

    for (const auto &old_bucket : buckets) {
      for (const auto &node : old_bucket) {
        auto new_bucket_index = get_bucket_index(node.key);
        new_buckets[new_bucket_index].force_insert(node.key, node.value);
      }
    }

    buckets = std::move(new_buckets);
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
  std::vector<LinkedList> buckets;
};
