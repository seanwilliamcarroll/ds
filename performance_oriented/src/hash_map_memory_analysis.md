# Hash Map Memory Analysis

## Measurement Strategy

We use macOS `malloc_size()` to track net heap bytes without tainting allocation
behavior. Unlike the common approach of prepending a size header to every allocation
(which adds 8 bytes per alloc and shifts alignment), `malloc_size()` queries the
allocator for the usable size of an existing pointer — zero overhead on the allocation
itself.

The only overhead is an atomic increment/decrement in our `operator new`/`operator
delete` overrides. For open addressing this is negligible (a handful of vector
allocations). For chaining it's one atomic op per node insert — measurable but small
(~0.3ms at N=65536 vs ~2ms total insert time).

To keep timing benchmarks clean, the same source file (`hash_map_bench.cpp`) is compiled
into two CMake targets:
- `hash_map_bench` — timing only, no allocator override
- `hash_map_mem_bench` — compiled with `-DTRACK_MEMORY`, includes `alloc_counter.hpp`

`malloc_size()` returns the actual usable allocation size (allocators round up to bucket
sizes), so we measure real memory consumption, not requested bytes. This is arguably more
honest — it's what the process actually holds.

Platform guard: `alloc_counter.hpp` has `#if !defined(__APPLE__) #error` since
`malloc_size` is macOS-only. Linux equivalent would be `malloc_usable_size()` from
`<malloc.h>`.

## Results

Bytes per entry at N=65536:

| Implementation | Load 0.5 | Load 0.75 | Load 0.9 |
|---|---|---|---|
| Chaining | 64 | 64 | 64 |
| Linear Probing | 18 | 18 | 18 |
| Robin Hood | 16.25 | 16.25 | 16.25 |
| std::unordered_map | 57.25 | 44.75 | 44.75 |

### Why these numbers

**Chaining (64 bytes/entry):** Each entry is a heap-allocated `LinkedListNode` containing
two ints (8 bytes) + a `unique_ptr` (8 bytes) = 24 bytes requested, but `malloc` rounds
up. Plus the bucket array (`vector<LinkedList>`) overhead. The 64 bytes/entry is constant
across load factors because chaining allocates per-node regardless of table size.

**Linear Probing (18 bytes/entry):** Two vectors — `slot_state` (1 byte per slot,
`uint8_t` enum) and `slots` (8 bytes per slot, two ints). Total 9 bytes per slot. At load
factor 0.75, the table is ~1.33x the number of entries, so ~12 bytes per entry for the
slot data. The 18 bytes/entry includes vector overhead and allocator rounding. Constant
across load factors because the table doubles at the same relative threshold regardless.

**Robin Hood (16.25 bytes/entry):** Similar to linear probing but uses `vector<bool>`
(bit-packed, ~0.125 bytes per slot) instead of a `uint8_t` state vector. So ~8.125 bytes
per slot vs LP's 9. The bit-packing saves memory at the cost of slower access (bit
manipulation per probe). Replacing `vector<bool>` with `vector<uint8_t>` would bring it
to ~18 bytes/entry, matching linear probing.

**std::unordered_map (44.75–57.25 bytes/entry):** Chaining implementation with per-node
heap allocation, but its allocator is more efficient than our naive `make_unique` — 44.75
bytes vs our 64. The variation across load factors (57.25 at 0.5 vs 44.75 at 0.75/0.9)
comes from the bucket array being larger relative to entries at lower load.

### Memory vs time tradeoff

Robin Hood is both the most memory-efficient (16.25 bytes/entry) and fastest in most
scenarios. Chaining uses 4x the memory of Robin Hood and is 14x slower in erase+find.
The memory advantage compounds the cache advantage — smaller working set means more of the
table fits in cache.

### Erase + find memory (bytes_per_survivor at N=65536)

After inserting 65536 entries and erasing half:

| Implementation | Load 0.75 |
|---|---|
| Chaining | 112 |
| Linear Probing | 36 |
| Robin Hood | 32.5 |
| std::unordered_map | 57.5 |

Chaining jumps from 64 to 112 bytes per surviving entry — the bucket array is sized for
65536 entries but only 32768 remain. The erased nodes are freed, but the bucket array
isn't shrunk.

Open addressing tables also don't shrink, but since entries live in the flat slot array
(no per-entry allocation), the overhead is just unused slots. LP's 36 and RH's 32.5 are
roughly double their insert-time numbers — the table is half-empty.
