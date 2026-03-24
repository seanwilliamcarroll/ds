# Post 1: "Your Hash Map Hypotheses Are Probably Wrong"

## Hook

We (a human learning performance engineering and an LLM) wrote predictions for
5 benchmark scenarios before running any code. The predictions were reasonable and
wrong in almost every way that mattered.

(Invite the reader to form their own predictions before scrolling past the results.)

---

## Section 1: The Setup

### The four implementations

Brief descriptions — just enough for readers to reason about tradeoffs:

1. **Chaining** — separate chaining, linked list per bucket, `make_unique` per node
2. **Linear Probing** — open addressing, tombstone deletion, SoA layout (separate
   state array + key-value array)
3. **Robin Hood** — open addressing, Robin Hood insertion (displace elements with
   shorter probe distance), backshift deletion (no tombstones), SoA layout,
   `vector<bool>` state
4. **std::unordered_map** — stdlib wrapper, baseline

Design choice to mention: SoA layout. I (Sean) instinctively reached for separate
arrays to avoid struct padding waste. The cache behavior implications weren't on
my mind — that understanding came later from the data. (Don't explain the cache
angle yet.)

For Robin Hood, I used `vector<bool>` because I knew it was space-efficient. I
didn't consider the per-probe cost of bit manipulation. That shows up later.

### The scenarios

Brief table of the 5 scenarios at load factor 0.75:

| Scenario | What's timed |
|---|---|
| Insert | Insert N sequential keys into empty map |
| FindHit | Find all N keys (all hits) |
| FindMiss | Find N keys that don't exist |
| EraseAndFind | Erase half the entries, then find survivors |
| EraseChurn | Steady-state: insert one, erase one, repeat |

Sequential keys with identity hash (`std::hash<int>` = identity on most compilers).
This is the best case for open addressing — we'll break this assumption later.

N = 65536. Apple Silicon M4, LLVM/Clang 21, -O3.

---

## Section 2: The Predictions

### What we knew going in

I knew chaining and had heard the complaints about std::unordered_map being slow.
LP and RH were new to me. Claude provided the textbook analysis of clustering,
probe distances, and tombstone behavior.

My concerns:
- **RH insert/delete**: the displacement chains and backshifting sounded expensive
- **LP find**: wouldn't clusters degrade lookup?

Claude's concerns:
- **Load 0.9**: "primary clustering is brutal" for LP, chaining should close the gap
- **EraseChurn**: RH's O(k) backshift per erase should lose to LP's tombstone flip

### The prediction matrix (load 0.75)

One table. fast/medium/slow with one-line reasoning per cell. Cleaned up from the
raw hypotheses doc — keep it scannable.

| Scenario | Chaining | LP | RH | std |
|---|---|---|---|---|
| Insert | medium | fast | medium — swap overhead | slow |
| FindHit | slow — pointer chase | medium — clusters forming | fast — uniform probes | slow |
| FindMiss | slow — walk full chain | medium | fast — early termination | slow |
| EraseAndFind | medium | slow — tombstones degrade find | fast — backshift keeps table clean | medium |
| EraseChurn | medium | fast — tombstone flip | slow — O(k) backshift | medium |

### Aside: what does "expected probes" mean?

In open addressing, every operation starts by probing slots until you find what
you're looking for (or an empty slot). The expected number of probes depends on
how full the table is — the load factor α:

| Load factor (α) | Expected probes: 1/(1-α) |
|---|---|
| 0.5 (half full) | 2 |
| 0.75 | 4 |
| 0.9 | 10 |
| 0.99 | 100 |

Intuition: each probe has a (1-α) chance of hitting an empty slot. At 75% full,
there's a 25% chance each probe lands on empty, so on average you need 4 tries.
As the table fills up, empty slots get scarce and probe chains get long — that's
why we expected high load factors to hurt.

(This is for uniform/random hashing. Linear probing is worse in theory because
of clustering — occupied slots clump together, so you don't get independent
(1-α) chances per probe. But as we'll see, the theory overstates this.)

### Key beliefs

- Chaining degrades gracefully; open addressing has sharp failure modes at high load
- std always slowest (chaining + stdlib overhead)
- EraseChurn is where RH pays for its complexity

"Let's see what actually happened."

---

## Section 3: The Results

For each scenario: the numbers, what we expected, what happened, and why. Each
surprise builds toward the overarching lesson without stating it upfront.

### Insert — LP wins, but the magnitudes are wrong

| Implementation | cpu_time (ns) |
|---|---:|
| LP | 117K |
| RH | 197K |
| std | 1.15M |
| Chain | 2.27M |

- We got the ranking half right: LP is fastest.
- The magnitude is shocking: LP is **16x** faster than chaining. We expected 2-3x.
- Our chaining is slower than std — we predicted the opposite. std's allocator
  handles per-node allocation better than raw `make_unique`.
- RH's displacement overhead is real (1.7x slower than LP) but not crippling.

### FindHit — everything is the same

| Implementation | cpu_time (ns) |
|---|---:|
| LP | 45.4K |
| RH | 48.7K |
| Chain | 49.0K |
| std | 99.0K |

- We predicted RH fastest (early termination, uniform probes). It's tied with
  chaining. LP has a slight edge.
- All three custom implementations are within 8% of each other.
- The prediction that clustering would hurt LP at 0.75: wrong. The table is ~780KB
  at this size — doesn't fit in L1 (128KB). Cache misses per lookup dominate
  probe chain length.

### FindMiss — we got the ranking backwards

| Implementation | cpu_time (ns) |
|---|---:|
| RH | 32.5K |
| Chain | 35.7K |
| LP | 38.4K |
| std | 76.7K |

- RH wins as predicted — early termination is genuinely helpful for misses.
- But **chaining is second, not LP.** An empty bucket in chaining is an instant
  "not found" — no probing at all. We had chaining as "slow" here.
- LP is last among custom implementations. Tombstones from the benchmark's
  erase setup extend probe runs for miss lookups.

### EraseAndFind — the prediction we were most confident about, and most wrong

| Implementation | cpu_time (ns) |
|---|---:|
| LP | 20.9K |
| RH | 25.8K |
| Chain | 312K |
| std | 347K |

- We predicted: LP slow (tombstones degrade find), RH fast (clean table), Chaining
  medium.
- Reality: LP is **fastest**. 15x faster than chaining. RH close behind.
- Tombstone damage is real in theory — but scanning through tombstones in a flat
  array is cache-friendly. Pointer-chasing through scattered heap nodes is not.
  The constant factor of a cache miss dwarfs the algorithmic difference.
- **This is the clearest result in the entire benchmark.** We can talk about probe
  chain lengths and tombstone degradation all day, but a flat array scan is just
  fundamentally faster than following pointers to random memory locations.

### EraseChurn — the biggest prediction miss

| Implementation | cpu_time (ns/iter) |
|---|---:|
| RH | 4.2 |
| LP | 5.5 |
| Chain | 14.7 |
| std | 18.6 |

- We predicted: LP fast (tombstone flip is cheap), RH slow (backshift is O(k)).
  This was supposed to be where RH suffers.
- Reality: **RH is 1.3x faster than LP.**
- Why: we analyzed erase and insert as independent operations. In steady-state
  churn, they interact. RH's backshift keeps the table clean — no tombstones
  accumulate, so the next insert finds a short probe. LP's cheap tombstone erase
  pollutes the table, making the paired insert probe past dead slots.
- The per-operation complexity was technically correct but missed the system-level
  behavior. (And "O(1) tombstone flip" is only the marking — finding the element
  to erase is still O(1/(1-α)) probing, same as RH.)

---

## Section 4: What We Learned

Three themes that explain most of the prediction failures.

### Cache locality dominates algorithmic differences

Flat-array probing vs pointer-chasing: 15x gap (EraseAndFind). Algorithmic
refinements within open addressing (tombstones vs backshift, early termination):
1.2-1.3x differences. The data layout choice is the decision that matters most.

We were reasoning about probe chain lengths when we should have been reasoning
about cache lines.

### Load factor is a second-order effect at realistic sizes

(Brief aside — don't belabor. One paragraph + a small comparison showing all
three loads are within ~5% for custom implementations. The point: at table sizes
that don't fit in L1/L2, cache miss cost dominates probe length. Load factor
matters more at small sizes where extra probes are cheap cache hits.)

### Per-operation analysis misses system-level behavior

EraseChurn: O(1) erase + O(1/(1-α)) insert for LP, O(k) erase + short insert
for RH. The per-operation ranking said LP wins. The steady-state interaction —
tombstones degrading future inserts — inverted it.

---

## Section 5: Closing

Performance intuition is unreliable. The standard CS mental model (load factor
sensitivity, tombstone degradation, O(k) backshift cost) is correct in isolation
but incomplete — it doesn't account for cache behavior or system-level interactions.

Tease Part 2: "So open addressing wins and SoA layout is the key insight. The
obvious next step is to merge the arrays for even better locality. Right?"

---

## Open Questions

- **How much code to show?** Leaning toward: one diagram showing SoA layout (state
  array separate from KV array), brief code snippet of the LP find loop. Keep RH
  conceptual. Link to repo for full implementations.
- **Charts vs tables?** Tables are more precise. Could do both — chart for the
  visual comparison, table in a footnote or appendix for exact numbers.
- **The Part 2 tease** sets up the AoS disaster. Is it too cute? Or does it land
  well as a cliffhanger?
