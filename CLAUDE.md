# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

Uses CMake presets — always use these instead of raw cmake commands.

```bash
# Configure / Build / Test
cmake --preset debug && cmake --build --preset debug && ctest --preset debug

# Run a single test
ctest --preset debug -R "test_name_pattern"

# Run a single test verbose
ctest --preset debug -R "test_name_pattern" -V

# Run a benchmark (build release first)
cmake --preset release && cmake --build --preset release
./build-release/<module>/src/<bench_name>

# Pre-push validation (what the hook runs)
cmake --preset check && cmake --build --preset check && ctest --preset check
```

Three presets: **debug** (`build/`), **release** (`build-release/`), **check** (`build-check/`, Release + `-Werror`).

## Project Structure

Four topic directories, each with `src/` containing headers, tests, and benchmarks:

- **fundamentals/** — DP, graphs, binary search, two pointers/sliding window
- **compiler_systems/** — tries (multiple implementations), union-find, word search
- **performance_oriented/** — hash map internals, cache-friendly structures, sorting
- **concurrency/** — atomics, lock-free structures, synchronization (theory only so far)

Each problem follows the pattern:
- `problem.hpp` — header-only implementation (inline functions or templates)
- `problem_test.cpp` — Google Test suite, linked against `GTest::gtest_main`
- `problem_bench.cpp` (optional) — Google Benchmark, linked against `benchmark::benchmark`

CMakeLists.txt in each `src/` registers tests with `add_test()`.

## Code Style

- **Formatting:** `.clang-format` with LLVM base style. Pre-commit hook auto-formats staged files.
- **Linting:** `.clang-tidy` runs on pre-push (excludes `_test.cpp` and `_bench.cpp`).
- **Braces:** always required for control flow — no braceless if/for/while.
- **No end-of-line comments.**
- **Naming:**
  - Classes/structs: `UpperCamelCase`
  - Functions/methods: `snake_case`
  - Variables and member variables: `snake_case` (private members with trailing underscore `snake_case_`)
  - Template parameters: `UpperCamelCase`
  - Enums: `UpperCamelCase::UPPERCASE_ENTRIES`
  - Constants/constexpr: `UPPERCASE`
  - Type aliases: `UpperCamelCase` (always `using`, never `typedef`)
- **Headers:** `#pragma once`, header-only with `inline` or templates.
- **Casts:** explicit `static_cast<>` for narrowing/sign conversions (project compiles with `-Wconversion`).

## Git Hooks

Located in `.githooks/`. Pre-commit runs clang-format; pre-push does a full check build + tests + clang-tidy. Hooks block on failure.
