# Four Hash Maps, Three Key Distributions, and One Catastrophe

I'm learning performance engineering by building data structures from scratch and
benchmarking them. I'm working with Claude (an LLM) as a collaborator — it
provides the textbook analysis, I write the code, and we both make predictions
before running anything.

This post is about the first thing we built: hash maps. We wrote four
implementations, predicted how they'd perform, ran benchmarks with sequential
keys, and declared open addressing the winner. Then we changed the key
distribution and watched the winner become 11,000x slower.

## The implementations

Four `int → int` hash maps, each using `std::hash<int>` — which on most
compilers is the identity function. We confirmed this by inspecting the compiled
assembly: `hash(42)` compiles to literally `42`, no computation at all.[^identity]

[^identity]: At -O3 the compiler constant-folds `std::hash<int>()(42)` to
the literal 42. At -O0 it emits a function call, but the function body is
`ldrsw x0, [sp, #4]; ret` — load the int, sign-extend to `size_t`, return it.
See `scripts/verify_identity_hash.sh`.

1. **Chaining** — separate chaining with a linked list per bucket. Each node is
   heap-allocated with `make_unique`.

2. **Linear Probing (LP)** — open addressing with tombstone deletion. Uses a
   Structure of Arrays layout: a `vector<uint8_t>` for slot state (empty, occupied,
   deleted) separate from the key-value storage. I reached for this instinctively to
   avoid struct padding waste. I didn't realize at the time that it had important
   cache implications — that understanding came later from the data.

3. **Robin Hood (RH)** — open addressing with Robin Hood insertion (displace
   elements that are closer to their home slot) and backshift deletion (shift
   elements backward on erase, avoiding tombstones). Also SoA layout, but uses
   `vector<bool>` for the occupied state — space-efficient, but every probe pays
   for bit manipulation.

4. **std::unordered_map** — the standard library implementation, wrapped in a
   common interface. Our baseline.

## The scenarios

Five operations, all at load factor 0.75 with N = 65,536 entries:

| Scenario | What's timed |
|---|---|
| **Insert** | Insert N keys into an empty map |
| **FindHit** | Find all N keys (all present) |
| **FindMiss** | Find N keys that aren't in the map |
| **EraseAndFind** | Erase half the entries, then find the survivors |
| **EraseChurn** | Steady-state: insert one, erase one, repeat |

All benchmarks ran on Apple Silicon M4, compiled with LLVM/Clang 21 at -O3.
Medians of 10 repetitions.

## The three key distributions

We test each scenario with three key distributions. This is the variable that
matters — and the one I almost didn't think to vary.

**Sequential:** Keys 0, 1, 2, ..., N-1. With identity hash and a power-of-two
table, these map to consecutive slots. This is the best case for open addressing
— the hardware prefetcher handles linear scans perfectly.

**Uniform random:** N unique keys drawn uniformly from [0, 10N). Scattered
across the table. Each lookup is a potential cache miss regardless of data
layout.

**Normal (clustered):** Keys drawn from a normal distribution centered at N/2
with standard deviation N/8. About 68% of keys cluster into roughly N/4
consecutive slots around the center.

Why these three? After getting the sequential results, I kept thinking: how
often does a real system insert sequential keys into a hash map? And if it does,
wouldn't you use a more efficient bulk-loading strategy anyway?

I thought about what real hash maps look like. A `HashMap<std::string, MyType>`
hashes strings — and string hashes are much closer to uniformly distributed,
regardless of the input pattern. But we're hashing integers with the identity
function. With identity hash, the distribution of keys *is* the distribution
of slots. And integer keys cluster in plenty of real scenarios — auto-increment
IDs, timestamps, geographic coordinates, sensor readings.

The normal distribution seemed like a reasonable model for "keys that cluster
around a hot range." It turned out to be more adversarial than I expected — but
that's exactly the kind of thing you want a benchmark to reveal.

## A quick primer: expected probes

If you already know how open addressing works, skip ahead. If not, here's the
one thing you need to know.

In open addressing, every operation — insert, find, delete — starts by hashing
the key to a slot and then *probing* forward until it finds what it's looking
for (or an empty slot). How many probes does it take? That depends on how full
the table is — the load factor, α:

| Load factor (α) | Expected probes |
|---|---|
| 0.50 (half full) | 2 |
| 0.75 | 4 |
| 0.90 | 10 |
| 0.99 | 100 |

The intuition: each probe has a (1 - α) chance of landing on an empty slot. At
75% full, there's a 25% chance per probe, so on average you need about 4 tries.

This analysis assumes probes are independent — each slot has the same probability
of being empty. For linear probing that's not quite true, because occupied slots
clump together (primary clustering), making probes correlated. But the formula
gives the right order of magnitude.

## Our predictions

Before running anything, we wrote down what we expected.

I knew chaining — it's the textbook default — and I'd heard the complaints about
`std::unordered_map` being slow. Linear probing and Robin Hood were new to me.
Claude provided the textbook analysis of clustering, probe distances, and
tombstone behavior.

I worried about Robin Hood: the displacement chains during insert and the
backshifting during delete sounded expensive. I also worried about LP: wouldn't
clusters degrade lookup?

Claude worried about EraseChurn: Robin Hood's O(k) backshift per erase should
lose to LP's O(1) tombstone flip. And about clustering: primary clustering should
hurt LP under non-ideal conditions.

We wrote a prediction matrix:

| Scenario | Chaining | LP | RH | std |
|---|---|---|---|---|
| Insert | medium | fast | medium — swap overhead | slow |
| FindHit | slow — pointer chase | medium — clusters | fast — uniform probes | slow |
| FindMiss | slow — walk full chain | medium | fast — early termination | slow |
| EraseAndFind | medium | slow — tombstones hurt | fast — clean table | medium |
| EraseChurn | medium | fast — tombstone flip | slow — O(k) backshift | medium |

Our key beliefs:

- Chaining degrades gracefully; open addressing has sharp failure modes
- `std` is always slowest (chaining overhead + stdlib bloat)
- EraseChurn is where Robin Hood pays for its complexity

## Act 1: Sequential keys

Sequential keys first. This is where most hash map benchmarks begin and end.

### Results (cpu_time, nanoseconds)

| Scenario | LP | RH | Chain | std |
|---|---:|---:|---:|---:|
| Insert | **117K** | 196K | 2.22M | 1.18M |
| FindHit | **45.4K** | 48.7K | 54.0K | 97.5K |
| FindMiss | 39.5K | **32.6K** | 36.9K | 76.9K |
| EraseAndFind | **21.0K** | 25.8K | 314K | 349K |

LP wins three of four scenarios. RH takes FindMiss. Open addressing dominates.

<!-- CHART: Grouped bar chart, 4 scenarios × 4 implementations. Log scale on
y-axis (ns). Color by implementation. The visual takeaway: LP's bars are
consistently shortest, Chain and std are 10-20x taller on Insert and
EraseAndFind. FindHit is the one where all four bars are close. -->

### What we got right and wrong

**Insert** — LP is fastest, as predicted. But it's 19x faster than chaining. We
expected maybe 2-3x. And our chaining is slower than `std` — we predicted the
opposite. `std::unordered_map`'s allocator handles per-node allocation better
than our naive `make_unique` calls.

**FindHit** — We predicted RH fastest thanks to its more uniform probe
distribution. In reality, LP, RH, and Chain are all within 8% of each other.
The table is about 780KB at this size — well beyond L1 cache (128KB on M4). At
this scale, the cost of a cache miss dominates the number of probes. Whether you
probe 2 slots or 4, you're paying for the same DRAM round trip.

**FindMiss** — RH wins as predicted, thanks to early termination (it can detect
that a key can't be present without reaching an empty slot). But chaining is
second, not last as we predicted. An empty bucket in chaining is an instant "not
found" — no probing needed. At load 0.75, a quarter of buckets are empty.

**EraseAndFind** — This is the prediction we were most confident about, and most
wrong. We said LP would be slow because tombstones degrade subsequent finds.
We said RH would be fast because backshift deletion keeps the table clean.
Reality: LP is fastest, 15x faster than chaining. Tombstone damage is real in
theory — but scanning through tombstones in a flat array is cache-friendly.
Chasing pointers through scattered heap nodes is not. The constant factor of a
cache miss dwarfs the algorithmic difference in probe count.

### The tempting conclusion

Open addressing dominates. LP wins three of four scenarios. Chaining is 15-19x
slower on insert and erase. Case closed?

This is where many performance blog posts would end. We almost did. But I kept
coming back to the same question: these keys are 0, 1, 2, ..., N-1, hashed by
the identity function into consecutive slots. How often does that actually
happen? Almost every optimization here is a tradeoff based on an assumption
about the input. What happens when the assumption breaks?

## Act 2: Uniform random keys

Same scenarios, but now keys are uniformly random instead of sequential. No more
consecutive slots. No more prefetcher-friendly linear scans. Every lookup is a
potential cache miss.

### Results (cpu_time, nanoseconds)

| Scenario | LP | RH | Chain | std |
|---|---:|---:|---:|---:|
| Insert | **705K** | 1.05M | 4.26M | 2.09M |
| FindHit | 128K | 127K | **125K** | 324K |
| FindMiss | 403K | **233K** | 233K | 322K |
| EraseAndFind | 106K | **74.0K** | 561K | 475K |

### What changed

The gap is closing. LP's insert advantage shrank from 19x to 6x over chaining.
FindHit: chaining *ties* LP and RH — all three are within 3% of each other. The
sequential advantage was partly cache flattery: the hardware prefetcher was doing
work that open addressing got credit for.

Rankings shifted too. RH takes EraseAndFind from LP (1.4x faster). Chaining ties
RH on FindMiss.

The ratio of uniform to sequential performance tells the real story:

| Scenario | LP | RH | Chain | std |
|---|---:|---:|---:|---:|
| Insert | 6.0x | 5.3x | 1.9x | 1.8x |
| FindHit | 2.8x | 2.6x | 2.3x | 3.3x |
| FindMiss | 10.2x | 7.1x | 6.3x | 4.2x |
| EraseAndFind | 5.0x | 2.9x | 1.8x | 1.4x |

<!-- CHART: Grouped bar chart showing the uniform/sequential slowdown ratio for
each implementation × scenario. The asymmetry is the point: LP and RH bars are
consistently 3-10x, Chain and std bars are 1.5-4x. LP's FindMiss bar at 10.2x
is the outlier. -->

Open addressing pays a higher tax for random access than chaining does. Chaining's
insert slows down 1.9x; LP's slows down 6x. The pointer chasing that made
chaining slow with sequential keys is less of a disadvantage when open
addressing's flat-array scans aren't benefiting from the prefetcher anyway.

LP's FindMiss is the most striking: 10.2x slower with uniform keys. Scattered
miss probes through tombstones are expensive when every probe is a cache miss.

But open addressing still wins on absolute numbers in most scenarios. Random
keys hurt everyone — they just hurt open addressing more.

## Act 3: Normal keys

Keys drawn from a normal distribution around N/2, with standard deviation N/8.
About 68% of keys cluster into roughly N/4 consecutive slots around the
center of the table.

This is adversarial for open addressing with identity hash. But it's not
artificial. Real workloads cluster around hot ranges. The question is: how badly
does it break things?

### Results (cpu_time, nanoseconds)

| Scenario | LP | RH | Chain | std |
|---|---:|---:|---:|---:|
| Insert | **744M** | **2,041M** | 4.35M | **2.08M** |
| FindHit | **522M** | **602M** | 513K | **313K** |
| FindMiss | 80.1K | **33.1K** | 47.9K | 438K |
| EraseAndFind | **786M** | **52.6M** | 528K | **456K** |

The numbers in bold are catastrophic — hundreds of milliseconds to *seconds*
per batch operation on just 65,536 entries.

<!-- CHART: This is the most important visualization in the post. Two options:
(a) Log-scale bar chart, all four implementations × 4 scenarios. The LP/RH
bars for Insert/FindHit/EraseAndFind tower over Chain/std by 3+ orders of
magnitude. FindMiss is the exception where RH is still competitive.
(b) Side-by-side panels: left panel shows Chain and std at linear scale (sub-ms),
right panel shows LP and RH at a different scale (hundreds of ms to seconds).
The contrast between panels IS the story. -->

### What happened

Open addressing with identity hash went quadratic.

LP's Insert takes 744 million nanoseconds — 6,341x slower than with sequential
keys. LP's FindHit takes 522 million — 11,477x slower. These aren't "slow."
They're broken. The normal distribution creates a massive cluster around N/2,
and with identity hash mapping those keys directly to those slots, every insert
and find has to probe through the entire cluster.

Robin Hood is even worse on some operations. RH Insert takes 2 *billion*
nanoseconds. Robin Hood's displacement rule — "take from the rich, give to the
poor" — means each insert into the dense cluster displaces an element, which
displaces another, propagating through thousands of entries.

Chaining barely notices. Chain FindHit goes from 54K (sequential) to 513K
(normal) — a 9.5x slowdown, not an 11,000x one. The clustered buckets have
longer chains, but each chain is independent. There's no cascading effect.
Remember our prediction that "chaining degrades gracefully"? We were right —
we just didn't appreciate what that would be worth until we saw the alternative.

And `std::unordered_map` wins. For the first time in any scenario, the
implementation we treated as a punching bag is the fastest on Insert, FindHit,
and EraseAndFind. `std` is also chaining under the hood, so it's immune to the
clustering catastrophe. Its node-based allocation, which was a liability with
sequential keys, is irrelevant when the alternative is probing through 50,000
occupied slots.

There's one exception. FindMiss with normal keys: RH scores 33.1K — nearly
identical to its 32.6K with sequential keys. Miss probes don't hit the dense
core. They start at a hashed slot, probe forward, and quickly find an empty
slot in the sparse tails of the distribution. The cluster can't hurt you if
you never enter it.

### The full picture

Here's which implementation wins each scenario, by key distribution:

| Scenario | Sequential | Uniform | Normal |
|---|---|---|---|
| Insert | LP | LP | std |
| FindHit | LP | Chain/RH/LP (tied) | std |
| FindMiss | RH | RH/Chain (tied) | RH |
| EraseAndFind | LP | RH | std |

<!-- CHART: Color-coded grid/heatmap — rows are scenarios, columns are key
distributions, cells show the winner. Green for LP, blue for RH, orange for
Chain, red for std. The visual: Act 1 is all green, Act 2 is mixed, Act 3
is mostly red. The color shift across columns tells the whole story at a
glance. -->

No implementation wins everywhere. The "best" hash map depends entirely on your
key distribution — which, when you're using identity hash, is a fancy way of
saying it depends on your data.

## What we learned

Three themes emerged from progressively breaking our assumptions.

### Your benchmark's key distribution is the result

Sequential keys told us "LP is 19x faster than chaining." Normal keys told us
"LP is 1,700x slower than `std`." Both statements are true. Neither is the
whole story.

Most hash map benchmarks use sequential or uniform-random keys. Those are the
easy cases — they're the ones where identity hash works fine and open addressing
looks good. The interesting cases involve clustering, which identity hash does
nothing to prevent.

### Cache locality dominates — until it doesn't

With sequential keys, flat-array probing versus pointer-chasing produces a
15x gap (EraseAndFind). This is the lesson from Act 1, and it's real.

But it has a precondition: the probing has to be short. When clustered keys
turn 4-probe lookups into 10,000-probe lookups, it doesn't matter that each
individual probe is cache-friendly. 10,000 fast probes is worse than 10 slow
ones.

Cache locality is the most important constant factor in hash map performance.
But it can't save you from an algorithmic blowup.

### Identity hash is the root cause

The catastrophe isn't inherent to open addressing. It's the interaction between
three things: identity hash, clustered keys, and linear probing. Identity hash
maps clustered keys to clustered slots. Linear probing turns clustered slots
into mega-clusters.

The obvious fix is a better hash function — one that scatters clustered keys
across the table. But scattering has a cost. For sequential keys, where
identity hash is perfect, any hash computation is pure overhead.

## Closing

We started with reasonable predictions, benchmarked with sequential keys, and
thought we understood hash map performance. Then we changed the key distribution
and every conclusion inverted.

The thing that sticks with me is how confident the sequential results felt.
LP was 19x faster — that's not a marginal difference you can explain away. It
felt like a settled question. But the result was an artifact of the best-case
input, and the best case is the one you should trust least.

Every optimization is a tradeoff built on an assumption: about the input, the
hardware, the access pattern. Open addressing assumes keys scatter reasonably
across the table. Identity hash assumes the keys are already well-distributed.
When those assumptions hold, you get 19x wins. When they don't, you get 11,000x
losses. The question isn't "which hash map is fastest?" It's "what does your
data actually look like, and how badly does your design degrade when you're
wrong about that?"

Performance engineering is empirical. The only honest approach is hypothesis,
benchmark, analyze — then change an assumption and do it again.

---

*N = 65,536. Apple Silicon M4 (P-core, 128KB L1, 16MB L2). LLVM/Clang 21.1.8,
-O3. Load factor 0.75. Medians of 10 repetitions. All code is
[on GitHub](https://github.com/TODO).*
