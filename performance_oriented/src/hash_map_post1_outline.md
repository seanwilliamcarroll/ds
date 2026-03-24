# Post 1: "Your Hash Map Hypotheses Are Probably Wrong"

## Hook

We wrote detailed predictions for 5 scenarios x 3 load factors before running any
benchmarks. The predictions were thoughtful, well-reasoned, and wrong in almost every
way that mattered. This post walks through the predictions and the data, scenario by
scenario.

(Invite the reader to form their own predictions before scrolling past the results.)

---

## Section 1: The Setup

### The four implementations

Brief descriptions — just enough for readers to reason about tradeoffs:

1. **Chaining** — separate chaining with linked list per bucket, `make_unique` per node
2. **Linear Probing** — open addressing, tombstone deletion, SoA layout (separate
   state array + key-value array)
3. **Robin Hood** — open addressing, Robin Hood insertion (swap with shorter probe
   distance), backshift deletion (no tombstones), SoA layout, `vector<bool>` state
4. **std::unordered_map** — stdlib wrapper, included as a baseline

Key design choice to call out: SoA layout. State array is separate from key-value
array. This will matter. (Don't explain why yet — let the data reveal it.)

### The benchmark scenarios

Brief table of the 5 original scenarios (Insert, FindHit, FindMiss, EraseAndFind,
EraseChurn) with one-line descriptions. Link to or footnote the full scenario
descriptions for interested readers.

Mention: sequential keys with identity hash (`std::hash<int>` = identity). This is
the best case for open addressing. We'll break this assumption in later posts.

### The machine

One line: Apple Silicon M4, LLVM/Clang 21, -O3. Link to environment doc for details.

---

## Section 2: The Predictions

Present the prediction matrices verbatim (from `hash_map_bench_hypotheses.md`), or a
cleaned-up version. Three tables (load 0.5, 0.75, 0.9) with fast/medium/slow ratings
and the short reasoning.

Also present the "Key Predictions" summary — the overarching beliefs:
- Load 0.5: LP wins everything
- Load 0.75: RH starts pulling ahead on finds
- Load 0.9: Chaining becomes competitive, LP's clustering is devastating
- std always slowest
- Erase churn: LP's O(1) tombstone beats RH's O(k) backshift

End with: "Let's see what actually happened."

---

## Section 3: The Results (scenario by scenario)

For each scenario: show the table, then "what we predicted" vs "what happened" with
analysis. Build toward the overarching lesson rather than revealing it upfront.

### Insert — LP wins, but the gap is wrong

- Prediction: LP fast, RH medium, Chaining medium, std slowest.
- Reality: LP wins by **16x** over chaining, not 2-3x. RH second at 1.7x.
- Surprise: our chaining is slower than std (we predicted the opposite).
  - Why: std's allocator is better than our `make_unique` per node.
  - Lesson preview: allocation strategy matters more than algorithm for chaining.
- Load factor barely matters for any custom implementation.

### FindHit — the "load factor doesn't matter" shock

- Prediction: RH fast (early termination), LP medium, Chaining slow at 0.75/0.9.
  At 0.9 we predicted LP "slow — primary clustering is brutal."
- Reality: all three custom implementations within 8%. LP has a slight edge. Load
  factor has **zero** effect on our implementations.
- This is the first big surprise. We built three full prediction matrices for
  load sensitivity that didn't materialize.
- Why: at N=65536 the table doesn't fit in L1/L2 regardless of fill level. The
  dominant cost is cache misses per lookup, not probe chain length.

### FindMiss — we got the ranking backwards

- Prediction: RH fast, LP medium, Chaining slow.
- Reality: RH does win (early termination is real for misses). But **chaining is
  second, not LP.** An empty bucket is an instant "not found."
- LP's load factor pattern is inverted: worst at 0.5 (49K), best at 0.9 (36K).
  - Why: tombstones from the benchmark's erase setup phase. At low load, LP probes
    past tombstones; at high load, real entries fill the gaps.
  - This is counterintuitive and worth dwelling on.

### EraseAndFind — the prediction we were most wrong about

- Prediction: LP slow (tombstones degrade find), RH fast (backshift keeps chains
  clean), Chaining medium.
- Reality: LP is **fastest** (20.9K), 14x faster than chaining. RH close behind.
- The hypothesis dramatically overstated tombstone damage. Cache-friendly flat-array
  scanning through tombstones easily beats pointer-chasing through scattered heap nodes.
- **This is the clearest result**: cache locality >> algorithmic elegance. The
  algorithmic argument (tombstones degrade probing) is correct in theory but
  irrelevant because the constant factor of a cache miss dwarfs it.

### EraseChurn — the biggest prediction miss

- Prediction: LP fast (O(1) tombstone flip), RH slow (O(k) backshift). This was
  supposed to be where Robin Hood suffers.
- Reality: RH is **1.3x faster than LP** and the fastest overall.
- Why: in steady-state churn, backshift keeps probe distances short. No tombstones
  accumulate. The paired insert after each erase has short probes. LP's cheap erase
  shifts cost to the insert half (must probe past tombstones to check for duplicates).
- The O(1) vs O(k) analysis was correct per-operation but missed the steady-state
  interaction between erase and insert.

---

## Section 4: Why We Were Wrong

Pull together the three themes that explain most of the prediction failures:

### 1. Cache locality dominates algorithmic differences

The gap between flat-array probing and pointer-chasing is 15x in EraseAndFind.
Algorithmic refinements (tombstones vs backshift, early termination, probe variance)
produce 1.2-1.3x differences within the open addressing family. The data layout
choice — open addressing vs chaining — is the decision that matters.

We were reasoning about probe chain lengths and O(k) backshifts when we should have
been reasoning about cache lines.

### 2. Load factor sensitivity was overstated

We built three full prediction matrices expecting dramatic degradation at 0.9. The
custom implementations are nearly identical across all three load factors. At N=65536,
cache miss cost dominates probe length. Load factor would matter at sizes fitting in
L2, but for realistic table sizes it's a second-order effect.

### 3. Steady-state behavior ≠ per-operation analysis

The EraseChurn prediction analyzed erase and insert separately: O(1) erase + O(1)
insert for LP, O(k) erase + O(1) insert for RH. But in steady state, LP's tombstones
accumulate between rehashes, making the insert side expensive. RH's expensive erase
keeps the table clean, making the insert side cheap. The system-level behavior inverted
the per-operation ranking.

---

## Section 5: Closing

Summarize: performance intuition is unreliable. Thoughtful, well-reasoned predictions
were wrong in the ways that mattered most. The recurring lesson is that performance is
empirical.

Tease Part 2: "So open addressing wins, SoA layout is the key insight, and the obvious
next optimization is... to merge the arrays for even better locality. Right?" (This is
the AoS disaster from Part 2.)

---

## Open Questions / Things to Decide

- **How much code to show?** The implementations are header-only. Show the hot loop
  for LP find? The Robin Hood swap? Or keep it conceptual and link to the repo?
- **Prediction matrix format:** The raw hypotheses doc has prose per cell. Clean up
  to just fast/medium/slow + one-line reason? Or include the full reasoning?
- **Where to put the data?** Inline tables (as in bench_results.md) or charts? The
  tables are information-dense but charts are more scannable.
- **Tone:** The blog plan says "showing the process, including the wrong turns." How
  self-deprecating vs analytical? I'm leaning analytical — the predictions were
  reasonable given standard CS education, the point is that the standard mental model
  is incomplete.
