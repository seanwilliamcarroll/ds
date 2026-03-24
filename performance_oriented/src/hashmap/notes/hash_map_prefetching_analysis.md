# Prefetching Analysis: Linear Probing Hash Map

Tested manual prefetching (`__builtin_prefetch`) in the linear probing probe loops.
Apple Silicon, P-core L2 = 16MB, 128-byte cache lines.

## Implementation

Added `__builtin_prefetch` for both the state array and key-value array one slot ahead
in each probe loop (insert, find, erase):

```cpp
while (...) {
    auto next = (bucket_index + 1) & (num_slots - 1);
    __builtin_prefetch(&slot_state[next], 0, 1);  // read, low locality
    __builtin_prefetch(&slots[next], 0, 1);
    // ... existing comparison logic ...
    bucket_index = next;
}
```

Two prefetches needed because of SoA layout — state and key-value are in separate memory
regions. (With AoS you'd only need one, which is one point in AoS's favor, though not
enough to overcome its density disadvantage — see `hash_map_perf_findings.md`.)

## Results

### Core scenarios at 65536 entries, load 0.75

| Scenario | Identity | Prefetch | Diff |
|---|---|---|---|
| Insert | 115K ns | 118K ns | +2% (noise) |
| FindHit | 45.4K ns | 45.5K ns | 0% |
| FindMiss | 47.7K ns | 39.3K ns | -18% (best case, inconsistent) |
| EraseAndFind | 20.9K ns | 20.9K ns | 0% |
| EraseChurn | 5.5 ns | 5.5 ns | 0% |

### Large scale at 4M entries, load 0.75

This is where prefetching should help the most — the table is ~50MB, fully cache-cold.

| Scenario | Identity | Prefetch | Diff |
|---|---|---|---|
| Insert Large | 7.97M ns | 7.88M ns | -1% (noise) |
| FindHit Large | 2.91M ns | 2.91M ns | 0% |
| FindHit Random Large | 36.3M ns | 36.2M ns | 0% |

No measurable improvement even at fully cache-cold table sizes.

### Hash quality scenarios at 65536 entries, load 0.75

| Scenario | Identity | Prefetch | Diff |
|---|---|---|---|
| Insert Random | 706K ns | 747K ns | +6% (noise/overhead) |
| FindHit Random | 142K ns | 127K ns | -10% |
| Insert Strided | 365K ns | 407K ns | +12% (worse) |
| FindHit Strided | 154K ns | 161K ns | +5% (noise) |

## Why prefetching didn't help

### The loop is too tight

The probe loop body is roughly 3-5 cycles of work:
1. Check `slot_state[i]` — one byte load
2. Maybe check `slots[i].key` — one int load (only if occupied)
3. Compute next index — one add + bitwise and
4. Branch back

A prefetch to DRAM takes ~100+ cycles. Even an L2 miss is ~10-15 cycles. The prefetch
for slot `i+1` fires, but we arrive at `i+1` only 3-5 cycles later — the prefetch
hasn't had time to complete.

### Prefetching further ahead doesn't work either

The natural fix would be to prefetch 2-3 slots ahead to give the memory system more
time. But at 0.75 load factor, the average probe chain is only ~2.5 slots long. Most
prefetches would be wasted (we've already found our slot or hit empty), polluting the
cache with unused data.

### Hardware prefetcher already handles sequential access

With identity hash, sequential keys map to sequential slots. The hardware prefetcher
detects this stride-1 pattern and prefetches automatically — our manual prefetches are
redundant.

### FindMiss: the one scenario with a signal

FindMiss showed an inconsistent -18% improvement at load 0.75. This makes sense
directionally: FindMiss probes until hitting an empty slot (longer chains than FindHit),
giving the prefetch more iterations to overlap with. But the improvement was inconsistent
across runs (first run showed noise, second showed -18%) and didn't appear at all in the
large-scale benchmarks where cache misses dominate.

## When would prefetching actually help?

Prefetching works best with:
- **Longer predictable access sequences** — B-tree traversals, large array scans, batch
  lookups where you know the next N keys in advance
- **Enough work per iteration** to hide the prefetch latency — e.g. processing a cache
  line's worth of data before moving to the next
- **Non-sequential access patterns** that defeat the hardware prefetcher — but only if
  the next address is known far enough in advance

Linear probing's short probe chains and tight loops are a poor fit. A more promising
approach would be **batch prefetching**: given a batch of keys to look up, compute all
their home slots, issue prefetches for all of them, then process them. This gives the
memory system 100+ cycles of useful work (computing other home slots) before you need
the first prefetched slot. This is what some high-performance hash table implementations
(e.g. Abseil's `PrefetchToLocalCache` in `raw_hash_set`) do.

## Verdict

Removed. The code complexity wasn't justified by inconsistent, marginal gains on a
single scenario. Batch prefetching would be a more promising direction if we revisit this.
