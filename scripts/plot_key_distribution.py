#!/usr/bin/env python3
"""Visualize key slot distributions from key_distribution_dump output.

Usage:
    ./key_distribution_dump > keys.csv
    python scripts/plot_key_distribution.py keys.csv
    python scripts/plot_key_distribution.py keys.csv --pattern normal
    python scripts/plot_key_distribution.py keys.csv --pattern normal --n 4096
"""

import argparse
import csv
import sys
from collections import Counter

import plotly.graph_objects as go
from plotly.subplots import make_subplots


def load_csv(path):
    """Load key dump CSV into list of dicts."""
    rows = []
    with open(path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            row["n"] = int(row["n"])
            row["table_size"] = int(row["table_size"])
            row["key"] = int(row["key"])
            row["slot"] = int(row["slot"])
            rows.append(row)
    return rows


def plot_distribution(rows, pattern, n):
    """Plot hit vs miss slot distributions for one (pattern, n) combo."""
    table_size = rows[0]["table_size"]
    hit_slots = [r["slot"] for r in rows if r["type"] == "hit"]
    miss_slots = [r["slot"] for r in rows if r["type"] == "miss"]

    bin_size = max(1, table_size // 256)

    fig = make_subplots(
        rows=2, cols=1,
        subplot_titles=("Hit keys (in table)", "Miss keys (not in table)"),
        shared_xaxes=True,
        vertical_spacing=0.08,
    )

    fig.add_trace(
        go.Histogram(x=hit_slots, xbins=dict(start=0, end=table_size, size=bin_size),
                      marker_color="steelblue", opacity=0.8, name="Hit"),
        row=1, col=1,
    )
    fig.add_trace(
        go.Histogram(x=miss_slots, xbins=dict(start=0, end=table_size, size=bin_size),
                      marker_color="coral", opacity=0.8, name="Miss"),
        row=2, col=1,
    )

    fig.update_layout(
        title_text=f"{pattern} keys — N={n:,}, table_size={table_size:,}",
        height=500, showlegend=False,
    )
    fig.update_xaxes(title_text="Slot", row=2, col=1)
    fig.update_yaxes(title_text="Count", row=1, col=1)
    fig.update_yaxes(title_text="Count", row=2, col=1)

    fig.show()


def print_overlap_stats(rows, pattern, n):
    """Print stats about how much miss keys overlap with occupied slots."""
    table_size = rows[0]["table_size"]
    hit_slots = Counter(r["slot"] for r in rows if r["type"] == "hit")
    miss_slots = Counter(r["slot"] for r in rows if r["type"] == "miss")

    occupied = set(hit_slots.keys())
    miss_in_occupied = sum(miss_slots[s] for s in occupied)
    total_miss = sum(miss_slots.values())

    print(f"\n{pattern} N={n:,} (table={table_size:,}):")
    print(f"  Occupied slots: {len(occupied)} / {table_size} ({100*len(occupied)/table_size:.1f}%)")
    print(f"  Miss keys hashing to occupied slot: {miss_in_occupied} / {total_miss} ({100*miss_in_occupied/total_miss:.1f}%)")

    # For normal: show the densest region
    if pattern == "normal" and hit_slots:
        peak_slot = hit_slots.most_common(1)[0][0]
        # Count hits in a window around peak
        window = table_size // 8
        nearby_hits = sum(
            v for s, v in hit_slots.items()
            if (s - peak_slot) % table_size < window
            or (peak_slot - s) % table_size < window
        )
        nearby_misses = sum(
            v for s, v in miss_slots.items()
            if (s - peak_slot) % table_size < window
            or (peak_slot - s) % table_size < window
        )
        print(f"  Hot zone (±{window} slots around peak {peak_slot}):")
        print(f"    Hits: {nearby_hits}, Misses: {nearby_misses}")


def main():
    parser = argparse.ArgumentParser(description="Visualize key slot distributions")
    parser.add_argument("csv_file", help="CSV from key_distribution_dump")
    parser.add_argument("--pattern", help="Filter to one pattern (sequential, uniform, normal)")
    parser.add_argument("--n", type=int, help="Filter to one N value")
    parser.add_argument("--no-plot", action="store_true", help="Print stats only, no plots")
    args = parser.parse_args()

    rows = load_csv(args.csv_file)

    # Get unique (pattern, n) combos
    combos = sorted(set((r["pattern"], r["n"]) for r in rows))

    if args.pattern:
        combos = [(p, n) for p, n in combos if p == args.pattern]
    if args.n:
        combos = [(p, n) for p, n in combos if n == args.n]

    if not combos:
        print("No matching data found.", file=sys.stderr)
        sys.exit(1)

    for pattern, n in combos:
        subset = [r for r in rows if r["pattern"] == pattern and r["n"] == n]
        print_overlap_stats(subset, pattern, n)

        if not args.no_plot:
            plot_distribution(subset, pattern, n)


if __name__ == "__main__":
    main()
