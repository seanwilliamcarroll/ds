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

## 4. Store probe distance in slot (Robin Hood) — DONE (net negative)

**Status:** Implemented in RobinHoodHashMapV2. Stores probe distance in the uint8_t
state byte (0 = empty, 1+ = probe distance + 1), eliminating the `get_home()`
recomputation during probing.

**Results:** 9–17% slower on insert at all scales and load factors. Tied or marginally
worse everywhere else. The bookkeeping of updating stored distances during Robin Hood
swaps (both displaced and displacing elements) costs more than the single AND instruction
it eliminates — `get_home()` with identity hash is essentially free.

**Verdict:** Not worth it with identity hash. Would only help with expensive hash
functions (Fibonacci multiply+shift, cryptographic) where eliminating a hash call per
probe step saves real cycles. See `hash_map_robin_hood_three_way_analysis.md`.

## 5. Prefetching — DONE (no improvement)

**Status:** Tested `__builtin_prefetch` for the next probe slot during linear probing.

**Results:** 0% improvement across all scenarios, including at 4M entries (fully
cache-cold, ~50MB table exceeding the 16MB L2). The probe loop body is ~3–5 cycles,
but a prefetch to DRAM takes 100+ cycles — the prefetched data hasn't arrived by the
time the next iteration needs it. Average probe chain length is ~2.5 slots, so
prefetching further ahead wastes effort on slots never visited. The hardware prefetcher
already handles sequential access patterns (identity hash + sequential keys).

**What would actually work:** Batch prefetching — compute N home slots upfront,
prefetch all, then process. This amortizes the latency across lookups rather than
across probe steps. See `hash_map_prefetching_analysis.md`.

## 6. Pool allocator for chaining — DONE

**Status:** Implemented in ChainingHashMapV2 with an intrusive free list.

**Results:** 3–6x faster on insert, ~10x faster on erase+find. Makes chaining
competitive with open addressing.

The intrusive free list reuses each node's `next` pointer to thread free nodes
together, requiring zero additional memory beyond the node pool itself. Key detail:
when growing, nodes must be fully reinitialized (constructor, not just key/value
assignment) to clear stale `next` pointers before reinsertion.
