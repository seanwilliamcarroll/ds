# Robin Hood V2: `vector<bool>` → `vector<uint8_t>` for Occupied State

Comparing RobinHoodHashMap (V1, `vector<bool>`) vs RobinHoodHashMapV2 (V2, `vector<uint8_t>`).
The only change is the type used to track which slots are occupied. Probe distance is still
recomputed via `get_home()` at every probe step in both versions.

Apple Silicon, P-core L2 = 16MB, 128-byte cache lines.

## What changed

V1 uses `vector<bool>`, which is bit-packed (~0.125 bytes per slot). Every probe requires
bit manipulation — shifts, masks, and potentially crossing byte boundaries.

V2 uses `vector<uint8_t>`, where each slot's occupied state is a single byte load.

## Core scenarios at 65536 entries

### Insert — V2 is 1.4–1.6x faster

| Load | V1 | V2 | Speedup |
|---|---|---|---|
| 0.5 | 255K ns | 181K ns | 1.41x |
| 0.75 | 305K ns | 194K ns | 1.58x |
| 0.9 | 330K ns | 202K ns | 1.63x |

The improvement increases with load factor. Higher load means longer probe chains,
so the per-probe overhead of bit manipulation accumulates more.

### FindHit — roughly tied

| Load | V1 | V2 | Diff |
|---|---|---|---|
| 0.5 | 49.3K ns | 48.9K ns | ~1% |
| 0.75 | 48.9K ns | 48.7K ns | ~0% |
| 0.9 | 48.9K ns | 48.7K ns | ~0% |

FindHit is dominated by key comparison, not state checks.

### FindMiss — V2 is ~5% faster

| Load | V1 | V2 | Speedup |
|---|---|---|---|
| 0.5 | 34.1K ns | 32.6K ns | 1.05x |
| 0.75 | 34.2K ns | 32.5K ns | 1.05x |
| 0.9 | 34.1K ns | 32.5K ns | 1.05x |

Consistent small improvement — FindMiss probes more empty slots, where the state
check is the only work.

### EraseAndFind — roughly tied

| Load | V1 | V2 | Diff |
|---|---|---|---|
| 0.5 | 25.2K ns | 25.7K ns | ~2% slower |
| 0.75 | 25.9K ns | 26.1K ns | ~1% slower |
| 0.9 | 25.4K ns | 25.8K ns | ~2% slower |

### EraseChurn — V1 is 1.4–1.6x faster

| Load | V1 | V2 | Regression |
|---|---|---|---|
| 0.5 | 2.73 ns | 4.31 ns | 1.58x slower |
| 0.75 | 2.71 ns | 4.17 ns | 1.54x slower |
| 0.9 | 2.77 ns | 3.85 ns | 1.39x slower |

Backward-shift deletion involves swapping `slot_state` values. `vector<bool>`'s
proxy-based swap is apparently cheaper than byte swaps in this tight loop, or the
compiler is optimizing the bit operations differently in this path. Worth investigating
further but the absolute times are tiny (2.7 ns vs 4.2 ns per operation).

## Hash quality scenarios at 65536 entries, load 0.75

| Scenario | V1 | V2 | Speedup |
|---|---|---|---|
| Insert Random | 1.09M ns | 1.10M ns | ~tied |
| FindHit Random | 174K ns | 142K ns | 1.23x |
| Insert Strided | 595K ns | 489K ns | 1.22x |
| FindHit Strided | 177K ns | 139K ns | 1.27x |

The strided/random lookup improvement is interesting — both versions recompute probe
distances identically, so this is purely from faster state checks on longer probe chains
in pathological distributions.

## Large scale at load 0.75

### Insert — V2 advantage holds at scale

| Entries | V1 | V2 | Speedup |
|---|---|---|---|
| 65K | 318K ns | 197K ns | 1.61x |
| 262K | 1.28M ns | 786K ns | 1.62x |
| 2M | 11.1M ns | 6.69M ns | 1.66x |
| 4M | 23.0M ns | 13.7M ns | 1.67x |

Crucially, V2 does not degrade at scale — unlike the AoS experiment with linear
probing. The `vector<uint8_t>` maintains SoA density (1 byte per slot in the state
array), so the cache behavior is preserved.

### FindHit — identical at all sizes

| Entries | V1 | V2 |
|---|---|---|
| 65K | 48.7K ns | 48.7K ns |
| 262K | 195K ns | 195K ns |
| 2M | 1.56M ns | 1.56M ns |
| 4M | 3.13M ns | 3.12M ns |

### FindHit Random — V2 slightly faster at small scale, tied at large

| Entries | V1 | V2 | Speedup |
|---|---|---|---|
| 65K | 169K ns | 135K ns | 1.25x |
| 262K | 1.05M ns | 1.01M ns | 1.04x |
| 2M | 15.3M ns | 15.5M ns | ~tied |
| 4M | 36.8M ns | 36.3M ns | ~tied |

At large scale with random access, both versions are dominated by cache misses.

## Summary

| Scenario type | Winner | Magnitude |
|---|---|---|
| Insert (all) | V2 | 1.4–1.67x faster, grows with load and scale |
| FindHit (sequential) | Tied | |
| FindMiss | V2 | ~5% faster |
| FindHit (random/strided) | V2 | 1.2–1.3x faster |
| EraseChurn | V1 | 1.4–1.6x slower (tiny absolute times) |

Replacing `vector<bool>` with `vector<uint8_t>` is a clear win. The insert speedup
alone justifies it. The EraseChurn regression is the only downside but involves
sub-5ns absolute times.

## Next steps

The `uint8_t` state array is currently just 0/1 (empty/occupied). This opens the door
for storing probe distance directly in the byte (0 = empty, 1+ = occupied with probe
distance value - 1, max 254). This would eliminate the `get_home()` recomputation at
every probe step — which is where most of the remaining per-probe cost is.
