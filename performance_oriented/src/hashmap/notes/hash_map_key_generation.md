# Benchmark Key Generation

## Three distributions

All benchmarks use three key distributions to stress different hash map behaviors:

- **Sequential** (`[0, N)`): best case for identity hash — keys fill contiguous slots with no collisions. Miss keys are `[N, 2N)`, which map to the empty half of the table.
- **Uniform** (shuffled from `[0, 10N)`): scattered across the table, exercising the general case. Miss keys use a prime offset (`+1,000,003`), which works reliably because occupied slots are scattered.
- **Normal** (`N(N/2, N/8)` + `X * SLOT_STRIDE`): pathological clustering. The `SLOT_STRIDE = 1<<20` trick makes `X` vanish in the slot computation (`key & (table_size-1)`) for any table up to 1M slots, so `normal_part` alone determines the slot. ~95% of keys cluster within N/4 slots of center.

## Normal miss keys

Miss keys for the normal distribution are generated independently (not offset from hit keys) using a normal distribution centered at `3N/2` instead of `N/2`. With `table_size ~ 2N`, this places the miss cluster on the opposite side of the table — N slots (50% of table) from the hit cluster. The 95% ranges (`[N/4, 3N/4]` hits vs `[5N/4, 7N/4]` misses) don't overlap.

The original approach — adding a fixed prime offset to each hit key — produced wildly non-monotonic FindMiss results because `(offset mod table_size)` varies with table size. For some N values the offset landed miss keys inside the hit cluster (68% overlap at N=4096), for others it landed them in empty space (0% at N=512). This made FindMiss benchmark noise, not signal.

## Verification

`key_distribution_dump.cpp` generates keys using the exact same code paths as the benchmarks and writes `(pattern, n, table_size, type, key, slot)` tuples to CSV. `scripts/plot_key_distribution.py` reads this CSV and reports overlap statistics (what fraction of miss keys hash to occupied slots) and optionally plots slot histograms via plotly. After the fix, all normal N values show 0% miss-to-hit slot overlap.

```bash
cmake --build --preset debug --target key_distribution_dump
./build/performance_oriented/src/hashmap/key_distribution_dump > /tmp/keys.csv
python3 scripts/plot_key_distribution.py /tmp/keys.csv --pattern normal --no-plot
```
