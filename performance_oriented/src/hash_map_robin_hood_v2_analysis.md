# Robin Hood V2: `vector<bool>` → `vector<uint8_t>` for Occupied State

Comparing RobinHoodHashMap (V1, `vector<bool>`) vs RobinHoodHashMapV2 (V2, `vector<uint8_t>`).
The only change is the type used to track which slots are occupied. Probe distance is still
recomputed via `get_home()` at every probe step in both versions.

Apple Silicon M4, LLVM/Clang 21.1.8, -O3. P-core L2 = 16MB, 128-byte cache lines.

## What changed

V1 uses `vector<bool>`, which is bit-packed (~0.125 bytes per slot). Every probe requires
bit manipulation — shifts, masks, and potentially crossing byte boundaries.

V2 uses `vector<uint8_t>`, where each slot's occupied state is a single byte load.

## Core scenarios at 65536 entries

### Insert — V2 is 1.4–1.6x faster

| Load | V1 | V2 | Speedup |
|---|---|---|---|
| 0.5 | 265K ns | 185K ns | 1.43x |
| 0.75 | 312K ns | 197K ns | 1.58x |
| 0.9 | 338K ns | 204K ns | 1.66x |

The improvement increases with load factor. Higher load means longer probe chains,
so the per-probe overhead of bit manipulation accumulates more.

### FindHit — roughly tied

| Load | V1 | V2 | Diff |
|---|---|---|---|
| 0.5 | 48.7K ns | 48.7K ns | ~0% |
| 0.75 | 48.7K ns | 48.7K ns | ~0% |
| 0.9 | 48.7K ns | 48.7K ns | ~0% |

FindHit is dominated by key comparison, not state checks.

### FindMiss — tied

| Load | V1 | V2 | Diff |
|---|---|---|---|
| 0.5 | 32.6K ns | 32.4K ns | ~0% |
| 0.75 | 32.5K ns | 32.5K ns | ~0% |
| 0.9 | 32.5K ns | 32.5K ns | ~0% |

Under Apple Clang, V2 showed a consistent ~5% advantage here. With LLVM-21, the gap
has disappeared — both variants produce nearly identical FindMiss times. The compiler
is likely generating equivalent code for the miss path regardless of state representation.

### EraseAndFind — roughly tied

| Load | V1 | V2 | Diff |
|---|---|---|---|
| 0.5 | 25.4K ns | 25.8K ns | ~2% slower |
| 0.75 | 25.4K ns | 25.8K ns | ~2% slower |
| 0.9 | 25.4K ns | 25.8K ns | ~2% slower |

### EraseChurn — V1 is 1.4–1.6x faster

| Load | V1 | V2 | Regression |
|---|---|---|---|
| 0.5 | 2.76 ns | 4.32 ns | 1.57x slower |
| 0.75 | 2.75 ns | 4.18 ns | 1.52x slower |
| 0.9 | 2.75 ns | 3.95 ns | 1.44x slower |

Backward-shift deletion involves swapping `slot_state` values. `vector<bool>`'s
proxy-based swap is apparently cheaper than byte swaps in this tight loop, or the
compiler is optimizing the bit operations differently in this path. Worth investigating
further but the absolute times are tiny (2.7 ns vs 4.2 ns per operation).

## Hash quality scenarios at 65536 entries, load 0.75

| Scenario | V1 | V2 | Speedup |
|---|---|---|---|
| Insert Random | 1.12M ns | 1.07M ns | 1.05x |
| FindHit Random | 166K ns | 151K ns | 1.10x |
| Insert Strided | 593K ns | 492K ns | 1.21x |
| FindHit Strided | 171K ns | 139K ns | 1.23x |

The strided lookup improvement is interesting — both versions recompute probe distances
identically, so this is purely from faster state checks on longer probe chains in
pathological distributions. The random-key advantage narrowed under LLVM-21 (was 1.23x
under Apple Clang, now 1.10x).

## Large scale at load 0.75

### Insert — V2 advantage holds at scale

| Entries | V1 | V2 | Speedup |
|---|---|---|---|
| 65K | 319K ns | 197K ns | 1.62x |
| 262K | 1.28M ns | 786K ns | 1.63x |
| 2M | 11.2M ns | 6.79M ns | 1.66x |
| 4M | 23.3M ns | 13.8M ns | 1.70x |

Crucially, V2 does not degrade at scale — unlike the AoS experiment with linear
probing. The `vector<uint8_t>` maintains SoA density (1 byte per slot in the state
array), so the cache behavior is preserved.

### FindHit — identical at all sizes

| Entries | V1 | V2 |
|---|---|---|
| 65K | 49.3K ns | 48.7K ns |
| 262K | 196K ns | 195K ns |
| 2M | 1.56M ns | 1.56M ns |
| 4M | 3.16M ns | 3.12M ns |

### FindHit Random — V2 slightly faster at small scale, tied at large

| Entries | V1 | V2 | Speedup |
|---|---|---|---|
| 65K | 158K ns | 135K ns | 1.17x |
| 262K | 1.03M ns | 1.01M ns | 1.02x |
| 2M | 15.3M ns | 15.5M ns | ~tied |
| 4M | 36.7M ns | 36.6M ns | ~tied |

At large scale with random access, both versions are dominated by cache misses.

## Summary

| Scenario type | Winner | Magnitude |
|---|---|---|
| Insert (all) | V2 | 1.4–1.70x faster, grows with load and scale |
| FindHit (sequential) | Tied | |
| FindMiss | Tied | (was ~5% V2 advantage under Apple Clang) |
| FindHit (random/strided) | V2 | 1.1–1.2x faster |
| EraseChurn | V1 | 1.4–1.6x slower (tiny absolute times) |

Replacing `vector<bool>` with `vector<uint8_t>` is a clear win. The insert speedup
alone justifies it. The EraseChurn regression is the only downside but involves
sub-5ns absolute times. The FindMiss advantage that existed under Apple Clang disappeared
with LLVM-21, suggesting that result was compiler-dependent rather than fundamental.

## Next steps

The `uint8_t` state array is currently just 0/1 (empty/occupied). This opens the door
for storing probe distance directly in the byte (0 = empty, 1+ = occupied with probe
distance value - 1, max 254). This would eliminate the `get_home()` recomputation at
every probe step — which is where most of the remaining per-probe cost is.
