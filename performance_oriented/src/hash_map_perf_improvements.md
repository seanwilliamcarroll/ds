# Hash Map Performance Improvements

Potential optimizations for our implementations, ordered roughly by expected impact.

## 1. Replace `vector<bool>` with `vector<uint8_t>` (Robin Hood)

Robin Hood uses `vector<bool>` for the occupied bitmap, which is bit-packed (~0.125
bytes per slot). This saves memory (16.25 vs ~18 bytes/entry) but every probe requires
bit manipulation to extract a single bit — shifts, masks, and potentially crossing byte
boundaries.

Switching to `vector<uint8_t>` makes each probe a single byte load. The memory cost is
small (~1.75 bytes/entry increase) and may be recouped through better cache behavior
since the access pattern becomes simpler and more predictable.

**Tradeoff:** memory vs probe throughput. At high load factors where probe chains are
longer, the per-probe cost dominates and `uint8_t` should win clearly.

## 2. Merge two arrays into one (Linear Probing, Robin Hood)

Both open addressing implementations use two separate vectors — one for metadata
(`slot_state` or `occupied`) and one for key-value slots. Every probe touches two
cache lines in unrelated memory regions.

Merging into a single `vector<Entry>` where each entry contains its metadata byte
alongside the key-value pair means a single cache line fetch gives you everything
needed for a probe step. The struct would be ~12 bytes (1 byte state + 3 padding +
4 byte key + 4 byte value), or 9 bytes packed.

**Tradeoff:** struct padding may waste bytes per slot, but the cache locality gain
should more than compensate. This is likely the single highest-impact change for
open addressing.

## 3. Better hash function

`std::hash<int>` is typically the identity function — `hash(k) = k`. Sequential keys
map to sequential slots, which is fine for our benchmarks (sequential insert is
actually cache-friendly). But in real workloads with clustered keys, identity hashing
creates long probe chains.

Fibonacci hashing (`key * 2654435769u >> shift`) scatters keys across the table with
minimal clustering. It's a single multiply and shift — negligible cost per operation.

**Tradeoff:** for our sequential-key benchmarks, a better hash might actually hurt
slightly (breaks the sequential access pattern). For realistic workloads it's a clear
win. Worth benchmarking both.

## 4. Store probe distance in slot (Robin Hood)

Robin Hood currently recomputes `probe_distance(getHome(key), current_slot)` at every
probe step during insert, find, and erase. This involves a hash computation and modular
arithmetic per probe.

Storing the probe distance (or home index) directly in each slot eliminates the
recomputation. The Robin Hood swap decision becomes a single integer comparison. This
adds 1-4 bytes per slot but removes a hash + mod per probe step.

**Tradeoff:** increases slot size (hurts cache density) but reduces per-probe compute.
Most impactful at high load factors where probe chains are long.

## 5. Prefetching

When probing linearly, the next slot to check is predictable. Issuing a
`__builtin_prefetch` for the next probe position while processing the current one
hides memory latency. At large table sizes where the working set exceeds L2/L3 cache,
this can significantly reduce stall cycles.

**Tradeoff:** prefetching adds instruction overhead and is only useful when the table
doesn't fit in cache. At N=65536 with ~9 bytes/slot, the table is ~590KB — near the
boundary of L2 cache. Larger tables would benefit more.

## 6. Pool allocator for chaining

Our chaining implementation calls `make_unique` for every inserted node, hitting the
general-purpose allocator each time. This is the primary reason chaining is 10-14x
slower than open addressing — not the algorithm, but the allocation pattern.

A pool/arena allocator pre-allocates a slab of nodes and hands them out via pointer
bump. This eliminates per-node `malloc` overhead and keeps nodes contiguous in memory
(improving cache behavior during traversal).

**Tradeoff:** adds implementation complexity and memory management responsibility.
Won't close the gap with open addressing entirely (pointer chasing still has
fundamental cache disadvantages) but should narrow it substantially. This is the
highest-impact change for chaining specifically.
