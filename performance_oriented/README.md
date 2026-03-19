# Performance-Oriented Data Structures

The goal here isn't just asymptotic complexity — it's understanding *why* one
structure beats another on real hardware. Cache behavior, memory layout, and
branch prediction matter as much as Big-O.

---

## Hash Map Internals

Most engineers use hash maps without understanding why `std::unordered_map` is
notoriously slow despite O(1) amortized operations.

**The problem with chaining (what `unordered_map` does):**
- Each bucket is a linked list — every lookup that hits a chain follows a pointer
- Pointer chasing is cache-hostile: each node is a separate heap allocation,
  scattered in memory
- High constant factors despite good asymptotic behavior

**Open addressing — the cache-friendly alternative:**
All entries live in a single contiguous array. On collision, probe for the next
open slot. No pointer chasing.

- **Linear probing:** probe `h, h+1, h+2, ...` — maximally cache-friendly, but
  suffers from primary clustering (long runs form and grow)
- **Quadratic probing:** probe `h, h+1, h+4, h+9, ...` — reduces clustering,
  slightly less cache-friendly
- **Double hashing:** probe offset is itself a hash — good distribution, but
  two hash computations per probe

**Robin Hood hashing:**
An open addressing scheme that reduces variance in probe length. When inserting,
if the new element has probed further from its home than the element currently
in the slot ("richer"), evict the current occupant and continue inserting it.
This keeps all elements close to their home slot.

- Lookup can terminate early: if the current element's probe distance is less
  than ours would be, the target isn't in the table
- Used in production hash maps (Rust's `HashMap`, Abseil's `flat_hash_map`)

**Load factor:**
The fraction of slots occupied. High load → more collisions, longer probe
sequences. Typical threshold to resize: 0.75 (chaining) or 0.5–0.875 (open
addressing). Resizing doubles the array and rehashes everything — O(n) but
amortized O(1) per insert.

**Deletion in open addressing:**
Chaining just unlinks a node. Open addressing can't simply empty a slot — probe
chains pass *through* occupied slots, so emptying one breaks lookups for elements
inserted past it.

- **Tombstones:** mark deleted slots as "deleted, but keep probing past me."
  Insert reuses tombstones; lookup probes through them. Simple, but tombstones
  accumulate and degrade probe lengths. Need periodic rehash to clean up.
- **Backshift deletion:** shift displaced elements backward to fill the gap.
  Stop at an empty slot or an element already at its home position. No tombstones,
  probe lengths stay clean. More complex, but pairs naturally with Robin Hood
  (you already track probe distances).

**What to understand:**
- Why `absl::flat_hash_map` and Rust's `HashMap` outperform `std::unordered_map`
- The tradeoff between load factor and memory usage
- How SIMD can accelerate probing (SSE2/AVX2 group lookup in Swiss table / `hashbrown`)

**Practice plan — build up from scratch (`int → int`):**

| # | Phase | What | Why | Status |
|---|-------|------|-----|--------|
| 0 | Chaining | Hash map with separate chaining (linked list per bucket), insert, find, erase, rehash | Build the baseline; feel the pointer-chasing pain firsthand before fixing it | |
| 1 | Linear probing | Open-addressed hash map with insert, find, erase (tombstone deletion), rehash | Core mechanics: hashing, probing, load factor, resize | |
| 2 | Robin Hood | Swap to Robin Hood insertion + backshift deletion | Reduces probe variance, eliminates tombstones, enables early termination on lookup | |
| 3 | Benchmarking | Benchmark all three implementations against `std::unordered_map` | See *why* open addressing wins — vary load factor, hit/miss ratio, table size | |

---

## Cache-Friendly Data Structures

Modern CPUs are fast; memory is slow. A cache miss costs ~100–300 cycles.
Structure layout often matters more than algorithmic complexity.

**SoA vs AoS:**

Array of Structs (AoS) — the natural OOP layout:
```
struct Particle { float x, y, z, mass; };
Particle particles[N];
```
If you're iterating over positions only, you load `mass` into cache on every
cache line even though you never use it. Wasted bandwidth.

Struct of Arrays (SoA) — the SIMD/performance layout:
```
struct Particles { float x[N], y[N], z[N], mass[N]; };
```
Now iterating over `x` is a sequential read of a dense array — optimal
prefetching, and amenable to SIMD (process 4 or 8 floats per instruction).
This is the standard layout in game engines and scientific computing.

**B-tree vs BST:**
A BST has O(log n) operations but terrible cache behavior — each node is a
separate allocation, and a lookup traverses O(log n) pointer hops to scattered
memory.

A B-tree stores multiple keys per node (branching factor B). A node fits in
one or a few cache lines. A lookup still takes O(log_B n) steps, but each step
is a sequential scan of a hot cache line rather than a pointer chase.

B-trees dominate in databases and filesystems precisely because pointer chasing
is catastrophic when data doesn't fit in L1/L2.

**Heap vs linked list (priority queue):**
A binary heap is a complete binary tree stored in a flat array — `parent(i) =
(i-1)/2`, `left(i) = 2i+1`, `right(i) = 2i+2`. No pointers.

A pointer-based heap (e.g., Fibonacci heap) has better asymptotic bounds for
decrease-key but much worse constants due to pointer chasing and poor locality.
In practice, binary heaps win unless decrease-key is the bottleneck.

**False sharing:**
When two threads write to different variables that share a cache line, every
write invalidates the other core's cache — even though they're touching
different data. Fix: pad structs so each thread-local field occupies its own
cache line (typically 64 bytes).

---

## Sorting

Sorting is where algorithmic theory and hardware reality collide most visibly.

**Quicksort:**
- Average O(n log n), worst O(n²) on bad pivots — mitigated by random pivot or
  median-of-three
- In-place, low overhead — works well in L1/L2 cache for small arrays
- Cache-friendly in the partition step: sequential scan from both ends
- Not stable

**Mergesort:**
- O(n log n) worst case, stable
- Requires O(n) auxiliary space — the merge step writes to a second buffer
- More cache misses than quicksort for in-memory data (two arrays touched
  during merge), but better for external sort (sequential I/O)
- Preferred for linked lists (no random access needed) and stable sort requirements

**Heapsort:**
- O(n log n) worst case, in-place, not stable
- Poor cache behavior: heap operations jump between distant indices
- Rarely used in practice despite good asymptotics — quicksort and mergesort
  both have better constants

**Introsort (what `std::sort` actually is):**
Quicksort by default, but falls back to heapsort when recursion depth exceeds
O(log n) (avoiding the O(n²) worst case), and insertion sort for small
subarrays (low overhead for n < ~16). Best of all worlds in practice.

**Radix sort:**
- O(nk) where k is the key width in digits/bits — not comparison-based
- Sorts integers or fixed-width keys by processing one digit at a time (LSD or MSD)
- Linear time for fixed-width integers (32-bit, 64-bit) — beats comparison sort
  for large n
- Cache behavior depends on implementation: LSD radix sort scatters writes
  across buckets, which is cache-unfriendly. Blocked/counting variants mitigate this.
- Used in GPU sorting and high-performance integer sorting

**When to use what:**
- General purpose, in-memory: introsort (`std::sort`)
- Need stability: mergesort (`std::stable_sort`)
- Integer keys, large n: radix sort
- External / disk-based: mergesort (sequential I/O)
- Tiny arrays (< 16 elements): insertion sort
