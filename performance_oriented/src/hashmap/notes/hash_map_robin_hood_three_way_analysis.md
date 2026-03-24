# Robin Hood Three-Way Comparison: uint8_t vs bool vs Stored Probe Distance

Comparing three variants of the Robin Hood hash map. All use SoA layout (separate
state and key-value arrays) and identical Robin Hood insertion + backshift deletion.

Apple Silicon M4, P-core L2 = 16MB, 128-byte cache lines. LLVM/Clang 21.1.8, -O3.

## The three variants

1. **V1 uint8_t** — `vector<uint8_t>` state (0/1), recomputes probe distance via
   `get_home()` at every probe step
2. **V1 bool** — `vector<bool>` (bit-packed) state, same recomputation
3. **V2 stored PD** — `vector<uint8_t>` where 0 = empty, 1+ = probe distance + 1,
   eliminating the `get_home()` recomputation

The hypothesis was that storing probe distance in the state byte would be faster
by avoiding a hash + mask per probe step. It wasn't.

## Core scenarios at 65536 entries

### Insert — V1 uint8_t wins, stored PD adds overhead

| Load | V1 uint8_t | V1 bool | V2 stored PD | uint8_t vs bool | uint8_t vs V2 |
|---|---|---|---|---|---|
| 0.5 | 185K ns | 265K ns | 201K ns | 1.43x faster | 1.09x faster |
| 0.75 | 197K ns | 312K ns | 224K ns | 1.58x faster | 1.14x faster |
| 0.9 | 204K ns | 338K ns | 239K ns | 1.66x faster | 1.17x faster |

V1 uint8_t is the fastest inserter. V2's stored probe distance adds overhead
maintaining the value during Robin Hood displacement swaps — you have to update
the stored distance for both the displaced and displacing elements. This extra
bookkeeping costs more than the `get_home()` call it eliminates.

### FindHit — all three identical

| Load | V1 uint8_t | V1 bool | V2 stored PD |
|---|---|---|---|
| 0.5 | 48.7K ns | 48.7K ns | 48.7K ns |
| 0.75 | 48.7K ns | 48.7K ns | 48.7K ns |
| 0.9 | 48.7K ns | 48.7K ns | 48.7K ns |

Sequential key lookup is dominated by the key comparison. The state check and
probe distance computation are negligible.

### FindMiss — all three tied

| Load | V1 uint8_t | V1 bool | V2 stored PD |
|---|---|---|---|
| 0.5 | 32.4K ns | 32.6K ns | 32.5K ns |
| 0.75 | 32.5K ns | 32.5K ns | 32.5K ns |
| 0.9 | 32.5K ns | 32.5K ns | 32.5K ns |

All three variants are within measurement noise. With Apple Clang, V1 bool was
~5% slower here (34.1K vs 32.5K) — LLVM-21 generates tighter code for the
`vector<bool>` bit extraction, eliminating the penalty. The early termination on
probe distance dominates regardless of state representation.

### EraseAndFind — all tied

| Load | V1 uint8_t | V1 bool | V2 stored PD |
|---|---|---|---|
| 0.5 | 25.8K ns | 25.4K ns | 25.8K ns |
| 0.75 | 25.8K ns | 25.4K ns | 25.8K ns |
| 0.9 | 25.8K ns | 25.4K ns | 25.8K ns |

~25.4–25.8K ns across all variants and load factors. Bool has a marginal edge
(~1.5% faster), likely from the smaller state array improving cache utilization
during the interleaved erase+find pattern.

### EraseChurn — bool is fastest, tiny absolute times

| Load | V1 uint8_t | V1 bool | V2 stored PD |
|---|---|---|---|
| 0.5 | 4.4 ns | 2.8 ns | 4.4 ns |
| 0.75 | 4.3 ns | 2.8 ns | 4.2 ns |
| 0.9 | 4.0 ns | 2.8 ns | 4.0 ns |

Bool wins this tight insert+erase churn loop by ~1.5x. The `vector<bool>` proxy
swap appears cheaper than byte swaps in this specific pattern, or the compiler
optimizes the bit operations differently. Both uint8_t variants are nearly tied.
Absolute times are sub-5ns, so this is not practically significant.

## Hash quality scenarios at 65536, load 0.75

| Scenario | V1 uint8_t | V1 bool | V2 stored PD |
|---|---|---|---|
| Insert Random | 1.07M ns | 1.12M ns | 1.08M ns |
| FindHit Random | 151K ns | 166K ns | 184K ns |
| Insert Strided | 492K ns | 593K ns | 497K ns |
| FindHit Strided | 139K ns | 171K ns | 138K ns |

V1 uint8_t wins or ties everywhere. Random/strided keys create longer probe chains,
amplifying the per-probe overhead of `vector<bool>`. V2's stored probe distance
doesn't help — and on FindHit Random it's slightly slower than V1 uint8_t, possibly
because the stored byte adds a data dependency the branch predictor can't hide.

## Large scale at load 0.75

### Insert — uint8_t advantage holds at scale

| Entries | V1 uint8_t | V1 bool | V2 stored PD | uint8_t vs bool | uint8_t vs V2 |
|---|---|---|---|---|---|
| 65K | 197K ns | 319K ns | 228K ns | 1.62x | 1.16x |
| 262K | 786K ns | 1.28M ns | 895K ns | 1.63x | 1.14x |
| 2M | 6.79M ns | 11.3M ns | 7.60M ns | 1.66x | 1.12x |
| 4M | 13.8M ns | 23.3M ns | 14.7M ns | 1.69x | 1.07x |

V1 uint8_t maintains a consistent ~1.1-1.2x lead over V2 at all scales. Bool falls
further behind at scale (1.6-1.7x).

### FindHit — all identical at scale

| Entries | V1 uint8_t | V1 bool | V2 stored PD |
|---|---|---|---|
| 65K | 48.7K ns | 49.3K ns | 48.7K ns |
| 262K | 195K ns | 196K ns | 195K ns |
| 2M | 1.56M ns | 1.56M ns | 1.56M ns |
| 4M | 3.12M ns | 3.16M ns | 3.12M ns |

### FindHit Random — all converge at scale

| Entries | V1 uint8_t | V1 bool | V2 stored PD |
|---|---|---|---|
| 65K | 135K ns | 158K ns | 180K ns |
| 262K | 1.01M ns | 1.03M ns | 1.08M ns |
| 2M | 15.5M ns | 15.3M ns | 15.8M ns |
| 4M | 36.6M ns | 36.7M ns | 37.1M ns |

At large scale with random access, all variants converge — cache misses dominate
everything else.

## Why stored probe distance didn't help

The key insight: **eliminating a recomputation only helps if the computation is
expensive.** With `std::hash<int>` (identity hash), `get_home(key)` is:

```cpp
return std::hash<int>()(key) & (num_slots - 1);  // one AND instruction
```

This is essentially free — a single cycle. Meanwhile, storing probe distance adds
cost on every insert:
- Writing the probe distance value (not just 0/1) to the state array
- Updating the stored distance for both elements during Robin Hood swaps
- Extra arithmetic to encode/decode (stored value = probe distance + 1)

The overhead of maintaining the stored value exceeds the cost of recomputing it.

This could change with a more expensive hash function (e.g. Fibonacci hash involves
a multiply + shift). But with identity hash, there's nothing to save.

## Summary

| Scenario | Winner | Magnitude |
|---|---|---|
| Insert (all scales) | V1 uint8_t | 1.1-1.2x over V2, 1.4-1.7x over bool |
| FindHit (sequential) | Tied | |
| FindMiss | Tied | All three within noise |
| FindHit (random/strided) | V1 uint8_t | Slight edge |
| EraseChurn | Bool | 1.5x, sub-5ns absolute |
| Large Insert | V1 uint8_t | Consistent ~1.1x over V2 |

The `vector<bool>` → `vector<uint8_t>` change from the previous experiment remains
the clear win for insert-heavy workloads. Adding stored probe distance on top was
a net negative — a reminder that optimizing away cheap operations can make things
worse if the bookkeeping cost exceeds what you save.
