# Hash Map Numbers Update Plan

After switching from Apple Clang (Xcode) to LLVM-21 (MacPorts) via CMakePresets.json,
all benchmark numbers need to be rerun and updated for consistency.

## Benchmarks to rerun

```bash
# Nuke stale build dirs (old compiler cached)
rm -rf build build-release build-check

# Rebuild with LLVM-21
cmake --preset release && cmake --build --preset release

# Timing benchmarks (all scenarios)
./build-release/performance_oriented/src/hash_map_bench --benchmark_format=csv > final_hashmap_bench_release.csv

# Memory benchmarks
./build-release/performance_oriented/src/hash_map_mem_bench --benchmark_format=csv > final_hashmap_mem_bench_release.csv
```

Also need to rerun the experiments that use LP variants not in the main bench registration
(Fibonacci hash, prefetching). These are commented out in the REGISTER macros but can be
run by uncommenting or with a separate build.

## Update order

### Step 1: Final scoreboard (definitive source)

**`hash_map_final_scoreboard.md`** — ~12 tables + summary + winner table.
Update from the new CSV run. This is the single source of truth for the
all-implementations comparison.

### Step 2: Per-experiment primary docs (6 files)

Each of these is the source of truth for a specific experiment. Update with
rerun data, or note if the experiment needs a separate benchmark run.

| File | Experiment | Needs separate run? |
|---|---|---|
| `hash_map_bench_results.md` | Original 4-impl comparison (5 tables + prose) | No — covered by main bench |
| `hash_map_memory_analysis.md` | bytes/entry (2 tables + prose) | No — covered by mem bench |
| `hash_map_robin_hood_v2_analysis.md` | vector\<bool\> vs vector\<uint8_t\> (~8 tables) | No — both variants in main bench |
| `hash_map_robin_hood_three_way_analysis.md` | uint8_t vs bool vs stored PD (~8 tables) | No — all three in main bench |
| `hash_map_fibonacci_hash_analysis.md` | LP identity vs Fibonacci hash (5 tables) | **Yes** — Fibonacci LP is commented out in bench macros |
| `hash_map_prefetching_analysis.md` | LP with prefetching (3 tables) | **Yes** — prefetch LP is commented out in bench macros |

For the Fibonacci and prefetching docs: either uncomment the LP variants temporarily
and rerun, or note in the doc that numbers are from Apple Clang. Since both experiments
showed clear results (Fibonacci is a tradeoff, prefetching doesn't help), the qualitative
conclusions are unlikely to change — but exact numbers may shift.

### Step 3: Reference docs (2 files)

These cite numbers from the primary docs. Update last to match.

| File | What to update |
|---|---|
| `hash_map_blog_plan.md` | Ratios in post outlines, "New Findings" section numbers |
| `hash_map_perf_improvements.md` | Prose ratios citing findings from experiments |

### Step 4: Verify no numbers to update (2 files)

- `hash_map_bench_hypotheses.md` — qualitative only, no numbers
- `hash_map_bench_scenarios.md` — hardware constants only (cache sizes, table sizes)

## Known inconsistencies to fix

- `hash_map_perf_findings.md` and `hash_map_fibonacci_hash_analysis.md` have slightly
  different numbers for the same Fibonacci hash comparison (different runs). Unify from
  a single LLVM-21 run.

## Reproducibility doc

After rerunning, create `hash_map_bench_environment.md` capturing:
- Compiler: LLVM/Clang 21.1.8 (MacPorts, /opt/local/libexec/llvm-21/bin/clang++)
- Flags: -O3 -DNDEBUG (CMake Release), -Wall -Wextra -Wpedantic -Wshadow -Wconversion
- C++ standard: C++20
- CMake: 3.31.10
- Machine: Apple M4, 24GB RAM
- OS: macOS 26.3 (Darwin 25.3.0)
- Cache: P-core L1d 128KB, P-core L2 16MB, 128-byte cache lines
- No process pinning or turbo boost configuration
