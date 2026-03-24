# Post 1: "Your Hash Map Hypotheses Are Probably Wrong"

## Hook

We (a human learning performance engineering and an LLM) wrote predictions for
hash map benchmarks before running any code. Then we ran sequential keys and
declared open addressing the winner. Then we changed the key distribution and
watched the winner become 11,000x slower.

(Invite the reader to form their own predictions at each stage.)

---

## Section 1: The Setup

### The four implementations

Brief descriptions — just enough for readers to reason about tradeoffs:

1. **Chaining** — separate chaining, linked list per bucket, `make_unique` per node
2. **Linear Probing (LP)** — open addressing, tombstone deletion, SoA layout
   (separate state array + key-value array)
3. **Robin Hood (RH)** — open addressing, Robin Hood insertion (displace elements
   with shorter probe distance), backshift deletion (no tombstones), SoA layout,
   `vector<bool>` state
4. **std::unordered_map** — stdlib wrapper, baseline

Design choice to mention: SoA layout. I (Sean) instinctively reached for separate
arrays to avoid struct padding waste. The cache behavior implications weren't on
my mind — that understanding came later from the data. (Don't explain the cache
angle yet.)

For Robin Hood, I used `vector<bool>` because I knew it was space-efficient. I
didn't consider the per-probe cost of bit manipulation. That shows up later.

### The scenarios

Five scenarios, all at load factor 0.75, N=65536:

| Scenario | What's timed |
|---|---|
| Insert | Insert N keys into empty map |
| FindHit | Find all N keys (all hits) |
| FindMiss | Find N disjoint keys (all misses) |
| EraseAndFind | Erase half, then find survivors |
| EraseChurn | Steady-state: insert one + erase one, repeat |

All implementations use identity hash (`std::hash<int>` = identity on most
compilers[^identity-hash]). Apple Silicon M4, LLVM/Clang 21, -O3.

[^identity-hash]: We confirmed this by compiling a minimal `std::hash<int>`
program at -O3 and inspecting the assembly. The compiler constant-folds
`hash(42)` to the literal `42` — no function call, no computation. The hash
is completely eliminated. See `scripts/verify_identity_hash.sh` for the test
and assembly output.

### The three key distributions

The benchmark tests each scenario under three key distributions:

1. **Sequential** — keys 0, 1, 2, ..., N-1. Identity hash maps these to
   consecutive slots. Best case for open addressing — the prefetcher handles
   this perfectly.
2. **Uniform random** — N unique keys drawn uniformly from [0, 10N). Scattered
   across the table. Each lookup is a potential cache miss.
3. **Normal (clustered)** — keys drawn from N(N/2, N/8). ~68% of keys cluster
   into ~N/4 consecutive slots around the center. Moderate-to-severe clustering.

(Brief explanation of *why* these three patterns: sequential is the textbook
best case, uniform is realistic, normal is adversarial-but-plausible. Real
workloads cluster — think auto-increment IDs, timestamps, geographic data.)

### Aside: what does "expected probes" mean?

In open addressing, every operation probes slots until it finds what it's looking
for (or an empty slot). Expected probes depend on load factor α:

| Load factor (α) | Expected probes: 1/(1-α) |
|---|---|
| 0.5 (half full) | 2 |
| 0.75 | 4 |
| 0.9 | 10 |
| 0.99 | 100 |

Intuition: each probe has a (1-α) chance of hitting an empty slot. At 75% full,
25% chance per probe → ~4 tries on average.

This assumes uniform/random hashing. Linear probing is worse in theory — occupied
slots clump together (primary clustering), so probes aren't independent.

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
- **EraseChurn**: RH's O(k) backshift per erase should lose to LP's tombstone flip
- **Clustering**: primary clustering should hurt LP under non-ideal conditions

### The prediction matrix

| Scenario | Chaining | LP | RH | std |
|---|---|---|---|---|
| Insert | medium | fast | medium — swap overhead | slow |
| FindHit | slow — pointer chase | medium — clusters | fast — uniform probes | slow |
| FindMiss | slow — walk full chain | medium | fast — early termination | slow |
| EraseAndFind | medium | slow — tombstones degrade find | fast — clean table | medium |
| EraseChurn | medium | fast — tombstone flip | slow — O(k) backshift | medium |

Key beliefs:
- Chaining degrades gracefully; open addressing has sharp failure modes
- std always slowest (chaining + stdlib overhead)
- EraseChurn is where RH pays for its complexity

---

## Section 3: Act 1 — Sequential Keys (The Comfortable Answer)

Sequential keys first. This is where most benchmarks stop.

### The results

| Scenario | LP | RH | Chain | std |
|---|---:|---:|---:|---:|
| Insert | **117K** | 196K | 2.22M | 1.18M |
| FindHit | **45.4K** | 48.7K | 54.0K | 97.5K |
| FindMiss | 39.5K | **32.6K** | 36.9K | 76.9K |
| EraseAndFind | **21.0K** | 25.8K | 314K | 349K |

(EraseChurn omitted for now — per-iteration numbers too small for this table's
format. Can include if it adds to the narrative.)

### What we got right and wrong

**Insert: LP wins, but the magnitudes are shocking.** LP is 19x faster than
chaining. We expected 2-3x. Our chaining is slower than std — std's allocator
handles per-node allocation better than raw `make_unique`.

**FindHit: everything is the same.** LP, RH, and Chain within 8% of each other.
We predicted RH fastest (uniform probes). The table is ~780KB — doesn't fit in
L1 (128KB). Cache misses per lookup dominate probe chain length.

**FindMiss: we got the ranking backwards.** RH wins as predicted. But chaining is
second — an empty bucket is an instant "not found." We had chaining as "slow."

**EraseAndFind: the prediction we were most confident about, most wrong.** We said
LP slow (tombstones), RH fast (clean table). Reality: LP fastest, 15x over
chaining. Scanning tombstones in a flat array is cache-friendly. Chasing pointers
through heap nodes is not.

### The tempting conclusion

Open addressing dominates. LP wins 3 of 4 scenarios, RH takes the other. Chaining
is 15-19x slower on insert and erase. Case closed.

This is where many performance blog posts would end. We almost did.

---

## Section 4: Act 2 — Uniform Keys (The Cracks Appear)

Same scenarios, but keys are uniformly random instead of sequential. Every lookup
is a potential cache miss regardless of data layout.

### The results

| Scenario | LP | RH | Chain | std |
|---|---:|---:|---:|---:|
| Insert | **705K** | 1.05M | 4.26M | 2.09M |
| FindHit | 128K | 127K | **125K** | 324K |
| FindMiss | 403K | **233K** | 233K | 322K |
| EraseAndFind | 106K | **74.0K** | 561K | 475K |

### What changed

**The gap between open addressing and chaining is shrinking.** Insert: LP was 19x
faster with sequential keys, now 6x. FindHit: Chain *ties* LP and RH. The
sequential advantage was partly cache flattery — the prefetcher was doing work
that open addressing got credit for.

**Rankings shift.** RH takes EraseAndFind from LP (1.4x). Chain ties RH on
FindMiss. LP's FindMiss degrades the most (10x from sequential to uniform,
vs 7x for RH).

**Uniform/Sequential ratio tells the story:**

| Scenario | LP (U/S) | RH (U/S) | Chain (U/S) | std (U/S) |
|---|---:|---:|---:|---:|
| Insert | 6.0x | 5.3x | 1.9x | 1.8x |
| FindHit | 2.8x | 2.6x | 2.3x | 3.3x |
| FindMiss | 10.2x | 7.1x | 6.3x | 4.2x |
| EraseAndFind | 5.0x | 2.9x | 1.8x | 1.4x |

Open addressing pays a higher tax for random access than chaining does. LP's
FindMiss 10x penalty is striking — scattered miss probes through tombstones are
expensive when every probe is a cache miss.

**But open addressing still wins overall.** The absolute numbers favor LP and RH
in most scenarios. Random keys hurt everyone, they just hurt open addressing more.

---

## Section 5: Act 3 — Normal Keys (The Catastrophe)

Keys drawn from a normal distribution around N/2. ~68% of keys cluster into N/4
consecutive slots. This is adversarial for open addressing with identity hash, but
it's not artificial — real workloads cluster (timestamps, auto-increment IDs,
geographic coordinates).

### The results

| Scenario | LP | RH | Chain | std |
|---|---:|---:|---:|---:|
| Insert | **744M** | **2,041M** | 4.35M | 2.08M |
| FindHit | **522M** | **602M** | 513K | 313K |
| FindMiss | 80.1K | **33.1K** | 47.9K | 438K |
| EraseAndFind | **786M** | **52.6M** | 528K | 456K |

Bold = catastrophic (millions of ns → seconds per operation).

### What happened

**Open addressing with identity hash goes quadratic.** LP Insert: 744 *million*
ns (6,341x slower than sequential). LP FindHit: 522 million (11,477x). These
aren't "slow" — they're broken. The normal distribution creates a massive cluster
around N/2, and every insert/find probes through the entire cluster.

**Robin Hood is even worse on some scenarios.** RH Insert is 2 *billion* ns —
Robin Hood displacement chains propagate through the dense cluster, touching and
shifting thousands of elements per insert.

**Chaining barely notices.** Chain FindHit: 513K normal vs 54K sequential — a 9.5x
slowdown, not 11,000x. Longer chains in the clustered buckets, but no cascading
effects. Exactly the "degrades gracefully" behavior we predicted for the wrong
reasons.

**std::unordered_map wins.** For the first time in any scenario, std is fastest on
Insert, FindHit, and EraseAndFind. The implementation we treated as a punching bag
is the most robust.

**FindMiss is the exception.** RH's FindMiss is 33.1K with normal keys — nearly
identical to its 32.6K with sequential. Miss probes hit the sparse tails of the
distribution, not the dense core. The cluster doesn't hurt if you never probe into
it.

### The complete picture

Rankings by pattern, all scenarios:

| Scenario | Sequential winner | Uniform winner | Normal winner |
|---|---|---|---|
| Insert | LP | LP | std |
| FindHit | LP | Chain/RH/LP (tied) | std |
| FindMiss | RH | RH/Chain (tied) | RH |
| EraseAndFind | LP | RH | std |

No implementation wins everywhere. The "best" hash map depends entirely on your
key distribution — which is another way of saying it depends on your hash function.

---

## Section 6: What We Learned

Three themes that emerged from progressively breaking our assumptions.

### 1. Your benchmark's key distribution IS the result

Sequential keys told us "LP is 19x faster than chaining." Normal keys told us
"LP is 1,700x slower than std." Both statements are true. Neither is the whole
story.

Most hash map benchmarks use sequential or random-uniform keys. Those are the easy
cases. The interesting — and realistic — cases involve clustering, which identity
hash does nothing to prevent.

### 2. Cache locality dominates, until it doesn't

Sequential keys: flat-array probing vs pointer-chasing produces a 15x gap
(EraseAndFind). This is the lesson from Act 1, and it's real.

But it has a precondition: the probing has to be short. With clustered keys, LP
probes through thousands of occupied slots. Each probe is still cache-friendly,
but doing 10,000 cache-friendly probes is worse than doing 10 cache-unfriendly
ones. O(N) × fast < O(1) × slow when N is large enough.

### 3. Identity hash is the root cause

The catastrophe isn't inherent to open addressing — it's the interaction between
identity hash, clustered keys, and linear probing. Identity hash maps clustered
keys to clustered slots. Linear probing turns clustered slots into mega-clusters.

This points directly at the fix: a better hash function that scatters clustered
keys. But that's a tradeoff too — scattering costs cycles, and for sequential
keys (where identity hash is perfect), it's pure overhead.

---

## Section 7: Closing

We started with reasonable predictions, benchmarked with sequential keys, and
thought we understood hash map performance. Then we changed the key distribution
and every conclusion inverted.

The pattern repeats throughout this series: measure one thing, think you understand
the system, change a variable, realize you don't. Performance engineering is
empirical. The only honest approach is hypothesis, benchmark, analyze — and then
change an assumption and do it again.

---

## Open Questions

- **How much code to show?** Leaning toward: one diagram showing SoA layout (state
  array separate from KV array), brief code snippet of the LP find loop. The normal
  key generation is worth showing — the `normal_part + X * (1<<20)` trick is
  interesting. Link to repo for full implementations.
- **Charts vs tables?** Sequential and uniform tables are fine. The normal results
  beg for a bar chart — the 11,000x bars next to 1.6x bars are visceral.
- **How to handle EraseChurn?** Per-iteration numbers (4-18 ns) don't fit the table
  format with the other scenarios (21K-2M ns). Could use a separate mini-table, or
  fold it into a "steady-state" sidebar.
- **How deep on the normal key generation?** The `normal_part + X * (1<<20)` trick
  is mathematically interesting (stride vanishes in `key & (num_buckets-1)`). Could
  be a sidebar or footnote for the technically curious.
