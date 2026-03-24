# Hash Map Final Scoreboard

All numbers from a single benchmark run on Apple Silicon M4 (P-core L2 = 16MB,
128-byte cache lines) using LLVM/Clang 21.1.8, -O3. cpu_time in nanoseconds
unless noted. Seven implementations, three load factors, all scenarios.

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
| **LP** | **112K** | **117K** | **125K** |
| RH | 185K | 197K | 204K |
| RH-V2 | 201K | 224K | 239K |
| RH-bool | 265K | 312K | 338K |
| ChainV2 | 308K | 339K | 355K |
| std | 1.20M | 1.15M | 1.20M |
| Chain | 1.84M | 2.27M | 2.54M |

LP wins by 1.7x over RH and 19x over Chain. LP is nearly load-factor-insensitive —
resize cost is amortized. Chain degrades with load factor; open addressing doesn't.

### FindHit

| | Load 0.5 | Load 0.75 | Load 0.9 |
|---|---:|---:|---:|
| **LP** | **45.4K** | **45.4K** | **45.4K** |
| ChainV2 | 46.6K | 46.3K | 46.6K |
| RH | 48.7K | 48.7K | 48.7K |
| RH-V2 | 48.7K | 48.7K | 48.7K |
| RH-bool | 48.7K | 48.7K | 48.7K |
| Chain | 49.1K | 49.0K | 49.0K |
| std | 97.3K | 99.0K | 102K |

All custom implementations within ~10% of each other. Sequential key access with
identity hash is dominated by cache behavior, not probe strategy. std is ~2x slower.

### FindMiss

| | Load 0.5 | Load 0.75 | Load 0.9 |
|---|---:|---:|---:|
| **RH** | **32.4K** | **32.5K** | **32.5K** |
| **RH-V2** | **32.5K** | **32.5K** | **32.5K** |
| **RH-bool** | **32.6K** | **32.5K** | **32.5K** |
| ChainV2 | 35.2K | 36.1K | 36.2K |
| Chain | 36.1K | 35.7K | 36.3K |
| LP | 48.7K | 38.4K | 35.7K |
| std | 48.7K | 76.7K | 80.2K |

Robin Hood wins via early termination — all three RH variants are tied. LP's FindMiss
behavior is unusual: worst at low load (48.7K at 0.5) and best at high load (35.7K at
0.9). At low load, the table is mostly empty but LP must still probe past tombstones
from the erase phase of the BM_FindMiss setup; at high load, more entries fill the gaps
and LP finds empty slots sooner via sequential scan.

### EraseAndFind

| | Load 0.5 | Load 0.75 | Load 0.9 |
|---|---:|---:|---:|
| **LP** | **20.9K** | **20.9K** | **20.9K** |
| RH-bool | 25.4K | 25.4K | 25.4K |
| RH-V2 | 25.8K | 25.8K | 25.8K |
| RH | 25.8K | 25.8K | 25.8K |
| ChainV2 | 28.5K | 28.7K | 28.7K |
| Chain | 301K | 312K | 312K |
| std | 358K | 347K | 347K |

LP's flat-array scanning through tombstones is faster than Robin Hood's cleaner table.
Cache-friendly sequential access > clean probe chains. Chain/std are 15-17x slower
(pointer chasing after half the entries deleted).

### EraseChurn (ns per iteration)

| | Load 0.5 | Load 0.75 | Load 0.9 |
|---|---:|---:|---:|
| **ChainV2** | **2.5** | **2.6** | **2.5** |
| RH-bool | 2.8 | 2.8 | 2.8 |
| RH-V2 | 4.4 | 4.2 | 4.0 |
| RH | 4.4 | 4.3 | 4.0 |
| LP | 5.7 | 5.6 | 5.6 |
| Chain | 14.9 | 15.1 | 14.9 |
| std | 19.0 | 19.1 | 18.7 |

ChainV2 wins — pool allocator makes insert+erase a free-list push/pop (~2.5ns). All
absolute times are sub-6ns for the top 5, so this scenario has minimal practical
significance. RH-bool's proxy swap is slightly cheaper than RH's byte swap in this
tight loop.

---

## Hash Quality Scenarios at N=65536, Load 0.75

### Insert_Random

| Implementation | cpu_time | vs best |
|---|---:|---:|
| **LP** | **704K** | **1.00x** |
| ChainV2 | 1.04M | 1.48x |
| RH | 1.07M | 1.52x |
| RH-V2 | 1.08M | 1.53x |
| RH-bool | 1.12M | 1.59x |
| std | 2.05M | 2.91x |
| Chain | 4.26M | 6.06x |

### FindHit_Random

| Implementation | cpu_time | vs best |
|---|---:|---:|
| **LP** | **119K** | **1.00x** |
| RH | 151K | 1.27x |
| RH-bool | 166K | 1.40x |
| RH-V2 | 184K | 1.54x |
| Chain | 185K | 1.55x |
| ChainV2 | 196K | 1.65x |
| std | 356K | 3.00x |

LP wins random-key scenarios too. Random access breaks sequential prefetching for everyone
equally, but LP's simpler probe loop (no swap/displacement logic) gives it an edge.

### Insert_Strided

| Implementation | cpu_time | vs best |
|---|---:|---:|
| **LP** | **365K** | **1.00x** |
| RH | 492K | 1.35x |
| RH-V2 | 497K | 1.36x |
| RH-bool | 593K | 1.62x |
| ChainV2 | 648K | 1.77x |
| std | 1.25M | 3.43x |
| Chain | 2.64M | 7.23x |

### FindHit_Strided

| Implementation | cpu_time | vs best |
|---|---:|---:|
| **std** | **101K** | **1.00x** |
| RH-V2 | 138K | 1.37x |
| RH | 139K | 1.38x |
| LP | 154K | 1.52x |
| RH-bool | 171K | 1.70x |
| ChainV2 | 189K | 1.88x |
| Chain | 211K | 2.09x |

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
| **LP** | **118K** | **463K** | **3.92M** | **7.85M** |
| RH | 197K | 786K | 6.79M | 13.8M |
| RH-V2 | 228K | 895K | 7.60M | 14.7M |
| RH-bool | 319K | 1.28M | 11.3M | 23.3M |
| ChainV2 | 339K | 1.52M | 14.4M | 28.9M |
| std | 1.22M | 4.93M | 40.8M | 84.1M |
| Chain | 2.34M | 9.42M | 78.4M | 156M |

LP's advantage is consistent at scale. Chain is 20x slower at 4M entries.

### FindHit_Large (sequential access)

| Implementation | 64K | 256K | 2M | 4M |
|---|---:|---:|---:|---:|
| **LP** | **45.4K** | **182K** | **1.46M** | **2.91M** |
| ChainV2 | 44.4K | 186K | 1.48M | 3.13M |
| RH | 48.7K | 195K | 1.56M | 3.12M |
| RH-V2 | 48.7K | 195K | 1.56M | 3.12M |
| RH-bool | 49.3K | 196K | 1.56M | 3.16M |
| Chain | 49.3K | 196K | 1.69M | 3.42M |
| std | 97.6K | 390K | 3.16M | 6.83M |

Custom implementations converge at scale (within ~10% at 4M). Sequential access + identity
hash = perfect prefetcher utilization. std is 2.3x slower (pointer chasing defeats prefetcher).

### FindHit_Random_Large (random access)

| Implementation | 64K | 256K | 2M | 4M |
|---|---:|---:|---:|---:|
| LP | 101K | 941K | 14.9M | 35.9M |
| RH | 135K | 1.01M | 15.5M | 36.6M |
| RH-bool | 158K | 1.03M | 15.3M | 36.7M |
| RH-V2 | 180K | 1.08M | 15.8M | 37.1M |
| ChainV2 | 202K | 1.01M | 21.4M | 47.2M |
| Chain | 170K | 1.01M | 21.2M | 47.2M |
| std | 373K | 2.05M | 47.5M | 103M |

**At 4M with random access, open addressing converges (35-37M).** The probe strategy is
irrelevant — every lookup is a DRAM miss. Chaining is 1.3x slower (extra pointer chase per
lookup). std is 2.9x slower. This is the regime where data layout determines everything.

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
| FindMiss | RH (all variants tied) | Chain/ChainV2 (~1.10x) | Early termination on miss is real |
| EraseAndFind | LP | RH-bool (1.21x) | Tombstone scan is cache-friendly enough |
| EraseChurn | ChainV2 | RH-bool (1.08x) | Pool push/pop is 2.5ns; sub-6ns for top 5 |
| Insert Random | LP | ChainV2 (1.48x) | LP's simple loop wins even without sequential locality |
| FindHit Random | LP | RH (1.27x) | Same — simpler probe, fewer instructions |
| FindHit Strided | **std** | RH-V2 (1.37x) | Node layout immune to clustering |
| Insert Large (4M) | LP | RH (1.76x) | Consistent advantage at all scales |
| FindHit Large (4M) | LP | RH/RH-V2 (1.07x) | All converge; sequential access is prefetcher-friendly |
| FindHit Random Large (4M) | LP | RH (1.02x) | Open addressing converges; DRAM misses dominate |
| Memory efficiency | RH-bool | LP/RH/RH-V2 (18B) | Bit-packing saves 1.8 bytes/entry |

### Overall ranking (general-purpose, sequential keys)

1. **LP** — fastest in most scenarios, simplest implementation
2. **RH (uint8_t)** — wins FindMiss, competitive everywhere else, no tombstone accumulation
3. **RH-V2 (stored PD)** — no scenario where it beats RH; stored PD is dead weight with identity hash
4. **RH-bool** — memory-efficient, tied with RH on FindMiss, but 1.4-1.7x slower on insert
5. **ChainV2** — pool allocator makes chaining viable; wins EraseChurn
6. **std::unordered_map** — wins FindHit_Strided; 2-3x slower everywhere else
7. **Chain** — allocation bottleneck makes it 15-20x slower in key scenarios

### When each implementation is the right choice

- **LP:** you control key distribution or keys are naturally sequential/dense
- **RH:** general-purpose, miss-heavy workloads, or you want no tombstone accumulation
- **ChainV2:** erase-heavy churn with pool allocation, or you need stable pointers
- **std::unordered_map:** pathological key distributions (strided), or you need the standard API
- **RH-bool:** extreme memory pressure where 1.8 bytes/entry matters
