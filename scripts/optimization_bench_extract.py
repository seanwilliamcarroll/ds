#!/usr/bin/env python3
"""Extract and compare A/B optimization pair results from benchmark CSV.

Usage:
    python scripts/optimization_bench_extract.py <csv_file> [--pair FILTER]

Examples:
    python scripts/optimization_bench_extract.py results.csv
    python scripts/optimization_bench_extract.py results.csv --pair Merged
    python scripts/optimization_bench_extract.py results.csv --pair Pool
"""

import argparse
import csv
import re
import sys
from collections import defaultdict

# Maps benchmark struct names to (pair_name, side) for grouping.
# Side "A" is the baseline, "B" is the optimization.
PAIRS = {
    "LinearProbingHashMap_SoA": ("AoS", "A"),
    "LinearProbingHashMap_MergedStruct": ("AoS", "B"),
    "RobinHoodHashMap_RecomputedDist": ("StoredDist", "A"),
    "RobinHoodHashMap_StoredDist": ("StoredDist", "B"),
    "LinearProbingHashMap_NoPrefetch": ("Prefetch", "A"),
    "LinearProbingHashMap_Prefetch": ("Prefetch", "B"),
    "RobinHoodHashMap_BoolState": ("BoolState", "A"),
    "RobinHoodHashMap_Uint8State": ("BoolState", "B"),
    "ChainingHashMap_UniquePtr": ("Pool", "A"),
    "ChainingHashMap_PoolAllocator": ("Pool", "B"),
}

PAIR_DESCRIPTIONS = {
    "AoS": ("AoS Merge", "SoA (baseline)", "Merged struct"),
    "StoredDist": ("Stored Probe Distance", "Recomputed (baseline)", "Stored"),
    "Prefetch": ("Prefetching", "Off (baseline)", "On"),
    "BoolState": ("vector<bool> vs uint8_t", "Bool (baseline)", "Uint8"),
    "Pool": ("Pool Allocator", "UniquePtr (baseline)", "Pool"),
}

PATTERN_NAMES = {
    "KeyPattern::SEQUENTIAL": "Seq",
    "KeyPattern::UNIFORM": "Uniform",
    "KeyPattern::NORMAL": "Normal",
}


def parse_name(name):
    """Parse benchmark name into (impl, scenario, pattern, n, aggregate_type).

    Returns None for aggregate_type on raw data rows, or 'median'/'mean'/'stddev'.
    """
    name = name.strip('"')

    # Check for aggregate suffix
    agg = None
    for suffix in ("_median", "_mean", "_stddev", "_cv"):
        if name.endswith(suffix):
            agg = suffix[1:]  # strip leading underscore
            name = name[: -len(suffix)]
            break

    m = re.match(r"BM_(\w+)<([^,]+),\s*([^>]+)>/(\d+)", name)
    if not m:
        return None
    scenario = m.group(1)
    impl = m.group(2)
    pattern = m.group(3).strip()
    n = int(m.group(4))
    return impl, scenario, pattern, n, agg


def load_csv(path):
    """Load benchmark CSV, returning {(impl, scenario, pattern, n): cpu_time}.

    Prefers median rows. Falls back to first raw row per combination.
    """
    raw = {}
    medians = {}

    with open(path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            parsed = parse_name(row["name"])
            if parsed is None:
                continue
            impl, scenario, pattern, n, agg = parsed
            key = (impl, scenario, pattern, n)
            cpu_time = float(row["cpu_time"])

            if agg == "median":
                medians[key] = cpu_time
            elif agg is None and key not in raw:
                raw[key] = cpu_time

    # Prefer median, fall back to first raw
    result = {}
    for key in raw.keys() | medians.keys():
        result[key] = medians.get(key, raw.get(key))
    return result


def fmt_time(ns):
    """Format nanoseconds for display."""
    if ns >= 1e9:
        return f"{ns / 1e9:.2f}s"
    if ns >= 1e6:
        return f"{ns / 1e6:.1f}M"
    if ns >= 1e3:
        return f"{ns / 1e3:.1f}K"
    return f"{ns:.1f}"


def fmt_ratio(a_time, b_time):
    """Format B/A ratio. <1 means B is faster."""
    if a_time == 0:
        return "—"
    ratio = b_time / a_time
    if ratio < 1:
        return f"**{1/ratio:.2f}x faster**"
    if ratio > 1:
        return f"{ratio:.2f}x slower"
    return "tied"


def print_pair(pair_name, data, target_n=None):
    """Print comparison table for one optimization pair."""
    desc, a_label, b_label = PAIR_DESCRIPTIONS[pair_name]

    # Find all impls for this pair
    a_impls = [k for k, (p, s) in PAIRS.items() if p == pair_name and s == "A"]
    b_impls = [k for k, (p, s) in PAIRS.items() if p == pair_name and s == "B"]
    if not a_impls or not b_impls:
        return
    a_impl, b_impl = a_impls[0], b_impls[0]

    # Collect all (scenario, pattern, n) combos for this pair
    combos = set()
    for impl, scenario, pattern, n in data:
        if impl in (a_impl, b_impl):
            combos.add((scenario, pattern, n))

    if not combos:
        print(f"\n### {desc}\n\nNo data found.\n")
        return

    # Pick target N (largest available if not specified)
    all_ns = sorted(set(n for _, _, n in combos))
    if target_n is None:
        target_n = all_ns[-1]

    scenarios = ["Insert", "FindHit", "FindMiss", "EraseAndFind"]
    patterns = ["KeyPattern::SEQUENTIAL", "KeyPattern::UNIFORM", "KeyPattern::NORMAL"]

    print(f"\n### {desc} (N={target_n:,})\n")
    print(f"| Scenario | Pattern | {a_label} | {b_label} | Change |")
    print("|---|---|---:|---:|---|")

    for scenario in scenarios:
        for pattern in patterns:
            key_a = (a_impl, scenario, pattern, target_n)
            key_b = (b_impl, scenario, pattern, target_n)
            a_time = data.get(key_a)
            b_time = data.get(key_b)
            pat_short = PATTERN_NAMES.get(pattern, pattern)

            if a_time is not None and b_time is not None:
                print(
                    f"| {scenario} | {pat_short} | "
                    f"{fmt_time(a_time)} | {fmt_time(b_time)} | "
                    f"{fmt_ratio(a_time, b_time)} |"
                )
            elif a_time is not None:
                print(f"| {scenario} | {pat_short} | {fmt_time(a_time)} | — | — |")
            elif b_time is not None:
                print(f"| {scenario} | {pat_short} | — | {fmt_time(b_time)} | — |")


def main():
    parser = argparse.ArgumentParser(description="Extract optimization bench results")
    parser.add_argument("csv_file", help="Benchmark CSV file")
    parser.add_argument("--pair", help="Filter to a specific pair (e.g. AoS, Pool)")
    parser.add_argument("--n", type=int, help="Target N value (default: largest)")
    args = parser.parse_args()

    data = load_csv(args.csv_file)
    if not data:
        print(f"No benchmark data found in {args.csv_file}", file=sys.stderr)
        sys.exit(1)

    pair_names = list(PAIR_DESCRIPTIONS.keys())
    if args.pair:
        pair_names = [p for p in pair_names if args.pair.lower() in p.lower()]
        if not pair_names:
            print(f"No pair matching '{args.pair}'", file=sys.stderr)
            sys.exit(1)

    print("# Optimization Benchmark Results\n")
    for pair_name in pair_names:
        print_pair(pair_name, data, args.n)
        print()


if __name__ == "__main__":
    main()
