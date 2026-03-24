#pragma once

#ifndef __APPLE__
#error                                                                         \
    "alloc_counter.hpp requires macOS (uses malloc_size from <malloc/malloc.h>)"
#endif

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <malloc/malloc.h>
#include <new>

// Global allocation counter using macOS malloc_size().
// Tracks net bytes currently allocated (allocs - frees). Thread-safe.
//
// malloc_size() returns the actual usable size the allocator gave you, which
// may be larger than requested (allocators round up to bucket sizes). This
// measures real memory consumption, not requested bytes.
//
// Overrides global operator new/delete, so include in exactly one translation
// unit that participates in linking.
//
// Usage:
//   alloc_counter::reset();
//   // ... do work ...
//   auto bytes = alloc_counter::current();

namespace alloc_counter {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
inline std::atomic<int64_t> net_bytes{0};

inline void reset() { net_bytes.store(0, std::memory_order_relaxed); }

inline int64_t current() { return net_bytes.load(std::memory_order_relaxed); }

} // namespace alloc_counter

// NOLINTBEGIN(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
void *operator new(size_t size) {
  void *ptr = std::malloc(size);
  if (ptr == nullptr) {
    throw std::bad_alloc();
  }
  alloc_counter::net_bytes.fetch_add(static_cast<int64_t>(malloc_size(ptr)),
                                     std::memory_order_relaxed);
  return ptr;
}

void operator delete(void *ptr) noexcept {
  if (ptr == nullptr) {
    return;
  }
  alloc_counter::net_bytes.fetch_sub(static_cast<int64_t>(malloc_size(ptr)),
                                     std::memory_order_relaxed);
  std::free(ptr);
}
// NOLINTEND(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)

void operator delete(void *ptr, size_t /*unused*/) noexcept {
  ::operator delete(ptr);
}
