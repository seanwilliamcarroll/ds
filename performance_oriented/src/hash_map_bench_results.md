# Hash Map Benchmark Results

Analysis of benchmark results against predictions in `hash_map_bench_hypotheses.md`.
All numbers are cpu_time in nanoseconds at N=65536 unless noted.

## Insert

| Load | Chaining | Linear Probing | Robin Hood | std::unordered_map |
|------|----------|---------------|------------|-------------------|
| 0.5 | 1,778,280 | **126,675** | 261,192 | 1,215,470 |
| 0.75 | 2,215,800 | **131,962** | 308,264 | 1,159,260 |
| 0.9 | 2,429,540 | **128,692** | 335,820 | 1,191,820 |

**Hypothesis said:** LP fast, RH medium, Chaining medium, std slowest.

**What happened:** LP wins by a massive margin (~14x faster than chaining). RH is second
(~2.5x slower than LP). But our chaining is actually **slower than std::unordered_map** —
we predicted std would be slowest due to "extra generality overhead," but its allocator is
better optimized than our raw `make_unique` per node.

LP's insert cost is nearly identical across load factors. This makes sense — the load
factor only controls resize frequency, and at N=65536 the amortized resize cost is small.

## Find Hit

| Load | Chaining | Linear Probing | Robin Hood | std::unordered_map |
|------|----------|---------------|------------|-------------------|
| 0.5 | 49,026 | **45,426** | 48,672 | 97,421 |
| 0.75 | 49,112 | **45,428** | 48,677 | 113,404 |
| 0.9 | 49,117 | **45,464** | 48,677 | 113,552 |

**Hypothesis said:** RH fast (early termination + uniform probes), LP medium, Chaining slow.
At 0.9 we predicted LP "slow — primary clustering is brutal" and chaining closing the gap.

**What happened:** All three custom implementations are within ~8% of each other. LP has a
slight edge. RH's early termination doesn't help for hits — you still have to find the
key regardless. Load factor has essentially zero effect on our implementations.

The load factor insensitivity is the biggest surprise. At N=65536 the table doesn't fit in
L1/L2 cache regardless of fill level, so the dominant cost is cache misses per lookup, not
probe chain length. Our entire "Load 0.9" section predicted dramatic degradation for open
addressing — it didn't materialize.

## Find Miss

| Load | Chaining | Linear Probing | Robin Hood | std::unordered_map |
|------|----------|---------------|------------|-------------------|
| 0.5 | 35,245 | 41,120 | **34,070** | 48,714 |
| 0.75 | 36,234 | 37,206 | **34,104** | 86,137 |
| 0.9 | 35,305 | 37,811 | **34,106** | 86,333 |

**Hypothesis said:** RH fast, LP medium, Chaining slow (at all loads).

**What happened:** RH does win as predicted — early termination is genuinely helpful for
misses. But we had the chaining/LP ranking **backwards**: chaining is second, LP is third.
An empty bucket in chaining is an immediate "not found" with no probe. LP has to walk
through clusters before hitting an empty slot, which is slower.

std::unordered_map degrades sharply from load 0.5 (48k) to 0.75 (86k). Our implementations
barely budge.

## Erase + Find Survivors

| Load | Chaining | Linear Probing | Robin Hood | std::unordered_map |
|------|----------|---------------|------------|-------------------|
| 0.5 | 289,936 | **20,872** | 25,376 | 338,950 |
| 0.75 | 288,903 | **20,886** | 25,376 | 339,195 |
| 0.9 | 293,879 | **20,937** | 25,390 | 350,679 |

**Hypothesis said:** LP slow (tombstones degrade find), RH fast (backshift keeps chains
clean), Chaining medium.

**What happened:** LP is **fastest**, 14x faster than chaining. RH is close behind. The
hypothesis dramatically overstated tombstone damage — the cache-friendliness of a flat
array scanning through tombstones easily beats pointer-chasing through a linked list,
even when half the slots are tombstones.

This is the clearest result in the entire benchmark: **cache locality >> algorithmic
elegance**. Tombstone probing through contiguous memory is cheap. Pointer chasing through
scattered heap allocations is expensive. The algorithmic argument (tombstones degrade
probe chains) is correct in theory but irrelevant at this scale because the constant
factor difference between a cache hit and a cache miss dwarfs it.

## Erase-Heavy Churn

| Load | Chaining | Linear Probing | Robin Hood | std::unordered_map |
|------|----------|---------------|------------|-------------------|
| 0.5 | 14.39 | 5.41 | **2.74** | 19.09 |
| 0.75 | 14.89 | 5.67 | **2.92** | 21.13 |
| 0.9 | 15.04 | 5.47 | **2.99** | 19.27 |

(cpu_time in ns **per iteration**, not per N elements)

**Hypothesis said:** LP fast (O(1) tombstone flip), RH slow (O(k) backshift per erase),
Chaining medium. This was supposed to be the scenario where Robin Hood suffers.

**What happened:** Robin Hood is **2x faster than LP** and the fastest overall. This is our
biggest prediction miss.

Why backshift wins in steady-state churn: Robin Hood keeps probe distances uniform and
short, so backshift after erase typically moves 0-2 elements. The table stays clean — no
tombstones means the paired insert also has short probes. LP's erase is O(1) (flag flip),
but tombstones accumulate between rehashes. Each insert must probe past tombstones to
verify the key doesn't already exist, even when reusing the first tombstone slot. The
cheap erase shifts cost to the insert half of the churn cycle.

## Overarching Lessons

**Cache locality dominates everything at scale.** The difference between flat-array
probing and pointer-chasing is 10-14x in erase+find. Algorithmic refinements (tombstones
vs backshift, early termination, probe variance) produce 1.2-2x differences within the
open addressing family. The data layout choice (open addressing vs chaining) is the
decision that matters most.

**Load factor sensitivity was overstated.** We built three full prediction matrices for
loads 0.5/0.75/0.9, but the custom implementations are nearly identical across all three.
At N=65536, cache miss cost dominates probe length. Load factor would matter more at
sizes that fit in L2 (N < ~4096) where extra probes are cheap.

**Our chaining implementation is worse than std::unordered_map.** The hypothesis assumed
std would always be slowest due to generality overhead. In practice, std's allocator
(likely pooling or arena allocation) beats our naive `make_unique` per node. Chaining's
problem isn't the algorithm — it's the allocation strategy.

**Robin Hood has no bad scenario in this benchmark.** We predicted erase churn would
expose its weakness. Instead, backshift's table-cleaning effect makes subsequent operations
cheaper, turning a theoretical disadvantage into a practical advantage. To actually hurt
Robin Hood, you'd likely need adversarial hash distributions that create long displacement
chains, or table sizes small enough that the per-probe swap overhead matters relative to
cache-hit probes.
