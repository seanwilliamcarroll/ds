# Hash Map Benchmark Environment

All benchmark numbers in this directory were collected in a single session on 2026-03-23
under the following conditions.

## Compiler

- **Toolchain:** LLVM/Clang 21.1.8 (MacPorts)
- **Path:** `/opt/local/libexec/llvm-21/bin/clang++`
- **C++ standard:** C++20
- **Optimization:** `-O3 -DNDEBUG` (CMake Release preset)
- **Warnings:** `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`

## Build

- **CMake:** 3.31.10
- **Preset:** `release` (see `CMakePresets.json`)
- **Build dir:** `build-release/`

## Machine

- **CPU:** Apple M4
- **RAM:** 24 GB
- **OS:** macOS 26.3 (Darwin 25.3.0)
- **Cache:**
  - P-core L1d: 128 KB
  - P-core L2: 16 MB
  - Cache line: 128 bytes

## Benchmark framework

- Google Benchmark (via CMake FetchContent)
- Output format: `--benchmark_format=csv`

## Process configuration

- No process pinning (macOS does not expose `taskset`-style affinity)
- No turbo boost configuration (Apple Silicon does not expose frequency controls)
- Background processes not explicitly killed, but machine was otherwise idle

## Raw data

- `final_hashmap_bench_release.csv` — timing benchmarks (all implementations, all scenarios)
- `final_hashmap_mem_bench_release.csv` — memory benchmarks (bytes per entry)

## Previous compiler note

Numbers collected before 2026-03-23 used Apple Clang (Xcode). Notable differences
with LLVM-21:
- LP FindMiss load sensitivity reversed (was worst at high load, now worst at low load)
- RH-bool FindMiss penalty disappeared (~5% slower with Apple Clang, now tied)
- std::unordered_map FindHit more consistent across load factors (anomalous spike at
  0.75 with Apple Clang eliminated)
