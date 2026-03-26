# Hash Map Blog Series Plan

## The Story

A learning project that follows a repeating pattern: **hypothesize, measure, get surprised,
learn why.** The unique angle is that we wrote careful predictions *before* benchmarking —
and systematically got them wrong. Most performance blog posts present the final optimized
result. This series shows the process, including the wrong turns.

Working series title: **"Building a Hash Map from Scratch and Getting Everything Wrong"**

Apple Silicon M4, P-core L2 = 16MB, 128-byte cache lines. LLVM/Clang 21.1.8, -O3.
All implementations are `int -> int` with identity hash (`std::hash<int>` = identity on
most compilers).

## The Moral (two layers)

**Surface:** Cache locality dominates algorithmic cleverness. A flat array scanned linearly
beats a theoretically-optimal algorithm that chases pointers.

**Deeper:** Performance intuition is unreliable. Thoughtful, well-reasoned predictions were
wrong in important ways. Claude recommended AoS merge — catastrophically wrong. Stored probe
distance *should* help — doesn't. The recurring lesson is that performance is empirical, not
theoretical. The only honest approach is hypothesis, benchmark, analyze.

---

## TODO

### 1. Re-run pattern bench with fixed normal miss keys

The original `make_miss_keys` used a fixed prime offset (`+1,000,003`) for all non-sequential
patterns. For normal keys this produced wildly non-monotonic FindMiss results because
`offset mod table_size` varies with table size — for some N values the miss keys land inside
the hit cluster (68% overlap at N=4096), for others they land in empty space (0% at N=512).

Fixed: normal miss keys are now independently generated from a normal distribution centered at
`3N/2` (hit center is `N/2`), placing the miss cluster on the opposite side of the table.
Verified 0% slot overlap at all N values using `key_distribution_dump` + `plot_key_distribution.py`.
See `hash_map_key_generation.md` for details.

**Action:** rebuild release, re-run `hash_map_pattern_bench` with `--benchmark_out_format=csv`.
Only FindMiss/Normal is invalidated, but re-run everything for a clean dataset from one session.

### 2. Update post 1 with corrected numbers and observations

Replace FindMiss/Normal numbers in the post 1 outline and analysis docs. Re-check narrative
conclusions that depend on FindMiss/Normal data:

- **"RH's FindMiss is immune to clustering"** — this was a key surprise in post 1. With the
  old miss keys, some N values had 0% overlap (miss keys in empty space → fast), which looked
  like Robin Hood's early termination was the cause. With fixed miss keys consistently landing
  in empty space for all implementations, this claim might weaken or need reframing. The early
  termination story is still real, but the magnitude vs LP might change.
- **LP degradation numbers** (e.g. "11,000x slower from sequential to normal") — these came
  from FindMiss/Normal which was hitting the cluster for some N values. The actual degradation
  with properly-placed miss keys may be smaller.
- **std::unordered_map "wins 3 of 4 normal scenarios"** — check if FindMiss/Normal still
  counts as a std win or if the rankings shift.
- **LP cluster contagion section** — incorporate the finding that LP FindMiss/Normal is slow
  despite 0% home slot overlap. Displacement spreads the cluster into a contiguous occupied run;
  miss keys land inside it and probe O(cluster_size). Contrasts with RH (early termination
  escapes) and chaining (vertical collisions, immune). Probe count table is compelling. See
  `hash_map_lp_cluster_contagion.md` for full details. Already drafted into post 1 outline
  Section 5.

**Source material:**
- `hash_map_post1_outline.md` — detailed outline
- `hash_map_bench_hypotheses.md` — original predictions
- `hash_map_pattern_bench_design.md` — design decisions

### 3. Run optimization bench and analyze results

Run `hash_map_optimization_bench` (all 5 A/B pairs) with `--benchmark_out_format=csv`.
Use `scripts/optimization_bench_extract.py` to compare.

**The 5 pairs:**
1. **AoS merge** — `LinearProbingHashMap_SoA` vs `LinearProbingHashMap_MergedStruct`
2. **Stored probe distance** — `RobinHoodHashMap_RecomputedDist` vs `RobinHoodHashMap_StoredDist`
3. **Prefetching** — `LinearProbingHashMap_NoPrefetch` vs `LinearProbingHashMap_Prefetch`
4. **vector\<bool\> vs uint8_t** — `RobinHoodHashMap_BoolState` vs `RobinHoodHashMap_Uint8State`
5. **Pool allocator** — `ChainingHashMap_UniquePtr` vs `ChainingHashMap_PoolAllocator`

Previous stored dist results had the miss key aliasing bug — those numbers are now suspect.
The other 4 pairs should be unaffected (AoS, prefetch, bool use LP or don't use FindMiss/Normal;
pool uses chaining which is immune to clustering). Still worth a clean run from one session.

Also run the large-scale AoS benchmarks (64K–4M) to capture the cache-thrashing regression
that grows with scale.

### 4. Write post 2: "The Optimization That Made Everything Worse"

**Hook:** Post 1 ended with open addressing dominating sequential keys and collapsing on
clustered keys. The natural next step: optimize. We tried five things. Three made it worse.

**Optimizations that failed or backfired:**

1. **AoS merge** — "colocate metadata with data for better locality"
   - Plausible argument: one cache line per probe instead of two array lookups
   - Reality: 1.3–21x regression. The hot path is state scanning, not key reading.
     128 states per cache line vs ~10 merged 12-byte slots.
   - Lesson: AoS vs SoA depends on your access pattern, not a universal rule

2. **Stored probe distance** — "store metadata to avoid recomputing home slot"
   - Reality: 9–17% slower on insert. `get_home()` with identity hash is one AND
     instruction. Bookkeeping overhead exceeds recomputation cost.
   - Also: uint8_t probe distance overflows silently on clustered keys, corrupting
     the Robin Hood invariant. We added `std::abort()` guards after catching this.
   - When it *would* help: expensive hash functions, partial-key schemes

3. **Prefetching** — "prefetch next probe slot to hide memory latency"
   - Reality: 0% improvement. Probe loop body is ~3–5 cycles, prefetch to DRAM takes
     100+. The prefetch hasn't completed by the time we need it.
   - What would actually work: batch prefetching (compute N home slots, prefetch all)

**Optimizations that worked:**

4. **vector\<bool\> → vector\<uint8_t\>** for Robin Hood: 1.4–1.7x faster insert.
   `vector<bool>` bit-packing forces read-modify-write on every state update.

5. **Pool allocator for chaining:** 3–6x faster insert, ~10x faster erase+find.
   Fixes chaining's actual bottleneck (per-node malloc, not the algorithm).

**Fibonacci hash** is excluded from this post — it's a richer story (fixes one pathology,
creates another, 15x regression at 4M sequential) that deserves its own treatment. Could be
post 3 or a standalone piece.

**What readers take away:** "Theoretically better" can be practically worse. Before
optimizing, understand: (1) how expensive is the thing you're replacing? (2) what does
the optimization cost? (3) what does the hardware already do for you?

---

## Open / Future

These are not blocked on anything above. Pick up whenever.

- **Fibonacci hash post** — the tradeoff story: 1.65–2.1x faster for clustering, 1.3–1.5x
  slower for sequential, 15x slower at 4M. Fixes one pathology, creates another. Rerun
  pattern bench with Fibonacci hash to show the Normal fix side-by-side with the sequential
  regression.
- **Fibonacci hash on Robin Hood** — only tested on LP. Robin Hood's uniform probe lengths
  might interact differently with scattered keys.
- **Batch prefetching** — the approach that might actually work for hash maps.
- **String keys** — expensive hash, expensive comparison, changes the entire tradeoff landscape.
- **SIMD / Swiss table probing** — how Abseil's `flat_hash_map` actually works.
- **Visualizations** — the analysis docs are all tables. Blog posts need charts.
- **Code snippets** — decide which ~10–20 line hot loops to show per post.
