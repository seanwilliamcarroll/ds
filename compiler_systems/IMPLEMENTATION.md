# Compiler / Systems — Implementation Notes

## Build Setup

- **Build system:** CMake (≥ 3.20), C++20
- **Test framework:** GoogleTest (fetched via `FetchContent`, pinned to v1.15.2)
- **Structure:** header-only implementations in `src/`, with corresponding `_test.cpp` files

### Building and running tests

```bash
cmake -B build -S .       # from repo root
cmake --build build
ctest --test-dir build
```

## Approach

Implementations are stubs first, then filled in incrementally. Each header is
self-contained — no dependencies beyond the standard library. Tests start simple
and grow as the implementation does.

## Things to Try

- **Catch2** as an alternative test framework — header-only, arguably nicer
  assertion syntax (`REQUIRE`, `SECTION`-based test structure). Worth comparing
  ergonomics with GoogleTest after a few implementations.
- **Benchmarking:** Google Benchmark (`benchmark` library) integrates well with
  the GoogleTest CMake setup. Useful for comparing path compression strategies
  or measuring amortized cost.