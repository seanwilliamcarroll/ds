**Note:** This analysis is stale — it incorrectly claims RH escapes via early
termination. With correct miss keys, RH is also slow (2.46M). The updated
analysis is in `posts/hash_map_baseline.md`.

# LP Cluster Contagion: Miss Keys Probe Through Displaced Entries

## The finding

Normal miss keys have 0% home slot overlap with hit keys, yet LP FindMiss is 60x
slower than sequential and 5x slower than uniform at N=65536. Probe counting reveals
the cause:

| N | avg probes/miss | max probes/miss | false hits |
|---|---|---|---|
| 256 | 1.4 | 53 | 0 |
| 512 | 1.6 | 105 | 0 |
| 4096 | 4.6 | 839 | 0 |
| 32768 | 32.2 | 11,056 | 0 |
| 65536 | 64.7 | 23,425 | 0 |

All lookups correctly return nullopt. They just have to probe through thousands of
occupied slots to get there.

## Why

The normal hit keys cluster around slots N/2 ± N/4. With LP, collisions at popular
home slots don't stay local — each displaced key lands in the next slot, pushing the
next occupant further. The cluster grows in one direction, eventually spanning a huge
contiguous occupied run. At N=65536 (table=131072), the cluster extends across 23K+
contiguous slots — roughly 18% of the table.

Miss keys hash to home slots outside the hit cluster's home slot range (verified:
0% overlap). But LP displacement means those "empty" home slots are now filled with
displaced hit keys. A miss lookup lands in the occupied run and has to probe forward
until it reaches the far edge.

## Why chaining is immune

Chaining collisions are vertical (longer chains at the same bucket), not horizontal
(spilling into neighbors). A miss key that hashes to an empty bucket returns
immediately, no matter how dense the neighboring buckets are. The hot zone stays
contained.

## What this means

The normal distribution doesn't just make *your* keys slow to find — it pollutes
the entire table, making *unrelated* keys slow too. LP's cluster is contagious. This
is the same displacement mechanism that makes FindHit/Normal catastrophic (521M ns
at N=65536), but applied to keys that aren't even in the table.

This also explains why the warm-up experiment showed no improvement: the slowdown
isn't cold cache lines, it's genuine O(cluster_size) probing.

## Ruled out

- **Cold cache lines**: warm-up pass made no difference (2.17M → 2.17M)
- **Table size mismatch**: dump tool and actual map agree on capacity (131072)
- **Home slot overlap**: 0% — miss keys genuinely hash to different slots than hit keys
- **False hits**: 0 — all miss lookups correctly return nullopt
