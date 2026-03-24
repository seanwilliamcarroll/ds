# Hash Map Performance Findings

Empirical results from benchmarking our implementations on Apple Silicon (M-series, P-core L2 = 16MB, 128-byte cache lines).

## Finding 1: AoS vs SoA for open addressing slots

**Background:** The original LinearProbingHashMap was implemented with separate arrays (SoA) — a `vector<SlotState>` for metadata and a `vector<pair<int,int>>` for key-value pairs. This was a deliberate choice based on the intuition that probing scans slot states far more heavily than it reads key-value data, so keeping the states in a dense, separate array would be more cache-friendly.

Claude recommended merging them into a single struct per slot (AoS), arguing that colocating metadata with key-value data would mean one cache line per probe instead of two. We tried it.

**Result:** The original SoA layout was better. The merged struct (AoS) was **1.3–21x slower** across nearly all scenarios.

### What we tested

LinearProbingHashMap (original, SoA) uses two separate vectors:
- `vector<SlotState> slot_state` — metadata only
- `vector<pair<int,int>> slots` — key-value pairs

LinearProbingHashMapV2 (AoS) merges them into:
- `vector<Slot> slots` where `Slot { SlotState state; int key; int value; }` — 12 bytes with padding

V2 also added Fibonacci hashing (see Finding 2), so the comparison isolates the layout change imperfectly — but the layout is clearly the dominant factor given that V2 is slower in every scenario except strided keys (which is the Fibonacci hash winning, not the layout).

### Results at 65536 entries, load 0.75

| Scenario | V1 (SoA) | V2 (AoS) | Change |
|---|---|---|---|
| Insert | 128K ns | 183K ns | 1.4x slower |
| FindHit | 45K ns | 60K ns | 1.3x slower |
| FindMiss | 41K ns | 80K ns | 2.0x slower |
| EraseAndFind | 21K ns | 30K ns | 1.4x slower |
| EraseChurn | 5 ns | 12 ns | 2.4x slower |

### Cache collapse at scale

| Entries | V1 FindHit | V2 FindHit | Change |
|---|---|---|---|
| 65K | 45K ns | 64K ns | 1.4x slower |
| 2M | 1.5M ns | 20.8M ns | 14x slower |
| 4M | 2.9M ns | 62M ns | 21x slower |

### Why

Linear probing's inner loop is: **check state → skip or match**. Most probes are skip-overs — the key-value data isn't needed at all. With SoA, the state array is dense:

- 128-byte cache line ÷ 1 byte per state = **128 states per fetch**
- 128-byte cache line ÷ 12 bytes per merged slot = **~10 slots per fetch**

The merged struct loads key-value bytes into cache just to check if a slot is empty. At small table sizes, the wasted cache capacity causes a modest ~1.4x penalty. At large sizes (4M entries, ~50MB table), the inflated working set thrashes the cache entirely — 21x slower.

FindMiss is hit hardest at every size (2x at 65K) because it probes until finding an empty slot, never reading any key-value data at all. Every cache line loaded is 83% waste.

### The one scenario where AoS would help

On a **successful find**, you do need the key (to confirm match) and value (to return). If most probes ended in a match rather than a skip, AoS would avoid a second cache miss to the key-value array. But in practice, linear probing's average probe length at 0.75 load is ~2.5, meaning most operations touch 1-2 skip slots before hitting the target — the state array scan dominates.

### Takeaway

AoS vs SoA is not a universal rule — it depends on which fields the hot path actually reads. For linear probing, the hot path is state scanning, and SoA keeps that path compact. The textbook "merge for locality" advice assumes every probe needs all fields. The original SoA design was the right call from the start.

## Finding 2: Fibonacci hashing fixes pathological clustering

**Hypothesis:** Replacing `std::hash<int>` (identity function) with Fibonacci hashing (`key * 2654435769u >> shift`) eliminates clustering for strided keys.

**Result:** Correct. Fibonacci hashing was **1.65–2.1x faster** for strided keys (multiples of 16).

| Scenario | Identity hash | Fibonacci hash | Change |
|---|---|---|---|
| Insert Strided | 376K ns | 228K ns | 1.65x faster |
| FindHit Strided | 153K ns | 71K ns | 2.1x faster |

### Why

With identity hash and power-of-two table sizes, `hash(k) & (size - 1)` for `k = 16i` maps all keys to the same few slots. Fibonacci hashing multiplies by the golden ratio constant and takes the high bits (via right shift, not mask — the high bits have better entropy from the multiply). This scatters keys uniformly regardless of input pattern.

### Tradeoff

For sequential keys, identity hash gives sequential slot access — cache-optimal. Fibonacci hashing breaks this pattern, costing ~1.3x on sequential benchmarks. Whether this tradeoff is worth it depends on expected key distributions. Real workloads rarely have perfectly sequential keys.

## Finding 3: Pool allocator transforms chaining performance

**Hypothesis:** Per-node `make_unique` allocation is the primary bottleneck in chaining hash maps, not the algorithm itself.

**Result:** Correct. Arena allocation with an intrusive free list made chaining **3–6x faster** on insert and **~10x faster** on erase+find, making it competitive with open addressing.

The intrusive free list reuses each node's `next` pointer to thread free nodes together, requiring zero additional memory beyond the node pool itself.
