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
  void push(const T &val);

  T pop();

  const T &top() const;

  [[nodiscard]]
  size_t size() const;

  [[nodiscard]]
  bool empty() const;

private:
  std::vector<T> data_;

  void bubble_up(size_t index);

  void bubble_down(size_t index);
};
