# Fibonacci Hash vs Identity Hash: Linear Probing Analysis

Comparing LinearProbingHashMap (V1, identity hash) vs LinearProbingHashMapV2 (V2, Fibonacci hash).
Both use the same SoA layout (separate state and key-value arrays). The only difference is the
hash function.

- V1: `std::hash<int>()(key)` — identity, i.e. `hash(x) = x`
- V2: `(static_cast<uint32_t>(key) * 2654435769U) >> shift` — Fibonacci, high bits via shift

Apple Silicon, P-core L2 = 16MB, 128-byte cache lines.

## Isolating compute cost vs memory access pattern cost

A natural question: is the V2 slowdown on sequential keys from the extra multiply+shift
instructions, or from scattering keys to non-sequential slots (worse cache behavior)?

We considered disassembling and comparing, or using a trick where we compute the Fibonacci
hash but return identity (to keep instruction count equal but preserve access patterns). But
the compiler would optimize away the unused multiply, making that unreliable.

Instead, the scaling data answers the question cleanly. Look at sequential FindHit across
table sizes:

| Entries | V1 | V2 | Ratio | Table fits in? |
|---|---|---|---|---|
| 256 | 182 ns | 208 ns | 1.14x | L1 (~2.3KB) |
| 4096 | 2.8K ns | 3.2K ns | 1.11x | L1 (~37KB) |
| 32768 | 22.8K ns | 29.0K ns | 1.28x | L2 (~295KB) |
| 65536 | 45.6K ns | 65.5K ns | 1.44x | L2 (~590KB) |
| 2M | 1.46M ns | 14.9M ns | 10.3x | Busts L2 |
| 4M | 2.91M ns | 43.7M ns | 15.0x | Fully cold |

**At 256 entries**, the entire table fits in L1 — every access is a cache hit regardless of
access pattern. The ~14% gap is purely the compute cost of the Fibonacci multiply+shift vs
the identity function (which is a no-op).

**At 4M entries**, the table is fully cache-cold. Identity hash gives sequential access
(each lookup is near the previous one, prefetcher-friendly). Fibonacci hash scatters keys
across 50MB of memory — every lookup is a cache miss. The 15x gap is almost entirely memory
access patterns.

**Key insight:** ~14% is compute overhead. Everything above that is cache/memory effects,
growing with table size as more of the access pattern becomes cache-cold.

### Confirmation: random access erases the gap

When access is already random, the hash function can't make it worse:

| Entries | V1 FindHit_Random_Large | V2 FindHit_Random_Large | Ratio |
|---|---|---|---|
| 65536 | 133K ns | 132K ns | 1.00x |
| 262144 | 971K ns | 1.06M ns | 1.09x |
| 2M | 15.1M ns | 16.1M ns | 1.06x |
| 4M | 35.9M ns | 38.3M ns | 1.07x |

With random keys looked up in random order, both hash functions produce scattered access.
The remaining ~7% gap is the compute cost of the multiply+shift — consistent with the ~14%
measured at L1-resident sizes (the difference shrinks because memory latency dominates and
the hash computation happens during stalls).

## Full comparison at 65536 entries

### Core scenarios (all load factors)

| Scenario | Load | V1 | V2 | Ratio |
|---|---|---|---|---|
| Insert | 0.5 | 123.9K ns | 168.3K ns | 1.36x slower |
| Insert | 0.75 | 123.7K ns | 180.5K ns | 1.46x slower |
| Insert | 0.9 | 131.8K ns | 194.1K ns | 1.47x slower |
| FindHit | 0.5 | 45.4K ns | 65.5K ns | 1.44x slower |
| FindHit | 0.75 | 45.6K ns | 65.7K ns | 1.44x slower |
| FindHit | 0.9 | 45.6K ns | 65.0K ns | 1.43x slower |
| FindMiss | 0.5 | 48.8K ns | 71.2K ns | 1.46x slower |
| FindMiss | 0.75 | 39.6K ns | 70.3K ns | 1.78x slower |
| FindMiss | 0.9 | 42.1K ns | 70.3K ns | 1.67x slower |
| EraseAndFind | 0.5 | 20.9K ns | 32.2K ns | 1.54x slower |
| EraseAndFind | 0.75 | 20.9K ns | 32.7K ns | 1.57x slower |
| EraseAndFind | 0.9 | 21.0K ns | 33.3K ns | 1.59x slower |
| EraseChurn | 0.5 | 5.5 ns | 13.9 ns | 2.54x slower |
| EraseChurn | 0.75 | 5.4 ns | 12.7 ns | 2.34x slower |
| EraseChurn | 0.9 | 5.2 ns | 12.0 ns | 2.29x slower |

Sequential keys are identity hash's best case — keys map to sequential slots, giving
linear cache access. Fibonacci hash scatters these, turning every probe into a potential
cache miss. The penalty is consistent across load factors for find/insert (~1.4–1.5x),
worse for FindMiss (~1.7x, more probing to reach empty), and worst for EraseChurn (~2.3x,
tight single-operation loop where the hash dominates).

### Hash quality scenarios (load 0.75)

| Scenario | V1 | V2 | Ratio |
|---|---|---|---|
| Insert Random | 738K ns | 873K ns | 1.18x slower |
| FindHit Random | 87.9K ns | 126K ns | 1.44x slower |
| **Insert Strided** | **376K ns** | **213K ns** | **1.77x faster** |
| **FindHit Strided** | **154K ns** | **73.0K ns** | **2.11x faster** |

**Random keys:** V2 is still slower because identity hash with random keys already gives
reasonable distribution — the keys are random, so `hash(k) & mask` scatters them naturally.
Fibonacci hash adds compute cost without improving distribution.

**Strided keys:** This is where Fibonacci hash earns its keep. With stride-16 keys, identity
hash maps everything to the same few slots (catastrophic clustering). Fibonacci hash scatters
them properly. The 2x speedup on find is pure clustering elimination.

### Large scale (load 0.75)

| Scenario | Entries | V1 | V2 | Ratio |
|---|---|---|---|---|
| Insert | 65K | 134K ns | 180K ns | 1.35x slower |
| Insert | 262K | 504K ns | 791K ns | 1.57x slower |
| Insert | 2M | 4.11M ns | 14.1M ns | 3.43x slower |
| Insert | 4M | 8.73M ns | 43.1M ns | 4.93x slower |
| FindHit | 65K | 45.5K ns | 65.1K ns | 1.43x slower |
| FindHit | 262K | 182K ns | 328K ns | 1.80x slower |
| FindHit | 2M | 1.46M ns | 14.9M ns | 10.3x slower |
| FindHit | 4M | 2.91M ns | 43.7M ns | 15.0x slower |
| FindHit Random | 65K | 133K ns | 132K ns | 1.00x (tied) |
| FindHit Random | 262K | 971K ns | 1.06M ns | 1.09x slower |
| FindHit Random | 2M | 15.1M ns | 16.1M ns | 1.06x slower |
| FindHit Random | 4M | 35.9M ns | 38.3M ns | 1.07x slower |

The sequential-key gap explodes with scale because identity hash preserves spatial locality
that the hardware prefetcher can exploit. Fibonacci hash destroys this.

The random-key gap stays flat (~7%) because both are already random — the hash function
can't make random access worse, so you're only paying the ~14% compute overhead (reduced
because memory stalls hide some of it).

## Summary

| Scenario type | Winner | Why |
|---|---|---|
| Sequential keys, fits in cache | V1 by ~14% | Compute overhead of multiply+shift |
| Sequential keys, busts cache | V1 by 10–15x | Identity preserves spatial locality |
| Random keys (any size) | Roughly tied | Both scatter; ~7% compute cost |
| Strided/clustered keys | V2 by 1.8–2.1x | Fibonacci eliminates pathological clustering |

Fibonacci hashing is a tradeoff: you trade sequential-key performance for robustness against
pathological key distributions. For a general-purpose hash map, this is usually the right
trade — real workloads rarely have perfectly sequential keys, but clustered keys are common.
For a specialized data structure where you know keys are sequential (e.g. dense integer IDs),
identity hash is strictly better.
