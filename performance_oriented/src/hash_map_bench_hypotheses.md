# Hash Map Benchmark Hypotheses

Predictions for each scenario before running benchmarks.
Scale: fast / medium / slow (relative to other implementations in that scenario).

| Scenario | Chaining | Linear Probing | Robin Hood | std::unordered_map |
|---|---|---|---|---|
| Insert | medium — pointer chasing on collision, but no probe storms | fast — just walk to next empty slot, no swaps | medium — swap overhead on every collision even when load is low | slow — same chaining pain plus stdlib overhead (allocator, more generality) |
| Find hit | slow — pointer chase per chain node, cache-hostile | medium — linear scan is cache-friendly, but clusters cause long probes | fast — probe lengths are uniform, early termination if probe distance exceeds occupant's | slow — same pointer chasing as chaining, plus stdlib indirection |
| Find miss | slow — must walk entire chain to confirm miss | medium — probes until empty slot, clusters make this worse than hit | fast — early termination kicks in sooner for misses (probe distance exceeds occupant's quickly) | slow — same as chaining |
| Erase + find survivors | medium — erase is just unlink, find still pointer-chases | slow — tombstones accumulate, surviving finds probe through dead slots | fast — backshift keeps probe chains clean, survivor finds stay short | medium — same as chaining |
| High load find (~0.9) | medium — chains get longer but each bucket is independent, degrades linearly | slow — primary clustering is brutal at high load, very long probe runs | medium — probe lengths stay uniform but swaps during insert to reach 0.9 were expensive, finds still decent | medium — similar to chaining, maybe slightly better due to stdlib tuning |
| Erase-heavy churn | medium — unlink is cheap, but constant malloc/free for nodes | fast — tombstone erase is O(1) flag flip, rehash cleans up periodically | slow — every erase does backshift (O(k) shifts), repeated churn amplifies this | medium — same alloc churn as chaining |

## Key predictions

- **Robin Hood should dominate find-heavy scenarios** due to uniform probe lengths and early termination.
- **Linear probing should win insert-only** — simplest probe logic, no swap overhead.
- **Linear probing should suffer on erase + find** — tombstone accumulation degrades all subsequent operations.
- **Robin Hood should suffer on erase-heavy churn** — backshift deletion is O(k) per erase vs O(1) tombstone flip.
- **Chaining should be consistently mediocre** — never terrible, never great. Pointer chasing is the constant tax.
- **std::unordered_map should be consistently slowest** — chaining implementation with extra generality overhead (hash policy, allocator, type erasure).
- **High load is where open addressing hurts most** — both linear probing and Robin Hood degrade, chaining holds up better.
