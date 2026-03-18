#pragma once

#include <vector>

// MinHeap — a complete binary tree stored in an array where every parent
// is less than or equal to its children. The minimum element is always at
// the root (index 0).
//
// Array index relationships:
//   Parent of i   → (i - 1) / 2
//   Left child    → 2i + 1
//   Right child   → 2i + 2
//
// Operations:
//   push(val)  — insert a value               O(log n)
//   pop()      — remove and return minimum    O(log n)
//   top()      — peek at minimum              O(1)
//   size()     — number of elements           O(1)
//   empty()    — true if no elements          O(1)

template <typename T> class MinHeap {
public:
  void push(const T &val) {
    data_.push_back(val);
    bubble_up(data_.size() - 1);
  }

  T pop() {
    std::swap(data_.front(), data_.back());
    auto output = data_.back();
    data_.pop_back();
    bubble_down(0);
    return output;
  }

  [[nodiscard]]
  const T &top() const {
    return data_.front();
  }

  [[nodiscard]]
  size_t size() const {
    return data_.size();
  }

  [[nodiscard]]
  bool empty() const {
    return data_.empty();
  }

private:
  std::vector<T> data_;

  void bubble_up(size_t index) {
    while (index > 0) {
      auto parent_index = (index - 1) / 2;
      if (data_[index] < data_[parent_index]) {
        std::swap(data_[parent_index], data_[index]);
        index = parent_index;
      } else {
        return;
      }
    }
  }

  void bubble_down(size_t index) {
    // Need to compare index with its two children, swapping if bigger than one
    while (index < data_.size()) {
      auto left_child = (2 * index) + 1;
      auto right_child = left_child + 1;
      if (left_child >= data_.size()) {
        return;
      }
      if (right_child < data_.size()) {
        if (data_[left_child] < data_[right_child] &&
            data_[left_child] < data_[index]) {
          std::swap(data_[left_child], data_[index]);
          index = left_child;
        } else if (data_[right_child] < data_[index]) {
          std::swap(data_[right_child], data_[index]);
          index = right_child;
        } else {
          return;
        }
      } else if (data_[left_child] < data_[index]) {
        std::swap(data_[left_child], data_[index]);
        index = left_child;
      } else {
        return;
      }
    }
  }
};
