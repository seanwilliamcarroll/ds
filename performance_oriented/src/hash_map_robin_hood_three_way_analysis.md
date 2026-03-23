# Robin Hood Three-Way Comparison: uint8_t vs bool vs Stored Probe Distance

Comparing three variants of the Robin Hood hash map. All use SoA layout (separate
state and key-value arrays) and identical Robin Hood insertion + backshift deletion.

Apple Silicon, P-core L2 = 16MB, 128-byte cache lines.

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
| 0.5 | 183K ns | 265K ns | 204K ns | 1.45x faster | 1.11x faster |
| 0.75 | 196K ns | 313K ns | 223K ns | 1.60x faster | 1.14x faster |
| 0.9 | 204K ns | 340K ns | 236K ns | 1.67x faster | 1.16x faster |

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

### FindMiss — uint8_t and V2 tied, bool ~5% slower

| Load | V1 uint8_t | V1 bool | V2 stored PD |
|---|---|---|---|
| 0.5 | 32.5K ns | 34.1K ns | 32.4K ns |
| 0.75 | 32.4K ns | 34.1K ns | 32.5K ns |
| 0.9 | 32.5K ns | 34.1K ns | 32.4K ns |

FindMiss probes until hitting an empty slot or early-terminating on probe distance.
The bit manipulation overhead of `vector<bool>` shows up here since the state check
is the dominant operation per probe step.

### EraseAndFind — all tied

~25.5K ns across all variants and load factors.

### EraseChurn — bool is fastest, tiny absolute times

| Load | V1 uint8_t | V1 bool | V2 stored PD |
|---|---|---|---|
| 0.5 | 4.23 ns | 2.71 ns | 4.20 ns |
| 0.75 | 3.97 ns | 2.67 ns | 3.68 ns |
| 0.9 | 4.01 ns | 2.68 ns | 3.67 ns |

Bool wins this tight insert+erase churn loop by ~1.5x. The `vector<bool>` proxy
swap appears cheaper than byte swaps in this specific pattern, or the compiler
optimizes the bit operations differently. Both uint8_t variants are nearly tied.
Absolute times are sub-5ns, so this is not practically significant.

## Hash quality scenarios at 65536, load 0.75

| Scenario | V1 uint8_t | V1 bool | V2 stored PD |
|---|---|---|---|
| Insert Random | 1.01M ns | 1.10M ns | 1.03M ns |
| FindHit Random | 147K ns | 166K ns | 167K ns |
| Insert Strided | 489K ns | 594K ns | 498K ns |
| FindHit Strided | 139K ns | 177K ns | 137K ns |

V1 uint8_t wins or ties everywhere. Random/strided keys create longer probe chains,
amplifying the per-probe overhead of `vector<bool>`. V2's stored probe distance
doesn't help — and on FindHit Random it's slightly slower than V1 uint8_t, possibly
because the stored byte adds a data dependency the branch predictor can't hide.

## Large scale at load 0.75

### Insert — uint8_t advantage holds at scale

| Entries | V1 uint8_t | V1 bool | V2 stored PD | uint8_t vs bool | uint8_t vs V2 |
|---|---|---|---|---|---|
| 65K | 196K ns | 319K ns | 226K ns | 1.63x | 1.15x |
| 262K | 799K ns | 1.27M ns | 902K ns | 1.59x | 1.13x |
| 2M | 6.56M ns | 10.7M ns | 7.62M ns | 1.63x | 1.16x |
| 4M | 13.4M ns | 22.5M ns | 15.1M ns | 1.68x | 1.13x |

V1 uint8_t maintains a consistent ~13% lead over V2 at all scales. Bool falls
further behind at scale (1.6-1.7x).

### FindHit — all identical at scale

| Entries | V1 uint8_t | V1 bool | V2 stored PD |
|---|---|---|---|
| 65K | 48.7K ns | 48.7K ns | 48.7K ns |
| 262K | 195K ns | 195K ns | 195K ns |
| 2M | 1.56M ns | 1.56M ns | 1.56M ns |
| 4M | 3.12M ns | 3.12M ns | 3.12M ns |

### FindHit Random — all converge at scale

| Entries | V1 uint8_t | V1 bool | V2 stored PD |
|---|---|---|---|
| 65K | 150K ns | 170K ns | 160K ns |
| 262K | 1.01M ns | 1.03M ns | 1.03M ns |
| 2M | 15.6M ns | 15.3M ns | 15.4M ns |
| 4M | 36.4M ns | 36.7M ns | 36.0M ns |

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
| FindMiss | V1 uint8_t ≈ V2 | Both ~5% over bool |
| FindHit (random/strided) | V1 uint8_t | Slight edge |
| EraseChurn | Bool | 1.5x, sub-5ns absolute |
| Large Insert | V1 uint8_t | Consistent ~13% over V2 |

The `vector<bool>` → `vector<uint8_t>` change from the previous experiment remains
the clear win. Adding stored probe distance on top was a net negative — a reminder
that optimizing away cheap operations can make things worse if the bookkeeping cost
exceeds what you save.
