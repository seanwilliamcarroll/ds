#!/usr/bin/env python3
"""Extract and summarize hash map pattern benchmark results from CSV.

Usage:
  ./build-release/performance_oriented/src/hash_map_pattern_bench \
      --benchmark_format=csv > pattern_bench.csv
  python3 scripts/pattern_bench_extract.py pattern_bench.csv
"""

import csv
import re
import sys

PATTERNS = ["SEQUENTIAL", "UNIFORM", "NORMAL"]
PATTERN_SHORT = {"SEQUENTIAL": "Seq", "UNIFORM": "Uniform", "Normal": "Normal", "NORMAL": "Normal"}

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
    """Parse 'BM_Insert<LinearProbingHashMap<0.75>, KeyPattern::NORMAL>/65536'
    into (scenario, impl, pattern, N)."""
    m = re.match(
        r'"?BM_(\w+)<(.+?),\s*KeyPattern::(\w+)>/(\d+)"?', name
    )
    if not m:
        return None, None, None, None
    scenario = m.group(1)
    impl = m.group(2).strip()
    pattern = m.group(3)
    n = int(m.group(4))
    return scenario, impl, pattern, n


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

    # Load data
    data = {}  # (scenario, impl, pattern, n) -> cpu_time
    with open(sys.argv[1]) as f:
        reader = csv.DictReader(f)
        for row in reader:
            scenario, impl, pattern, n = parse_name(row["name"])
            if scenario is None:
                continue
            data[(scenario, impl, pattern, n)] = float(row["cpu_time"])

    # Discover structure
    scenarios = sorted(set(s for s, _, _, _ in data))
    impls = sorted(set(i for _, i, _, _ in data))
    ns = sorted(set(n for _, _, _, n in data))
    max_n = max(ns)

    print(f"# Pattern Benchmark Results (N={max_n})\n")

    # For each scenario, show a table: impl x pattern, at max N
    for scenario in scenarios:
        print(f"## {scenario}\n")

        # Collect data for this scenario at max_n
        rows = []
        for impl in impls:
            row_data = {}
            for pat in PATTERNS:
                val = data.get((scenario, impl, pat, max_n))
                if val is not None:
                    row_data[pat] = val
            if row_data:
                rows.append((impl, row_data))

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
        print(f"| Implementation | Seq | Uniform | Normal | Uniform/Seq | Normal/Seq |")
        print(f"|---|---:|---:|---:|---:|---:|")
        for impl, vals in rows:
            seq = vals.get("SEQUENTIAL")
            uni = vals.get("UNIFORM")
            norm = vals.get("NORMAL")
            seq_s = fmt_ns(seq) if seq else "-"
            uni_s = fmt_ns(uni) if uni else "-"
            norm_s = fmt_ns(norm) if norm else "-"
            uni_ratio = f"{uni / seq:.2f}x" if (uni and seq) else "-"
            norm_ratio = f"{norm / seq:.2f}x" if (norm and seq) else "-"
            print(
                f"| {short_name(impl)} | {seq_s} | {uni_s} | {norm_s} | {uni_ratio} | {norm_ratio} |"
            )
        print()

    # Cross-pattern comparison: for each pattern, rank implementations
    print("---\n")
    print("## Rankings by Pattern\n")
    for pat in PATTERNS:
        print(f"### {PATTERN_SHORT.get(pat, pat)} Keys\n")
        print(f"| Scenario | Winner | 2nd | 3rd | 4th |")
        print(f"|---|---|---|---|---|")
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
