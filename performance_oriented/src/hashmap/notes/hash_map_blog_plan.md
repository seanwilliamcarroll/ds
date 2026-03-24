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

## Post Outlines

### Part 1: "Your Hash Map Hypotheses Are Probably Wrong"

The hook: we benchmarked with sequential keys and declared open addressing the winner.
Then we changed the key distribution and watched the winner become 11,000x slower.

**Structure — three acts:**

1. **Setup:** four implementations, five scenarios, three key distributions, predictions
2. **Act 1 — Sequential keys:** LP dominates, open addressing wins big, case closed
3. **Act 2 — Uniform keys:** the gap narrows, rankings shift, cracks appear
4. **Act 3 — Normal keys:** open addressing goes quadratic, std::unordered_map wins,
   every conclusion from Act 1 inverts
5. **Lessons:** your benchmark's key distribution IS the result; cache locality dominates
   until probes go O(N); identity hash is the root cause

**Key surprises:**
- LP is 19x faster than chaining (sequential) and 1,700x slower than std (normal)
- Chaining degrades 9.5x from sequential to normal; LP degrades 11,000x
- std::unordered_map — the punching bag — wins 3 of 4 normal scenarios
- RH's FindMiss is immune to clustering (miss probes hit the sparse tails)
- The prediction matrix was wrong in almost every interesting way

**Source material:**
- `hash_map_pattern_bench.cpp` — the benchmark code
- `hash_map_pattern_bench_design.md` — design decisions and key generation math
- `hash_map_bench_hypotheses.md` — the original predictions
- `hash_map_post1_outline.md` — detailed outline

**What readers take away:** Hash map benchmarks with sequential or uniform keys are
measuring the best case. Real workloads cluster. Identity hash + clustering + open
addressing = quadratic. The "best" hash map depends entirely on your key distribution,
which means it depends on your hash function. This sets up Part 2's exploration of
better hash functions as both a fix and a new set of tradeoffs.


### Part 2: "The Optimization That Made Everything Worse"

The hook: Post 1 ended with "the fix is a better hash function." So we tried that.
Then we tried three other optimizations. Every one that "should" help didn't.

**Content — four optimizations that failed or backfired:**

1. **AoS merge** — "colocate metadata with data for better locality"
   - The plausible argument: one cache line per probe instead of two array lookups
   - Reality: 1.3-21x regression. The hot path is state scanning, not key reading.
     128 states per cache line vs ~10 merged 12-byte slots. Probing loads key-value
     bytes just to check if a slot is empty.
   - Lesson: AoS vs SoA depends on your access pattern, not a universal rule

2. **Fibonacci hashing** — "scatter keys better, reduce clustering"
   - This was supposed to fix the Normal catastrophe from Post 1
   - Reality: 1.65-2.1x faster for pathological clustering, but 1.3-1.5x slower for
     sequential keys. At 4M sequential entries, 15x slower (destroys prefetcher)
   - The tradeoff: fixes one pathology, creates another. ~14% is pure compute cost;
     the rest is cache/memory effects from scattering what was previously sequential
   - (Opportunity: rerun pattern bench with Fibonacci hash to show the Normal fix
     and quantify the sequential regression side-by-side)

3. **Stored probe distance** — "store metadata to avoid recomputing home slot"
   - Reality: 9-17% slower on insert. get_home() with identity hash is one AND
     instruction. The bookkeeping overhead exceeds the cost of recomputation.
   - When it *would* help: expensive hash functions, partial-key schemes

4. **Prefetching** — "prefetch next probe slot to hide memory latency"
   - Reality: 0% improvement. Probe loop body is ~3-5 cycles, prefetch to DRAM takes
     100+. The prefetch hasn't completed by the time we need it.
   - What would actually work: batch prefetching (compute N home slots, prefetch all)

**Also in this post (positive results):**
- **vector\<bool\> → vector\<uint8_t\>** for Robin Hood: 1.4-1.7x faster insert
- **Pool allocator for chaining:** 3-6x faster insert, ~10x faster erase+find. Fixes
  chaining's actual bottleneck (per-node malloc, not the algorithm)

**Source material:**
- `hash_map_perf_findings.md`, `hash_map_robin_hood_v2_analysis.md`
- `hash_map_fibonacci_hash_analysis.md`, `hash_map_prefetching_analysis.md`
- `hash_map_robin_hood_three_way_analysis.md`

**What readers take away:** "Theoretically better" can be practically worse. Before
optimizing, understand: (1) how expensive is the thing you're replacing? (2) what does
the optimization cost? (3) what does the hardware already do for you?


### Part 3 (optional): "Closing the Gap to Production"

This post is forward-looking — requires additional implementation work.

**Possible content:**
- Batch prefetching (the approach that might actually work for hash maps)
- Fibonacci hash on Robin Hood (untested — how does it interact with uniform probe lengths?)
- String keys (expensive hash, expensive comparison — changes the entire tradeoff landscape)
- Pool-allocated chaining at large scale with random keys (how far does it go?)
- SIMD/Swiss table probing (the big one — how Abseil's flat_hash_map actually works)

---

## Gaps to Fill Before Writing

### Missing benchmarks / experiments

1. ~~**Final scoreboard across all implementations.**~~ **DONE.** See
   `hash_map_final_scoreboard.md`. Single-run data for all 7 implementations across all
   scenarios. New findings: ChainV2 wins EraseChurn (~2.5ns), std wins FindHit_Strided,
   LP's FindMiss is inverted (worst at low load, best at high), open addressing
   converges at 4M random access (35-37M ns). ChainV2 uses 80 bytes/entry (more than
   Chain's 64 — pool pre-allocates).

2. ~~**ChainingV2 (pool allocator) in later benchmark rounds.**~~ **DONE.** Covered in
   final scoreboard. At 4M random access, ChainV2 is 46.5M ns vs LP's 35.9M (1.3x slower).
   Pool allocator helps but can't overcome pointer chasing at scale.

3. **Fibonacci hash on Robin Hood.** Only tested on LP. Robin Hood's uniform probe lengths
   might interact differently with scattered keys. Quick experiment: template Robin Hood
   with a Fibonacci hash option and run the hash quality + large scale scenarios.

4. ~~**Memory numbers for all variants.**~~ **DONE.** Covered in final scoreboard. All
   open-addressing variants are 18 bytes/entry except RH-bool (16.2). ChainV2 is 80
   (pool pre-allocation), Chain is 64, std is 44.8.

### Missing analysis

5. **Per-post narrative drafts.** The analysis docs are reference material (tables, numbers).
   Blog posts need narrative: setup, tension, resolution. Each experiment needs a "here's what
   I expected and why" paragraph before the results.

6. **Visualizations.** The analysis docs are all tables. Blog posts need charts — bar charts
   for cross-implementation comparison, line charts for scaling behavior. Decide on tooling
   (matplotlib? observable? simple SVGs?).

7. **Code snippets for posts.** Decide which code to show. Readers don't need full
   implementations — they need the hot loop, the data layout, the key insight. Pull out
   ~10-20 line snippets that illustrate each point.

### Cleanup

8. **Consolidate/deduplicate analysis docs.** There are 10 analysis/findings markdown files.
   Some overlap (e.g. perf_findings.md and perf_improvements.md both discuss AoS/SoA). Before
   writing posts, decide which docs are source-of-truth for which data. The blog plan (this
   file) maps posts to source docs, but the source docs themselves could use dedup.

9. ~~**Verify all benchmark numbers are from the same machine/conditions.**~~ **DONE.** Final
   clean run completed 2026-03-23. Both timing and memory benchmarks from a single session.
   Raw data in `final_hashmap_bench_release.csv` and `final_hashmap_mem_bench_release.csv`.

10. **Reproducibility notes.** Document compiler version, optimization flags, and any
    machine-specific settings (e.g. process pinning, turbo boost state). Readers will want
    to know.

---

## New Findings from Final Run (2026-03-23)

Surprises not captured in earlier per-experiment analyses:

1. **std::unordered_map wins FindHit_Strided** (101K vs LP's 154K, 1.52x faster). Node-based
   layout is immune to clustering — strided keys don't cause cache-line pollution like they do
   in open addressing. This is a great counterexample for Part 1: chaining has a real advantage
   in at least one scenario.

2. **LP's FindMiss load sensitivity is inverted** — worst at low load (48.7K at 0.5) and best
   at high load (35.7K at 0.9). At low load, the table is mostly empty but LP must probe past
   tombstones from the benchmark's erase setup; at high load, entries fill the gaps and LP finds
   empty slots sooner. This inverts the naive "higher load = worse performance" expectation.
   (Note: with Apple Clang the pattern ran the other direction — compiler codegen can flip
   which effect dominates.)

3. **ChainV2 wins EraseChurn** (~2.5ns vs RH-bool's ~2.8ns). Pool allocator's push/pop on
   the free list is marginally faster than Robin Hood's backshift+insert. We previously said
   RH won EraseChurn — that was before ChainV2 existed.

4. **ChainV2 uses more memory than Chain** (80 vs 64 bytes/entry). The pool pre-allocates
   `num_buckets` nodes, which overshoots when load < 1.0. Speed costs memory. Worth noting
   in the pool allocator discussion.

5. **Open addressing fully converges at 4M random access** (LP 35.9M, RH 36.6M, RH-V2 37.1M,
   RH-bool 36.7M — all within 3%). Chaining is 1.3x behind (47.2M). std is 2.9x behind (103M).
   The probe strategy is completely irrelevant when every lookup is a DRAM miss — only data
   layout matters.

6. **RH-bool FindMiss penalty disappeared** — was ~5% slower than RH uint8_t with Apple Clang,
   now tied (~32.5K across all three RH variants). LLVM-21 generates tighter `vector<bool>`
   bit extraction code. The bool penalty is now limited to insert-heavy scenarios.
