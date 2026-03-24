# Pattern Benchmark Design

Replaces the original benchmark as the basis for blog post 1. The original bench
used sequential keys exclusively and varied load factor — this bench holds load
factor constant (0.75) and varies key distribution instead.

## Motivation

The original benchmark's sequential keys with identity hash are the best case for
open addressing: keys map to consecutive slots, giving cache-friendly linear access.
This flatters LP and RH, and the results don't generalize to realistic workloads
where keys aren't conveniently sequential.

Key distribution is the more interesting variable. Load factor sensitivity turned
out to be a second-order effect at N=65536 (cache misses dominate probe length),
so we dropped it as a variable.

## Key distributions

Three patterns, all generating N unique `int` keys:

### Sequential

Keys: 0, 1, 2, ..., N-1. Identity hash maps these to consecutive slots.
Best-case cache behavior for open addressing — the hardware prefetcher handles
this perfectly.

### Uniform random

Keys: N unique values drawn uniformly from [0, 10N) via shuffle-and-take. Identity
hash scatters these across the table. This is the closest to "realistic" — each
lookup is a potential cache miss regardless of data layout.

### Normal (clustered)

Keys: `normal_part + X * (1 << 20)`, where:
- `normal_part` ~ N(N/2, N/8), rounded to int — determines the hash slot
- `X` ~ Uniform[0, 2000) — spreads keys in value space to ensure uniqueness

The trick: `1 << 20` is a multiple of any power-of-two table size up to 2^20, so
`X * (1 << 20)` vanishes in the slot computation `key & (num_buckets - 1)`. All
keys with the same `normal_part` map to the same slot. The X term only exists to
prevent duplicate key values.

With σ = N/8, ~68% of keys cluster into ~N/4 consecutive slots around N/2. This
creates moderate-to-severe clustering for open addressing — a dense core with
lighter tails. More realistic than strided keys (which are pathological) but much
worse than uniform.

**Overflow guard:** max key ≈ 2000 * (1<<20) + N ≈ 2.1 billion, under INT_MAX.

## Scenarios

Five scenarios, same operations as the original bench:

| Scenario | Setup (untimed) | Timed work |
|---|---|---|
| Insert | Empty map | Insert N keys |
| FindHit | Pre-fill N keys | Find all N keys |
| FindMiss | Pre-fill N keys | Find N disjoint keys |
| EraseAndFind | Pre-fill N, erase half | Find N/2 survivors |
| EraseChurn | Pre-fill N/2 | Insert one + erase one, repeat |

### Design decisions

**Lookup order matches distribution.** Sequential keys are looked up in insertion
order (0, 1, 2, ...) so the access pattern is sequential. Uniform/normal keys are
looked up in insertion order too — since they were generated in random/clustered
order, the access pattern naturally reflects the distribution. No artificial shuffle.

This matters because the original bench's sequential FindHit (45K ns) vs our uniform
FindHit measures two independent effects: key distribution and access pattern. By
keeping lookup order = insertion order for all patterns, we isolate the key
distribution effect.

**Miss keys use a prime offset.** `miss_key = hit_key + 1000003`. A prime can't be
a multiple of any power-of-two table size, so miss keys land in nearby but distinct
slots. This means miss probes encounter similar clustering as hit probes (realistic)
without aliasing to the exact same slots (which would create pathological O(N)
probes for LP — we learned this the hard way when using `N * 20` as the offset,
which was a multiple of `num_buckets`).

**EraseChurn uses pre-generated keys.** We generate N keys, pre-fill with the first
N/2, then churn through the second half. On wrap (if Google Benchmark runs more
iterations than N/2), both insert and erase indices reset together so the map
always has ~N/2 entries.

## Implementations

The original four, all at load factor 0.75:

| Short name | Class |
|---|---|
| Chain | ChainingHashMap — separate chaining, per-node make_unique |
| LP | LinearProbingHashMap — tombstone deletion, SoA layout |
| RH | RobinHoodHashMap — backshift deletion, vector\<bool\> state |
| std | StdUnorderedMapAdapter — std::unordered_map wrapper |

## Running

```bash
cmake --preset release && cmake --build --preset release --target hash_map_pattern_bench

# Full suite (~3-5 minutes)
./build-release/performance_oriented/src/hash_map_pattern_bench \
    --benchmark_format=csv > pattern_bench.csv

# Extract tables
python3 scripts/pattern_bench_extract.py pattern_bench.csv

# Single pattern
./build-release/performance_oriented/src/hash_map_pattern_bench \
    --benchmark_filter="SEQUENTIAL"

# Single scenario
./build-release/performance_oriented/src/hash_map_pattern_bench \
    --benchmark_filter="BM_Insert"
```

## What to watch for

- **How much does uniform hurt open addressing?** Sequential is the best case.
  Uniform breaks the prefetcher. The gap between sequential and uniform for LP/RH
  vs Chain/std tells us how much of LP's advantage was cache flattery.

- **Does normal clustering destroy open addressing?** Identity hash + clustered
  keys should create long probe chains for LP and RH. Chaining should degrade
  more gracefully (longer chains but no cascading clustering). If the normal
  results are catastrophic (100x+), that's a strong argument for better hash
  functions or different data structures under non-uniform workloads.

- **Does the ranking change across patterns?** If LP wins with sequential keys
  but loses with uniform/normal, the "LP is best" conclusion from the original
  bench was an artifact of the test setup.
