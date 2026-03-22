# Hash Map Performance Improvements

Potential optimizations for our implementations, ordered roughly by expected impact.
See `hash_map_perf_findings.md` for detailed benchmark results.

## 1. Replace `vector<bool>` with `vector<uint8_t>` (Robin Hood) — TODO

Robin Hood uses `vector<bool>` for the occupied bitmap, which is bit-packed (~0.125
bytes per slot). This saves memory (16.25 vs ~18 bytes/entry) but every probe requires
bit manipulation to extract a single bit — shifts, masks, and potentially crossing byte
boundaries.

Switching to `vector<uint8_t>` makes each probe a single byte load. The memory cost is
small (~1.75 bytes/entry increase) and may be recouped through better cache behavior
since the access pattern becomes simpler and more predictable.

**Tradeoff:** memory vs probe throughput. At high load factors where probe chains are
longer, the per-probe cost dominates and `uint8_t` should win clearly.

## 2. ~~Merge two arrays into one (Linear Probing, Robin Hood)~~ — REJECTED

**Status:** Tried in LinearProbingHashMapV2. Was 1.3–21x slower than the original
SoA layout across nearly all scenarios. The regression got dramatically worse at scale
(21x at 4M entries).

The original implementation already used separate arrays (SoA) — a deliberate design
choice based on the insight that probing scans slot states far more often than it reads
key-value data. This keeps the state array dense: 128 states per Apple Silicon cache
line vs ~10 merged 12-byte slots. The merged struct wastes cache capacity loading
key-value bytes just to check if a slot is empty.

**Verdict:** SoA is the correct layout for linear probing. Do not merge.

## 3. Better hash function — DONE (mixed results)

**Status:** Implemented Fibonacci hashing (`key * 2654435769u >> shift`) in
LinearProbingHashMapV2. Uses right-shift (not mask) to extract the high bits where
the multiply concentrates entropy.

**Results:**
- Strided keys (pathological): **1.65–2.1x faster** — clustering eliminated
- Sequential keys: **~1.3x slower** — breaks the cache-friendly sequential access pattern
- Random keys: **~1.3x slower** — no clustering to fix, but hash is slightly more expensive

**Verdict:** Clear win for real workloads where keys aren't perfectly sequential.
The shift (high bits) vs mask (low bits) distinction matters — low bits have weaker
entropy from the multiply. Store a `shift_` member updated on grow instead of
computing `log2` per probe.

## 4. Store probe distance in slot (Robin Hood) — TODO (proceed with caution)

Robin Hood currently recomputes `probe_distance(getHome(key), current_slot)` at every
probe step during insert, find, and erase. This involves a hash computation and modular
arithmetic per probe.

Storing the probe distance (or home index) directly in each slot eliminates the
recomputation. The Robin Hood swap decision becomes a single integer comparison. This
adds 1-4 bytes per slot but removes a hash + mod per probe step.

**Tradeoff:** increases slot size (hurts cache density) but reduces per-probe compute.
Most impactful at high load factors where probe chains are long.

**Caution:** Finding #2 showed that adding bytes per slot has real cache cost. The
state array's density is critical. If probe distance is stored alongside state in the
metadata array (not merged with key-value), the cost may be acceptable — but benchmark
carefully.

## 5. Prefetching — TODO

When probing linearly, the next slot to check is predictable. Issuing a
`__builtin_prefetch` for the next probe position while processing the current one
hides memory latency. At large table sizes where the working set exceeds L2/L3 cache,
this can significantly reduce stall cycles.

**Tradeoff:** prefetching adds instruction overhead and is only useful when the table
doesn't fit in cache. Our large-scale benchmarks (1<<22 = 4M entries, ~50MB table)
are fully cache-cold on Apple Silicon (P-core L2 = 16MB) — prefetching should help
most there.

## 6. Pool allocator for chaining — DONE

**Status:** Implemented in ChainingHashMapV2 with an intrusive free list.

**Results:** 3–6x faster on insert, ~10x faster on erase+find. Makes chaining
competitive with open addressing.

The intrusive free list reuses each node's `next` pointer to thread free nodes
together, requiring zero additional memory beyond the node pool itself. Key detail:
when growing, nodes must be fully reinitialized (constructor, not just key/value
assignment) to clear stale `next` pointers before reinsertion.
