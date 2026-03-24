# Hash Map Benchmark Scenarios

All scenarios run across 5 implementations: Chaining, ChainingV2 (pool allocator),
Linear Probing, Robin Hood, and std::unordered_map.

## Original scenarios

Baseline comparisons with sequential keys. Run at all three load factors (0.5, 0.75, 0.9)
to measure load factor sensitivity. Range: 256 to 65536 entries.

| Scenario | What it measures | Setup | Timed work |
|---|---|---|---|
| BM_Insert | Insert throughput | Empty map | Insert N sequential keys |
| BM_FindHit | Find (all hits) | Pre-fill N sequential keys | Find all N keys in insertion order |
| BM_FindMiss | Find (all misses) | Pre-fill keys [0, N) | Find keys [N, 2N) — none exist |
| BM_EraseAndFind | Find after heavy deletion | Pre-fill N, erase even keys | Find odd keys (N/2 survivors) |
| BM_EraseChurn | Steady-state insert+erase | Pre-fill N/2 keys | Insert one new key, erase one old key per iteration |

Sequential keys are the **best case** for identity hash — they map to sequential slots,
giving cache-friendly linear access. This flatters open addressing.

## Hash quality scenarios

Tests how key distribution affects performance. Exposes weaknesses in `std::hash<int>`
(identity function). Run at load 0.75 only — the variable is key distribution, not load
factor. Range: 256 to 65536 entries.

| Scenario | Keys | Lookup order | What it measures |
|---|---|---|---|
| BM_Insert_Random | Shuffled unique keys from [0, 10N) | N/A | Insert with scattered slot access |
| BM_FindHit_Random | Same random keys | Reshuffled | Realistic lookup: random keys in random order |
| BM_Insert_Strided | Multiples of 16 (0, 16, 32, ...) | N/A | Pathological clustering for identity hash |
| BM_FindHit_Strided | Same strided keys | Insertion order | Find under severe clustering |

**Why stride 16:** with power-of-two table sizes, `hash(k) & (size - 1)` for `k = 16i`
gives `0` when `size ≤ 16`, and only 2 distinct slots when `size = 32`, etc. This creates
worst-case clustering for open addressing and long chains for chaining. A good hash
function (e.g. Fibonacci hashing) would scatter these across the table.

**Why random keys matter:** random keys break the sequential-access cache advantage that
makes open addressing look good in the baseline benchmarks. With random keys, each probe
is a potential cache miss regardless of data layout.

Note: keys are read from a pre-built vector, adding one L1-hot sequential read per
operation vs the integer counter in original benchmarks. This cost is identical across
all implementations so relative comparisons remain valid.

## Large scale scenarios

Pushes table sizes past L2 cache to expose memory latency. Run at load 0.75 only.
Range: 65536 to 4194304 entries.

| Scenario | Keys | Lookup order | What it measures |
|---|---|---|---|
| BM_Insert_Large | Sequential | N/A | Insert throughput at scale |
| BM_FindHit_Large | Sequential | Sequential | Find with sequential access (best-case prefetch pattern) |
| BM_FindHit_Random_Large | Random | Shuffled | Find with random access (worst-case, every lookup is a cache miss) |

**Target machine (Apple Silicon):**

| Cache | Size | Notes |
|---|---|---|
| P-core L1d | 128 KB | |
| P-core L2 | 16 MB | No separate L3 — this is the last level |
| E-core L1d | 64 KB | |
| E-core L2 | 4 MB | Benchmarks may run here under load |
| Cache line | 128 bytes | Double x86's 64B — ~14 LP slots per line |

**Table sizes** at ~9 bytes/slot (LP) with 0.75 load factor:

| Entries | Table slots | Table size | Fits in? |
|---|---|---|---|
| 65536 (1<<16) | ~87K | ~780 KB | P-core L2, E-core L2 |
| 262144 (1<<18) | ~350K | ~3.1 MB | P-core L2, E-core L2 |
| 1048576 (1<<20) | ~1.4M | ~12.6 MB | P-core L2 (barely) |
| 4194304 (1<<22) | ~5.6M | ~50 MB | Nothing — fully cache-cold |

Current range tops out at 1<<20, which still mostly fits in P-core L2. If results at
1<<20 don't show clear cache pressure, extending to 1<<22 would guarantee it.

The 128-byte cache line is important: each line holds ~14 LP slots, so linear probing
gets ~14 probe steps per cache fetch. This amplifies sequential access advantages
compared to x86 (which gets ~7 per line). Prefetching matters less until the table
is large enough that consecutive probes cross line boundaries frequently.

## Running subsets

```bash
# All benchmarks (slow — several minutes)
./build-release/performance_oriented/src/hash_map_bench

# Just one scenario group
./build-release/performance_oriented/src/hash_map_bench --benchmark_filter="Strided"
./build-release/performance_oriented/src/hash_map_bench --benchmark_filter="Large"
./build-release/performance_oriented/src/hash_map_bench --benchmark_filter="Random"

# Original scenarios only
./build-release/performance_oriented/src/hash_map_bench --benchmark_filter="^BM_(Insert|FindHit|FindMiss|EraseAndFind|EraseChurn)<"

# Specific implementation
./build-release/performance_oriented/src/hash_map_bench --benchmark_filter="LinearProbing"

# CSV output for analysis
./build-release/performance_oriented/src/hash_map_bench --benchmark_format=csv > results.csv
```
