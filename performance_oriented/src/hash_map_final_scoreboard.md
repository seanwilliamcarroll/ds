# Hash Map Final Scoreboard

All numbers from a single benchmark run on Apple Silicon (P-core L2 = 16MB,
128-byte cache lines). cpu_time in nanoseconds unless noted. Seven implementations,
three load factors, all scenarios.

## Implementations

| Short name | Class | Key feature |
|---|---|---|
| Chain | ChainingHashMap | Separate chaining, per-node `make_unique` |
| ChainV2 | ChainingHashMapV2 | Separate chaining, pool allocator + intrusive free list |
| LP | LinearProbingHashMap | Open addressing, tombstone deletion, SoA layout |
| RH | RobinHoodHashMap | Robin Hood insertion, backshift deletion, uint8_t state |
| RH-bool | RobinHoodHashMap\<L, true\> | Same as RH but vector\<bool\> state (bit-packed) |
| RH-V2 | RobinHoodHashMapV2 | Robin Hood with stored probe distance in uint8_t |
| std | StdUnorderedMapAdapter | std::unordered_map wrapper |

---

## Core Scenarios at N=65536

### Insert

| | Load 0.5 | Load 0.75 | Load 0.9 |
|---|---:|---:|---:|
| **LP** | **110K** | **116K** | **121K** |
| RH | 183K | 196K | 203K |
| RH-V2 | 203K | 222K | 240K |
| RH-bool | 264K | 311K | 337K |
| ChainV2 | 305K | 334K | 350K |
| std | 1.22M | 1.17M | 1.20M |
| Chain | 1.81M | 2.23M | 2.40M |

LP wins by 1.7x over RH and 19x over Chain. LP is nearly load-factor-insensitive —
resize cost is amortized. Chain degrades with load factor; open addressing doesn't.

### FindHit

| | Load 0.5 | Load 0.75 | Load 0.9 |
|---|---:|---:|---:|
| LP | 45.4K | 45.4K | 45.4K |
| ChainV2 | 46.6K | **44.0K** | 48.8K |
| RH | 48.6K | 48.7K | 48.7K |
| RH-V2 | 48.7K | 48.7K | 48.7K |
| RH-bool | 48.7K | 48.7K | 48.7K |
| Chain | 49.2K | 49.2K | 49.0K |
| std | 97.3K | 113.6K | 97.4K |

All custom implementations within ~10% of each other. Sequential key access with
identity hash is dominated by cache behavior, not probe strategy. std is 2x slower.

### FindMiss

| | Load 0.5 | Load 0.75 | Load 0.9 |
|---|---:|---:|---:|
| **RH** | **32.5K** | **32.5K** | **32.5K** |
| **RH-V2** | **32.4K** | **32.4K** | **32.6K** |
| RH-bool | 34.1K | 34.1K | 34.1K |
| LP | 35.8K | 44.7K | **48.6K** |
| Chain | 36.4K | 36.3K | 35.8K |
| ChainV2 | 36.4K | 36.5K | 35.8K |
| std | 61.0K | 86.0K | 86.1K |

Robin Hood wins via early termination. **LP degrades sharply at high load** — 35.8K at
0.5 to 48.6K at 0.9 (+36%). This is the primary clustering penalty: more occupied slots
to probe through before hitting empty. Chaining is load-insensitive (empty bucket = immediate
miss). This is the one scenario where load factor clearly matters for a custom implementation.

### EraseAndFind

| | Load 0.5 | Load 0.75 | Load 0.9 |
|---|---:|---:|---:|
| **LP** | **20.9K** | **20.9K** | **20.9K** |
| RH-bool | 25.4K | 25.3K | 25.3K |
| RH-V2 | 25.7K | 25.8K | 25.8K |
| RH | 25.8K | 25.8K | 25.8K |
| ChainV2 | 28.5K | 28.8K | 28.6K |
| Chain | 292K | 296K | 296K |
| std | 337K | 339K | 335K |

LP's flat-array scanning through tombstones is faster than Robin Hood's cleaner table.
Cache-friendly sequential access > clean probe chains. Chain/std are 14-16x slower
(pointer chasing after half the entries deleted).

### EraseChurn (ns per iteration)

| | Load 0.5 | Load 0.75 | Load 0.9 |
|---|---:|---:|---:|
| **ChainV2** | **2.6** | **2.5** | **2.5** |
| RH-bool | 2.8 | 2.7 | 2.7 |
| RH-V2 | 4.3 | 4.0 | 3.7 |
| RH | 4.4 | 4.2 | 4.0 |
| LP | 5.6 | 5.5 | 5.5 |
| Chain | 15.1 | 14.8 | 14.3 |
| std | 19.5 | 18.9 | 19.2 |

ChainV2 wins — pool allocator makes insert+erase a free-list push/pop (~2.5ns). All
absolute times are sub-6ns for the top 5, so this scenario has minimal practical
significance. RH-bool's proxy swap is slightly cheaper than RH's byte swap in this
tight loop.

---

## Hash Quality Scenarios at N=65536, Load 0.75

### Insert_Random

| Implementation | cpu_time | vs best |
|---|---:|---:|
| **LP** | **727K** | **1.00x** |
| ChainV2 | 986K | 1.36x |
| RH | 1.02M | 1.41x |
| RH-V2 | 1.03M | 1.42x |
| RH-bool | 1.11M | 1.53x |
| std | 2.04M | 2.81x |
| Chain | 4.25M | 5.84x |

### FindHit_Random

| Implementation | cpu_time | vs best |
|---|---:|---:|
| **LP** | **115K** | **1.00x** |
| RH | 155K | 1.34x |
| RH-bool | 173K | 1.50x |
| Chain | 182K | 1.58x |
| RH-V2 | 185K | 1.60x |
| ChainV2 | 190K | 1.64x |
| std | 361K | 3.13x |

LP wins random-key scenarios too. Random access breaks sequential prefetching for everyone
equally, but LP's simpler probe loop (no swap/displacement logic) gives it an edge.

### Insert_Strided

| Implementation | cpu_time | vs best |
|---|---:|---:|
| **LP** | **367K** | **1.00x** |
| RH | 491K | 1.34x |
| RH-V2 | 497K | 1.36x |
| RH-bool | 594K | 1.62x |
| ChainV2 | 653K | 1.78x |
| std | 1.19M | 3.24x |
| Chain | 2.51M | 6.85x |

### FindHit_Strided

| Implementation | cpu_time | vs best |
|---|---:|---:|
| **std** | **98.7K** | **1.00x** |
| RH-V2 | 137K | 1.39x |
| RH | 140K | 1.41x |
| LP | 154K | 1.56x |
| RH-bool | 178K | 1.80x |
| ChainV2 | 195K | 1.97x |
| Chain | 212K | 2.15x |

**Surprise: std::unordered_map wins FindHit_Strided.** Strided keys (multiples of 16) cause
catastrophic clustering for open addressing with identity hash — all keys map to the same few
slots. std's node-based layout means the clustering doesn't translate to sequential cache-line
pollution. Each node is independently allocated, so scattered keys don't drag unrelated data
into cache. This is the one scenario where chaining's pointer-based layout is an advantage.

---

## Large-Scale Scenarios (Load 0.75)

### Insert_Large

| Implementation | 64K | 256K | 2M | 4M |
|---|---:|---:|---:|---:|
| **LP** | **117K** | **456K** | **3.89M** | **7.84M** |
| RH | 197K | 797K | 6.72M | 13.6M |
| RH-V2 | 227K | 912K | 7.46M | 15.5M |
| RH-bool | 319K | 1.75M | 11.7M | 23.8M |
| ChainV2 | 339K | 1.50M | 14.3M | 28.9M |
| std | 1.18M | 4.73M | 39.1M | 80.8M |
| Chain | 2.20M | 8.99M | 74.2M | 148M |

LP's advantage is consistent at scale. Chain is 19x slower at 4M entries.

### FindHit_Large (sequential access)

| Implementation | 64K | 256K | 2M | 4M |
|---|---:|---:|---:|---:|
| **LP** | **45.4K** | **182K** | **1.46M** | **2.91M** |
| RH | 48.7K | 195K | 1.56M | 3.11M |
| RH-V2 | 48.7K | 195K | 1.56M | 3.12M |
| ChainV2 | 48.9K | 197K | 1.58M | 3.15M |
| RH-bool | 48.7K | 199K | 1.56M | 3.19M |
| Chain | 49.1K | 197K | 1.71M | 3.40M |
| std | 114K | 440K | 3.49M | 7.54M |

Custom implementations converge at scale (within ~10% at 4M). Sequential access + identity
hash = perfect prefetcher utilization. std is 2.6x slower (pointer chasing defeats prefetcher).

### FindHit_Random_Large (random access)

| Implementation | 64K | 256K | 2M | 4M |
|---|---:|---:|---:|---:|
| LP | 110K | 951K | 15.2M | 35.9M |
| RH | 138K | 996K | 15.7M | 36.3M |
| RH-bool | 158K | 1.04M | 15.4M | 36.7M |
| RH-V2 | 164K | 1.06M | 15.2M | 36.1M |
| ChainV2 | 151K | 1.02M | 21.3M | 46.5M |
| Chain | 163K | 1.05M | 20.7M | 46.6M |
| std | 356K | 1.97M | 46.6M | 103M |

**At 4M with random access, open addressing converges (35-37M).** The probe strategy is
irrelevant — every lookup is a DRAM miss. Chaining is 1.3x slower (extra pointer chase per
lookup). std is 2.8x slower. This is the regime where data layout determines everything.

---

## Memory: bytes per entry at N=65536

| Implementation | bytes/entry | Notes |
|---|---:|---|
| RH-bool | 16.2 | Bit-packed state saves ~1.8 bytes/entry |
| LP | 18.0 | uint8_t state + 8-byte KV slot |
| RH | 18.0 | Same layout as LP (uint8_t state) |
| RH-V2 | 18.0 | Same — stored PD uses same uint8_t |
| std | 44.8 | Node-based, allocator-efficient |
| Chain | 64.0 | Per-node make_unique, worst case |
| ChainV2 | 80.0 | Pool pre-allocates num_buckets nodes upfront |

ChainV2 uses more memory than Chain (80 vs 64 bytes/entry) because the pool allocator
pre-allocates a node array sized to `num_buckets`, which may overshoot. The speed gain
(3-10x) justifies the memory cost.

---

## Winner Summary

| Scenario | Winner | Runner-up | Key insight |
|---|---|---|---|
| Insert (all loads) | LP | RH (1.7x) | Flat array, no swap overhead |
| FindHit (sequential) | Tied (LP slight edge) | — | Cache behavior dominates, not algorithm |
| FindMiss | RH/RH-V2 | RH-bool (1.05x) | Early termination on miss is real |
| EraseAndFind | LP | RH-bool (1.21x) | Tombstone scan is cache-friendly enough |
| EraseChurn | ChainV2 | RH-bool (1.08x) | Pool push/pop is 2.5ns; sub-6ns for top 5 |
| Insert Random | LP | ChainV2 (1.36x) | LP's simple loop wins even without sequential locality |
| FindHit Random | LP | RH (1.34x) | Same — simpler probe, fewer instructions |
| FindHit Strided | **std** | RH-V2 (1.39x) | Node layout immune to clustering |
| Insert Large (4M) | LP | RH (1.73x) | Consistent advantage at all scales |
| FindHit Large (4M) | LP | RH/RH-V2 (1.07x) | All converge; sequential access is prefetcher-friendly |
| FindHit Random Large (4M) | LP | RH-V2 (1.01x) | Open addressing converges; DRAM misses dominate |
| Memory efficiency | RH-bool | LP/RH/RH-V2 (18B) | Bit-packing saves 1.8 bytes/entry |

### Overall ranking (general-purpose, sequential keys)

1. **LP** — fastest in most scenarios, simplest implementation
2. **RH (uint8_t)** — wins FindMiss, competitive everywhere else, no tombstone accumulation
3. **RH-V2 (stored PD)** — no scenario where it beats RH; stored PD is dead weight with identity hash
4. **RH-bool** — memory-efficient but 1.4-1.7x slower on insert than RH
5. **ChainV2** — pool allocator makes chaining viable; wins EraseChurn
6. **std::unordered_map** — wins FindHit_Strided; 2-3x slower everywhere else
7. **Chain** — allocation bottleneck makes it 10-19x slower in key scenarios

### When each implementation is the right choice

- **LP:** you control key distribution or keys are naturally sequential/dense
- **RH:** general-purpose, miss-heavy workloads, or you want no tombstone accumulation
- **ChainV2:** erase-heavy churn with pool allocation, or you need stable pointers
- **std::unordered_map:** pathological key distributions (strided), or you need the standard API
- **RH-bool:** extreme memory pressure where 1.8 bytes/entry matters
