# Hash Map Benchmark Hypotheses

Predictions for each scenario before running benchmarks.
Scale: fast / medium / slow (relative to other implementations in that scenario).

## Load 0.5 (aggressive resizing, lots of empty slots)

| Scenario | Chaining | Linear Probing | Robin Hood | std::unordered_map |
|---|---|---|---|---|
| Insert | medium — few collisions but still heap-allocates nodes | fast — almost no collisions, straight to empty slot | medium — almost no collisions either, but swap machinery is wasted overhead | slow — same as chaining plus stdlib overhead |
| Find hit | medium — short chains (1-2 nodes), but still pointer chases | fast — very short probes, cache-friendly | fast — similar to linear probing at this load, early termination barely matters | slow — pointer chasing + stdlib indirection |
| Find miss | medium — chains are short, quick to exhaust | fast — hits empty slot almost immediately | fast — same as linear probing, empty slots everywhere | slow — same as chaining |
| Erase + find | medium — unlink cheap, finds still pointer-chase | medium — few tombstones accumulate at low load | fast — backshift is cheap (short shifts), probe chains stay clean | slow — same as chaining |
| High load find | N/A — this scenario forces 0.9 load, see below | | | |
| Erase-heavy churn | medium — alloc/free per node | fast — tombstone flip is O(1), and frequent resizes clear them | medium — backshift is cheap at low load (short chains), but still more work than a flag flip | medium — alloc/free churn |

At 0.5, open addressing dominates. Collisions are rare so linear probing's simplicity wins — Robin Hood's swap logic is overhead with no payoff.

## Load 0.75 (default, balanced)

| Scenario | Chaining | Linear Probing | Robin Hood | std::unordered_map |
|---|---|---|---|---|
| Insert | medium — moderate chains, heap alloc per new node | fast — walk to next empty, no swaps | medium — swaps start happening but not frequent | slow — chaining + stdlib overhead |
| Find hit | slow — chains avg ~3-4 nodes, pointer chase each | medium — cache-friendly but clusters forming | fast — uniform probe lengths, early termination helps | slow — pointer chasing + indirection |
| Find miss | slow — must walk full chain | medium — probes until empty, clusters extend this | fast — early termination on miss is Robin Hood's biggest win | slow — same as chaining |
| Erase + find | medium — unlink cheap, find still slow | slow — tombstones accumulate, degrade find | fast — backshift keeps chains clean | medium — same as chaining |
| High load find | N/A — this scenario forces 0.9 load, see below | | | |
| Erase-heavy churn | medium — alloc/free per node | fast — O(1) tombstone erase | slow — backshift is O(k) per erase, k growing with load | medium — alloc/free churn |

The sweet spot. Robin Hood's advantages (uniform probes, early termination) start to matter. Linear probing's clusters start forming but aren't devastating yet.

## Load 0.9 (stressed, near-full tables)

| Scenario | Chaining | Linear Probing | Robin Hood | std::unordered_map |
|---|---|---|---|---|
| Insert | medium — long chains but each insert is append-to-list | slow — long cluster walks to find empty slot | slow — long cluster walks plus swap cascades on every insert | medium — same as chaining |
| Find hit | medium — long chains (~5-10 nodes) but degradation is linear | slow — primary clustering is brutal, long probe runs | medium — probe lengths are long but uniform, early termination helps less when everyone is displaced | medium — similar to chaining |
| Find miss | medium — long chain walk but bounded by chain length | slow — must probe through massive clusters to reach empty | medium — early termination still helps but less effective at 0.9 | medium — similar to chaining |
| Erase + find | medium — unlink cheap, find through long chains | slow — tombstones everywhere, find is terrible | medium — backshift is expensive (long shifts) but find stays cleaner than linear probing | medium — same as chaining |
| High load find | medium — chains are long but each bucket independent, predictable degradation | slow — primary clustering at 0.9 is worst-case for linear probing | medium — better than linear probing (uniform probe lengths) but still long probes | medium — similar to chaining |
| Erase-heavy churn | medium — alloc/free per node, but O(1) unlink | medium — O(1) erase but tombstones saturate table, forces frequent rehash | slow — backshift at 0.9 load means shifting ~5-10 elements per erase | medium — same as chaining |

At 0.9, chaining's degradation is more graceful than open addressing. Linear probing's clustering is catastrophic. Robin Hood is better than linear probing but the swap/shift costs are high. This is where chaining closes the gap or wins.

## Key predictions

- **Load 0.5**: Linear probing wins almost everything. Not enough collisions for Robin Hood's complexity to pay off.
- **Load 0.75**: Robin Hood starts pulling ahead on find-heavy workloads. Linear probing still wins insert and erase-heavy churn.
- **Load 0.9**: Chaining becomes competitive with open addressing. Linear probing's clustering is devastating. Robin Hood is better than linear probing but its swap/shift overhead is high.
- **Across all loads**: std::unordered_map should be consistently slowest — it's chaining with extra generality overhead.
- **The crossover**: there should be a load factor somewhere between 0.75 and 0.9 where chaining overtakes linear probing on find workloads. Robin Hood should delay that crossover.
- **Erase-heavy churn**: linear probing's O(1) tombstone deletion should beat Robin Hood's backshift at every load factor, but the gap widens at higher load.
