#!/usr/bin/env python3
"""Extract and summarize hash map pattern benchmark results from CSV.

Handles repeated runs (--benchmark_repetitions=N): uses the _median rows
when present, falls back to raw rows for single-run data.

Usage:
  ./build-release/performance_oriented/src/hash_map_pattern_bench \
      --benchmark_format=csv --benchmark_repetitions=10 > pattern_bench.csv
  python3 scripts/pattern_bench_extract.py pattern_bench.csv
"""

import csv
import re
import sys

PATTERNS = ["SEQUENTIAL", "UNIFORM", "NORMAL"]
PATTERN_SHORT = {"SEQUENTIAL": "Seq", "UNIFORM": "Uniform", "NORMAL": "Normal"}

IMPL_SHORT = [
    ("ChainingHashMap", "Chain"),
    ("RobinHoodHashMap", "RH"),
    ("LinearProbingHashMap", "LP"),
    ("StdUnorderedMapAdapter", "std"),
]


def short_name(impl):
    s = impl
    for old, new in IMPL_SHORT:
        s = s.replace(old, new)
    return s


def parse_name(name):
    """Parse benchmark name into (scenario, impl, pattern, N, suffix).

    suffix is None for raw rows, or 'mean'/'median'/'stddev'/'cv' for aggregates.
    Examples:
      'BM_Insert<LP<0.75>, KeyPattern::NORMAL>/65536'         -> suffix=None
      'BM_Insert<LP<0.75>, KeyPattern::NORMAL>/65536_median'  -> suffix='median'
    """
    m = re.match(
        r'"?BM_(\w+)<(.+?),\s*KeyPattern::(\w+)>/(\d+)(?:_(mean|median|stddev|cv))?"?',
        name,
    )
    if not m:
        return None, None, None, None, None
    scenario = m.group(1)
    impl = m.group(2).strip()
    pattern = m.group(3)
    n = int(m.group(4))
    suffix = m.group(5)
    return scenario, impl, pattern, n, suffix


def fmt_ns(val):
    """Format nanosecond value for display."""
    if val >= 1e6:
        return f"{val / 1e6:.2f}M"
    elif val >= 1e3:
        return f"{val / 1e3:.1f}K"
    else:
        return f"{val:.1f}"


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <csv_file>", file=sys.stderr)
        sys.exit(1)

    # Load data — prefer median rows, fall back to raw (single-run) data
    medians = {}
    raws = {}
    stddevs = {}
    with open(sys.argv[1]) as f:
        reader = csv.DictReader(f)
        for row in reader:
            scenario, impl, pattern, n, suffix = parse_name(row["name"])
            if scenario is None:
                continue
            key = (scenario, impl, pattern, n)
            val = float(row["cpu_time"])
            if suffix == "median":
                medians[key] = val
            elif suffix == "stddev":
                stddevs[key] = val
            elif suffix is None:
                raws[key] = val  # last raw row wins (fine for single-run)

    data = medians if medians else raws
    has_stddev = bool(stddevs)

    # Discover structure
    scenarios = sorted(set(s for s, _, _, _ in data))
    impls = sorted(set(i for _, i, _, _ in data))
    ns = sorted(set(n for _, _, _, n in data))
    max_n = max(ns)

    source = "median of repetitions" if medians else "single run"
    print(f"# Pattern Benchmark Results (N={max_n}, {source})\n")

    # For each scenario, show a table: impl x pattern, at max N
    for scenario in scenarios:
        print(f"## {scenario}\n")

        rows = []
        for impl in impls:
            row_data = {}
            row_cv = {}
            for pat in PATTERNS:
                key = (scenario, impl, pat, max_n)
                val = data.get(key)
                if val is not None:
                    row_data[pat] = val
                sd = stddevs.get(key)
                if sd is not None and val:
                    row_cv[pat] = sd / val * 100  # CV as percentage
            if row_data:
                rows.append((impl, row_data, row_cv))

        if not rows:
            print("(no data)\n")
            continue

        # Sort by sequential time (or first available)
        def sort_key(r):
            for pat in PATTERNS:
                if pat in r[1]:
                    return r[1][pat]
            return float("inf")

        rows.sort(key=sort_key)

        # Print table
        if has_stddev:
            print(
                "| Implementation | Seq | Uniform | Normal | U/S | N/S | CV% (S/U/N) |"
            )
            print("|---|---:|---:|---:|---:|---:|---|")
        else:
            print("| Implementation | Seq | Uniform | Normal | U/S | N/S |")
            print("|---|---:|---:|---:|---:|---:|")

        for impl, vals, cvs in rows:
            seq = vals.get("SEQUENTIAL")
            uni = vals.get("UNIFORM")
            norm = vals.get("NORMAL")
            seq_s = fmt_ns(seq) if seq else "-"
            uni_s = fmt_ns(uni) if uni else "-"
            norm_s = fmt_ns(norm) if norm else "-"
            uni_ratio = f"{uni / seq:.2f}x" if (uni and seq) else "-"
            norm_ratio = f"{norm / seq:.2f}x" if (norm and seq) else "-"
            line = f"| {short_name(impl)} | {seq_s} | {uni_s} | {norm_s} | {uni_ratio} | {norm_ratio} |"
            if has_stddev:
                cv_parts = []
                for pat in PATTERNS:
                    cv = cvs.get(pat)
                    cv_parts.append(f"{cv:.1f}" if cv is not None else "-")
                line += f" {'/'.join(cv_parts)} |"
            print(line)
        print()

    # Cross-pattern comparison: for each pattern, rank implementations
    print("---\n")
    print("## Rankings by Pattern\n")
    for pat in PATTERNS:
        print(f"### {PATTERN_SHORT.get(pat, pat)} Keys\n")
        print("| Scenario | Winner | 2nd | 3rd | 4th |")
        print("|---|---|---|---|---|")
        for scenario in scenarios:
            ranked = []
            for impl in impls:
                val = data.get((scenario, impl, pat, max_n))
                if val is not None:
                    ranked.append((short_name(impl), val))
            ranked.sort(key=lambda x: x[1])
            cells = []
            for i, (name, val) in enumerate(ranked):
                if i == 0:
                    cells.append(f"**{name}** ({fmt_ns(val)})")
                else:
                    ratio = val / ranked[0][1]
                    cells.append(f"{name} ({ratio:.2f}x)")
            while len(cells) < 4:
                cells.append("-")
            print(f"| {scenario} | " + " | ".join(cells) + " |")
        print()


if __name__ == "__main__":
    main()
