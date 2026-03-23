# Hash Map Blog Series Plan

## The Story

A learning project that follows a repeating pattern: **hypothesize, measure, get surprised,
learn why.** The unique angle is that we wrote careful predictions *before* benchmarking —
and systematically got them wrong. Most performance blog posts present the final optimized
result. This series shows the process, including the wrong turns.

Working series title: **"Building a Hash Map from Scratch and Getting Everything Wrong"**

Apple Silicon (M-series), P-core L2 = 16MB, 128-byte cache lines. All implementations are
`int -> int` with identity hash (`std::hash<int>` = identity on most compilers).

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

The hook: we wrote a prediction matrix before benchmarking, and it was wrong in interesting ways.

**Content:**
- Brief intro to the four implementations: chaining, linear probing (tombstones), Robin Hood
  (backshift deletion), std::unordered_map
- The prediction matrix (include it verbatim — readers form their own predictions)
- 5 benchmark scenarios at 3 load factors (insert, find hit, find miss, erase+find, erase churn)
- Systematic comparison: prediction vs reality, scenario by scenario
- Key surprises:
  - Load factor sensitivity was overstated (cache misses dominate probe length at 65K entries)
  - Robin Hood was supposed to suffer on erase churn — it was fastest (backshift cleans the table)
  - Our chaining was slower than std::unordered_map (allocation strategy, not algorithm)
  - "Tombstones degrade probing" is correct in theory, irrelevant vs pointer chasing
- Lesson: cache locality > algorithmic elegance, and measure don't assume

**Source material:**
- `hash_map_bench_hypotheses.md` — the predictions
- `hash_map_bench_results.md` — the analysis
- `hash_map_bench_scenarios.md` — scenario descriptions

**What readers take away:** The mental model most engineers have about hash map performance
(load factor is critical, tombstones are expensive, Robin Hood's swaps are costly) is
incomplete. The constant factor difference between cache hits and cache misses dwarfs
algorithmic differences.


### Part 2: "The Optimization That Made Everything Worse"

The hook: "I merged the arrays for better locality and it got 21x slower."

**Content:**
- AoS vs SoA for open addressing slots
  - The plausible argument: colocate metadata with key-value data, one cache line per probe
  - The reality: 1.3-21x regression, getting worse at scale
  - Why: the hot path is state scanning, not key reading. 128 states per cache line vs ~10
    merged 12-byte slots. Probing loads key-value bytes just to check if a slot is empty.
  - FindMiss hit hardest (2x at 65K) — never reads keys at all, every cache line 83% waste
- vector<bool> vs vector<uint8_t> for Robin Hood occupied state
  - Bit-packed saves memory (16.25 vs ~18 bytes/entry) but every probe requires bit manipulation
  - Switching to uint8_t: 1.4-1.7x faster on insert, grows with load factor and scale
  - The one regression: EraseChurn (vector<bool> proxy swap cheaper), but sub-5ns absolute
- Pool allocator for chaining
  - The right fix for the right problem: chaining's bottleneck was per-node malloc, not the algorithm
  - Intrusive free list: zero extra memory, 3-6x faster insert, ~10x faster erase+find
  - Makes chaining competitive with open addressing
- Lesson: understand which fields your hot path actually reads. AoS vs SoA is not a universal
  rule — it depends on your access pattern.

**Source material:**
- `hash_map_perf_findings.md` — AoS/SoA, Fibonacci hash, pool allocator findings
- `hash_map_robin_hood_v2_analysis.md` — vector<bool> vs vector<uint8_t>
- `hash_map_memory_analysis.md` — memory per entry numbers

**What readers take away:** The textbook advice ("merge for locality") assumes every probe
needs all fields. When the hot path only reads metadata, keeping it dense and separate wins.
Also: before optimizing an algorithm, check if the real bottleneck is something mundane
like allocation strategy.


### Part 3: "Three Things That Should Have Helped But Didn't"

The hook: each optimization has a plausible "this should be faster" argument that fails.

**Content:**
- Fibonacci hashing (tradeoff, not a win)
  - Hypothesis: scatter keys better, reduce clustering
  - Reality: 1.65-2.1x faster for pathological strided keys, but 1.3-1.5x slower for
    sequential keys. At 4M entries with sequential keys, 15x slower (destroys prefetcher-
    friendly access pattern)
  - Isolating compute vs memory: ~14% is the multiply+shift cost (measured at L1-resident
    sizes where every access is a cache hit regardless of pattern). Everything above that
    is cache/memory effects.
  - Random-key gap stays flat at ~7% regardless of scale — confirms it's a memory pattern
    issue, not a compute issue
- Prefetching (loop too tight, hardware already does it)
  - Hypothesis: prefetch next probe slot to hide memory latency
  - Reality: 0% improvement on most scenarios, even at 4M entries (fully cache-cold)
  - Why: probe loop body is ~3-5 cycles. Prefetch to DRAM takes 100+ cycles. The prefetch
    for slot i+1 hasn't completed by the time we get there. Prefetching further ahead wastes
    effort since average probe chain is ~2.5 slots.
  - Hardware prefetcher already handles sequential access (identity hash + sequential keys)
  - What would actually work: batch prefetching (compute N home slots, prefetch all, process)
- Stored probe distance (optimizing away a 1-cycle operation)
  - Hypothesis: store probe distance in the state byte, eliminate get_home() recomputation
  - Reality: ~13% slower on insert at all scales, tied or worse everywhere else
  - Why: get_home() with identity hash is one AND instruction. Storing probe distance adds
    bookkeeping during Robin Hood swaps (update both displaced and displacing elements,
    encode/decode with +1 offset). The overhead exceeds the cost of a single AND.
  - When it would help: expensive hash functions (Fibonacci multiply+shift, cryptographic),
    small L1-resident tables, partial-key schemes where you can't recompute home
- Lesson: don't optimize what's already cheap. Understand what your hardware does for free
  (prefetcher, single-cycle operations). The cost of bookkeeping can exceed the cost of
  recomputation.

**Source material:**
- `hash_map_fibonacci_hash_analysis.md` — full Fibonacci comparison
- `hash_map_prefetching_analysis.md` — prefetching results
- `hash_map_robin_hood_three_way_analysis.md` — three-way Robin Hood comparison

**What readers take away:** "Theoretically better" can be practically worse. Before optimizing,
understand: (1) how expensive is the thing you're replacing? (2) what does the optimization
cost? (3) what does the hardware already do for you?


### Part 4 (optional): "Closing the Gap to Production"

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
   LP's FindMiss degrades 36% from load 0.5 to 0.9 (primary clustering), open addressing
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

1. **std::unordered_map wins FindHit_Strided** (98.7K vs LP's 154K, 1.56x faster). Node-based
   layout is immune to clustering — strided keys don't cause cache-line pollution like they do
   in open addressing. This is a great counterexample for Part 1: chaining has a real advantage
   in at least one scenario.

2. **LP's FindMiss degrades 36% from load 0.5 to 0.9** (35.8K → 48.6K). This is the primary
   clustering penalty we predicted in the hypotheses but didn't see in other scenarios. FindMiss
   is the scenario that exposes it because you must probe until empty — more occupied slots =
   longer probe runs. Good data point for the "load factor matters... sometimes" narrative.

3. **ChainV2 wins EraseChurn** (~2.5ns vs RH-bool's ~2.7ns). Pool allocator's push/pop on
   the free list is marginally faster than Robin Hood's backshift+insert. We previously said
   RH won EraseChurn — that was before ChainV2 existed.

4. **ChainV2 uses more memory than Chain** (80 vs 64 bytes/entry). The pool pre-allocates
   `num_buckets` nodes, which overshoots when load < 1.0. Speed costs memory. Worth noting
   in the pool allocator discussion.

5. **Open addressing fully converges at 4M random access** (LP 35.9M, RH 36.3M, RH-V2 36.1M,
   RH-bool 36.7M — all within 2%). Chaining is 1.3x behind (46.5M). std is 2.8x behind (103M).
   The probe strategy is completely irrelevant when every lookup is a DRAM miss — only data
   layout matters.
