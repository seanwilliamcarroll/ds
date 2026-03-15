#!/usr/bin/env python3
"""Generate interactive plotly reports from Google Benchmark CSV output.

Usage:
    python3 bench_report.py raw results.csv [results2.csv ...]
    python3 bench_report.py speedup results.csv --baseline-type PtrTrie

Each CSV should be --benchmark_format=csv output from Google Benchmark.
Benchmark names are expected to follow the pattern:
    BM_Function<Type>/Size
"""

import argparse
import re
import sys
from pathlib import Path

import pandas as pd
import plotly.graph_objects as go
from plotly.subplots import make_subplots

COLORS = [
    "#636EFA", "#EF553B", "#00CC96", "#AB63FA",
    "#FFA15A", "#19D3F3", "#FF6692", "#B6E880",
]


def parse_benchmark_name(name: str) -> dict:
    """Extract function, type, and size from a benchmark name like
    BM_InsertDense<IndexArenaTrie>/256."""
    match = re.match(r"^(.+)<(.+)>/(\d+)$", name)
    if not match:
        return {"function": name, "type": "", "size": 0}
    return {
        "function": match.group(1),
        "type": match.group(2),
        "size": int(match.group(3)),
    }


def pick_unit(series: pd.Series) -> tuple[str, float]:
    """Pick a single display unit for an entire series of ns values."""
    max_val = series.max()
    if max_val >= 1e6:
        return "ms", 1e6
    if max_val >= 1e3:
        return "μs", 1e3
    return "ns", 1


def load_csvs(paths: list[str]) -> pd.DataFrame:
    """Load one or more Google Benchmark CSVs and parse benchmark names."""
    frames = []
    for path in paths:
        df = pd.read_csv(path)
        df["source"] = Path(path).stem
        frames.append(df)
    combined = pd.concat(frames, ignore_index=True)

    parsed = combined["name"].apply(parse_benchmark_name).apply(pd.Series)
    combined = pd.concat([combined, parsed], axis=1)
    return combined


def build_raw_report(df: pd.DataFrame) -> go.Figure:
    """Build a plotly figure with raw CPU times, one subplot per benchmark function."""
    functions = df["function"].unique()
    n = len(functions)

    fig = make_subplots(
        rows=n,
        cols=1,
        subplot_titles=list(functions),
        vertical_spacing=0.08,
    )

    for i, func in enumerate(functions, start=1):
        func_df = df[df["function"] == func]
        types = func_df["type"].unique()
        unit, divisor = pick_unit(func_df["cpu_time"])

        for j, typ in enumerate(types):
            typ_df = func_df[func_df["type"] == typ].sort_values("size")
            fig.add_trace(
                go.Scatter(
                    x=typ_df["size"],
                    y=typ_df["cpu_time"] / divisor,
                    mode="lines+markers",
                    name=typ,
                    legendgroup=typ,
                    showlegend=(i == 1),
                    line=dict(color=COLORS[j % len(COLORS)]),
                    hovertemplate=(
                        f"<b>{typ}</b><br>"
                        f"size: %{{x}}<br>"
                        f"time: %{{y:.2f}} {unit}<br>"
                        "<extra></extra>"
                    ),
                ),
                row=i,
                col=1,
            )

        fig.update_xaxes(title_text="N", type="log", row=i, col=1)
        fig.update_yaxes(title_text=f"CPU time ({unit})", row=i, col=1)

    fig.update_layout(
        height=350 * n,
        title_text="Benchmark Report — Raw CPU Times",
        template="plotly_white",
        hovermode="x unified",
    )

    return fig


def build_speedup_report(df: pd.DataFrame, baseline: str) -> go.Figure:
    """Build a plotly figure with grouped bar charts showing speedup
    relative to baseline type. Each subplot is one benchmark function,
    bars are grouped by size with one bar per type."""
    functions = df["function"].unique()
    n = len(functions)

    fig = make_subplots(
        rows=n,
        cols=1,
        subplot_titles=list(functions),
        vertical_spacing=0.08,
    )

    for i, func in enumerate(functions, start=1):
        func_df = df[df["function"] == func]
        types = func_df["type"].unique()
        sizes = sorted(func_df["size"].unique())

        baseline_df = func_df[func_df["type"] == baseline].set_index("size")

        # Ensure baseline type appears first (leftmost bar in each group)
        ordered_types = [baseline] + [t for t in types if t != baseline]

        for j, typ in enumerate(ordered_types):
            typ_df = func_df[func_df["type"] == typ].set_index("size")
            speedups = []
            for size in sizes:
                if size in baseline_df.index and size in typ_df.index:
                    speedups.append(
                        baseline_df.loc[size, "cpu_time"]
                        / typ_df.loc[size, "cpu_time"]
                    )
                else:
                    speedups.append(None)

            fig.add_trace(
                go.Bar(
                    x=[str(s) for s in sizes],
                    y=speedups,
                    name=typ,
                    legendgroup=typ,
                    showlegend=(i == 1),
                    marker_color=COLORS[j % len(COLORS)],
                    hovertemplate=(
                        f"<b>{typ}</b><br>"
                        f"N=%{{x}}<br>"
                        f"speedup: %{{y:.2f}}x<br>"
                        "<extra></extra>"
                    ),
                ),
                row=i,
                col=1,
            )

        fig.update_xaxes(title_text="N", row=i, col=1)
        fig.update_yaxes(title_text=f"Speedup vs {baseline}", row=i, col=1)

        fig.add_hline(
            y=1.0, line_dash="dash", line_color="gray",
            opacity=0.5, row=i, col=1,
        )

    fig.update_layout(
        height=350 * n,
        title_text=f"Benchmark Report — Speedup vs {baseline}",
        template="plotly_white",
        barmode="group",
    )

    return fig


def main():
    parser = argparse.ArgumentParser(
        description="Generate plotly reports from Google Benchmark CSV output."
    )
    subparsers = parser.add_subparsers(dest="mode", required=True)

    raw_parser = subparsers.add_parser("raw", help="Raw CPU time report")
    raw_parser.add_argument("csvs", nargs="+", help="Google Benchmark CSV files")

    speedup_parser = subparsers.add_parser(
        "speedup", help="Speedup report relative to a baseline type"
    )
    speedup_parser.add_argument("csvs", nargs="+", help="Google Benchmark CSV files")
    speedup_parser.add_argument(
        "--baseline-type", required=True,
        help="Type to use as the 1.0x baseline"
    )

    args = parser.parse_args()
    df = load_csvs(args.csvs)
    stem = Path(args.csvs[0]).stem

    if args.mode == "raw":
        fig = build_raw_report(df)
        out = f"{stem}_raw.html"
        fig.write_html(out)
        print(f"Report written to {out}")

    elif args.mode == "speedup":
        types = list(df["type"].unique())
        if args.baseline_type not in types:
            print(
                f"Error: '{args.baseline_type}' not found. "
                f"Available types: {', '.join(types)}",
                file=sys.stderr,
            )
            sys.exit(1)
        fig = build_speedup_report(df, args.baseline_type)
        out = f"{stem}_speedup.html"
        fig.write_html(out)
        print(f"Report written to {out}")


if __name__ == "__main__":
    main()
