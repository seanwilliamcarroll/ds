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
    BM_InsertDense<IndexArenaTrie>/256.

    For nested templates like BM_Insert<ChainingHashMap<0.5>>/256,
    also extracts impl and load as separate fields."""
    match = re.match(r"^([^<]+)<(.+)>/(\d+)$", name)
    if not match:
        return {"function": name, "type": "", "impl": "", "load": "", "size": 0}

    full_type = match.group(2)

    # Try to split nested template: Foo<0.75> -> impl=Foo, load=0.75
    nested = re.match(r"^(\w+)<([^>]+)>$", full_type)
    if nested:
        impl = nested.group(1)
        load = nested.group(2)
    else:
        impl = full_type
        load = ""

    return {
        "function": match.group(1),
        "type": full_type,
        "impl": impl,
        "load": load,
        "size": int(match.group(3)),
    }


def parse_density_benchmark_name(name: str) -> dict:
    """Extract algorithm, n, and density_pct from a benchmark name like
    BM_SPFA/100/25."""
    match = re.match(r"^BM_(\w+)/(\d+)/(\d+)$", name)
    if not match:
        return {"algorithm": name, "n": 0, "density_pct": 0}
    return {
        "algorithm": match.group(1),
        "n": int(match.group(2)),
        "density_pct": int(match.group(3)),
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


def build_raw_report(df: pd.DataFrame, group_by: str = "type") -> go.Figure:
    """Build a plotly figure with raw CPU times, one subplot per benchmark function.

    group_by controls the legend grouping: "type" (default), "impl", or "load".
    """
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
        groups = func_df[group_by].unique()
        unit, divisor = pick_unit(func_df["cpu_time"])

        for j, group in enumerate(groups):
            group_df = func_df[func_df[group_by] == group].sort_values("size")
            fig.add_trace(
                go.Scatter(
                    x=group_df["size"],
                    y=group_df["cpu_time"] / divisor,
                    mode="lines+markers",
                    name=group,
                    legendgroup=group,
                    showlegend=(i == 1),
                    line=dict(color=COLORS[j % len(COLORS)]),
                    hovertemplate=(
                        f"<b>{group}</b><br>"
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
        hovermode="closest",
    )

    return fig


def build_speedup_report(df: pd.DataFrame, baseline: str,
                         group_by: str = "type") -> go.Figure:
    """Build a plotly figure with grouped bar charts showing speedup
    relative to baseline. Each subplot is one benchmark function,
    bars are grouped by size with one bar per group.

    group_by controls grouping: "type" (default), "impl", or "load".
    baseline should match a value in the group_by column.
    """
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
        groups = func_df[group_by].unique()
        sizes = sorted(func_df["size"].unique())

        baseline_df = func_df[func_df[group_by] == baseline].set_index("size")

        # Ensure baseline appears first (leftmost bar in each group)
        ordered_groups = [baseline] + [g for g in groups if g != baseline]

        for j, group in enumerate(ordered_groups):
            group_df = func_df[func_df[group_by] == group].set_index("size")
            speedups = []
            for size in sizes:
                if size in baseline_df.index and size in group_df.index:
                    speedups.append(
                        baseline_df.loc[size, "cpu_time"]
                        / group_df.loc[size, "cpu_time"]
                    )
                else:
                    speedups.append(None)

            fig.add_trace(
                go.Bar(
                    x=[str(s) for s in sizes],
                    y=speedups,
                    name=group,
                    legendgroup=group,
                    showlegend=(i == 1),
                    marker_color=COLORS[j % len(COLORS)],
                    hovertemplate=(
                        f"<b>{group}</b><br>"
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


def load_density_csvs(paths: list[str]) -> pd.DataFrame:
    """Load CSVs with two-parameter benchmark names (BM_Algo/n/density_pct)."""
    frames = []
    for path in paths:
        df = pd.read_csv(path)
        df["source"] = Path(path).stem
        frames.append(df)
    combined = pd.concat(frames, ignore_index=True)
    parsed = combined["name"].apply(parse_density_benchmark_name).apply(pd.Series)
    return pd.concat([combined, parsed], axis=1)


def build_density_report(df: pd.DataFrame) -> go.Figure:
    """One subplot per n value. x-axis = density_pct, y-axis = cpu_time.
    One line per algorithm so the crossover point is visually obvious."""
    ns = sorted(df["n"].unique())
    algorithms = sorted(df["algorithm"].unique())
    n_rows = len(ns)

    fig = make_subplots(
        rows=n_rows,
        cols=1,
        subplot_titles=[f"n = {n}" for n in ns],
        vertical_spacing=0.06,
    )

    for i, n in enumerate(ns, start=1):
        n_df = df[df["n"] == n]
        unit, divisor = pick_unit(n_df["cpu_time"])

        for j, algo in enumerate(algorithms):
            algo_df = n_df[n_df["algorithm"] == algo].sort_values("density_pct")
            fig.add_trace(
                go.Scatter(
                    x=algo_df["density_pct"],
                    y=algo_df["cpu_time"] / divisor,
                    mode="lines+markers",
                    name=algo,
                    legendgroup=algo,
                    showlegend=(i == 1),
                    line=dict(color=COLORS[j % len(COLORS)]),
                    hovertemplate=(
                        f"<b>{algo}</b><br>"
                        f"density: %{{x}}%<br>"
                        f"time: %{{y:.2f}} {unit}<br>"
                        "<extra></extra>"
                    ),
                ),
                row=i,
                col=1,
            )

        fig.update_xaxes(title_text="Density (%)", row=i, col=1)
        fig.update_yaxes(title_text=f"CPU time ({unit})", row=i, col=1)

    fig.update_layout(
        height=300 * n_rows,
        title_text="SPFA vs Dijkstra — CPU time by density",
        template="plotly_white",
        hovermode="closest",
    )

    return fig


def build_by_n_report(df: pd.DataFrame) -> go.Figure:
    """One subplot per density value. x-axis = n (log scale), y-axis = cpu_time.
    One line per algorithm — shows how each scales with n at fixed density."""
    densities = sorted(df["density_pct"].unique())
    algorithms = sorted(df["algorithm"].unique())
    n_rows = len(densities)

    fig = make_subplots(
        rows=n_rows,
        cols=1,
        subplot_titles=[f"density = {d}%" for d in densities],
        vertical_spacing=0.06,
    )

    for i, density in enumerate(densities, start=1):
        d_df = df[df["density_pct"] == density]
        unit, divisor = pick_unit(d_df["cpu_time"])

        for j, algo in enumerate(algorithms):
            algo_df = d_df[d_df["algorithm"] == algo].sort_values("n")
            fig.add_trace(
                go.Scatter(
                    x=algo_df["n"],
                    y=algo_df["cpu_time"] / divisor,
                    mode="lines+markers",
                    name=algo,
                    legendgroup=algo,
                    showlegend=(i == 1),
                    line=dict(color=COLORS[j % len(COLORS)]),
                    hovertemplate=(
                        f"<b>{algo}</b><br>"
                        f"n: %{{x}}<br>"
                        f"time: %{{y:.2f}} {unit}<br>"
                        "<extra></extra>"
                    ),
                ),
                row=i,
                col=1,
            )

        fig.update_xaxes(title_text="n", type="log", row=i, col=1)
        fig.update_yaxes(title_text=f"CPU time ({unit})", row=i, col=1)

    fig.update_layout(
        height=300 * n_rows,
        title_text="SPFA vs Dijkstra — CPU time by n",
        template="plotly_white",
        hovermode="closest",
    )

    return fig


def build_heatmap_report(df: pd.DataFrame) -> go.Figure:
    """Heatmap of SPFA/Dijkstra speedup ratio. x-axis = n, y-axis = density_pct.
    Values > 1 mean Dijkstra is faster; values < 1 mean SPFA is faster.
    Colorscale is centered at 1.0 so the crossover is immediately visible."""
    ns = sorted(df["n"].unique())
    densities = sorted(df["density_pct"].unique())

    spfa = df[df["algorithm"] == "SPFA"].set_index(["n", "density_pct"])["cpu_time"]
    dijkstra = df[df["algorithm"] == "Dijkstra"].set_index(["n", "density_pct"])["cpu_time"]

    # ratio[i][j] = SPFA_time / Dijkstra_time at (density=densities[i], n=ns[j])
    ratio = []
    text = []
    for d in densities:
        row = []
        row_text = []
        for n in ns:
            key = (n, d)
            if key in spfa.index and key in dijkstra.index:
                r = spfa[key] / dijkstra[key]
                row.append(r)
                row_text.append(f"{r:.2f}x")
            else:
                row.append(None)
                row_text.append("")
        ratio.append(row)
        text.append(row_text)

    fig = go.Figure(
        go.Heatmap(
            z=ratio,
            x=[str(n) for n in ns],
            y=[f"{d}%" for d in densities],
            text=text,
            texttemplate="%{text}",
            colorscale="RdBu_r",
            zmid=1.0,
            colorbar=dict(title="SPFA / Dijkstra"),
            hovertemplate=(
                "n: %{x}<br>"
                "density: %{y}<br>"
                "ratio: %{text}<br>"
                "<extra></extra>"
            ),
        )
    )

    fig.update_layout(
        title_text="SPFA / Dijkstra speedup ratio (>1 = Dijkstra faster, <1 = SPFA faster)",
        xaxis_title="n",
        yaxis_title="Density (%)",
        template="plotly_white",
        height=500,
    )

    return fig


def add_filter_args(parser: argparse.ArgumentParser):
    """Add --load and --impl filter flags to a subparser."""
    parser.add_argument(
        "--load", default=None,
        help="Filter to a specific load factor (e.g. 0.75). Lines grouped by impl."
    )
    parser.add_argument(
        "--impl", default=None,
        help="Filter to a specific implementation (e.g. RobinHoodHashMap). Lines grouped by load."
    )


def apply_filters(df: pd.DataFrame, args) -> tuple[pd.DataFrame, str]:
    """Apply --load and --impl filters. Returns (filtered_df, group_by_column).

    --load: filter to rows with that load, group by impl
    --impl: filter to rows with that impl, group by load
    Neither: group by type (original behavior)
    """
    if args.load and args.impl:
        print("Error: --load and --impl are mutually exclusive.", file=sys.stderr)
        sys.exit(1)

    if args.load:
        df = df[df["load"] == args.load]
        if df.empty:
            available = sorted(df["load"].unique()) if not df.empty else []
            print(f"Error: no data for --load {args.load}. Available: {available}",
                  file=sys.stderr)
            sys.exit(1)
        return df, "impl"

    if args.impl:
        df = df[df["impl"] == args.impl]
        if df.empty:
            print(f"Error: no data for --impl {args.impl}.", file=sys.stderr)
            sys.exit(1)
        return df, "load"

    return df, "type"


def main():
    parser = argparse.ArgumentParser(
        description="Generate plotly reports from Google Benchmark CSV output."
    )
    subparsers = parser.add_subparsers(dest="mode", required=True)

    raw_parser = subparsers.add_parser("raw", help="Raw CPU time report")
    raw_parser.add_argument("csvs", nargs="+", help="Google Benchmark CSV files")
    add_filter_args(raw_parser)

    speedup_parser = subparsers.add_parser(
        "speedup", help="Speedup report relative to a baseline type"
    )
    speedup_parser.add_argument("csvs", nargs="+", help="Google Benchmark CSV files")
    speedup_parser.add_argument(
        "--baseline-type", required=True,
        help="Baseline for 1.0x comparison (matches type, impl, or load depending on filters)"
    )
    add_filter_args(speedup_parser)

    density_parser = subparsers.add_parser(
        "density", help="CPU time vs density for two-parameter benchmarks (BM_Algo/n/density)"
    )
    density_parser.add_argument("csvs", nargs="+", help="Google Benchmark CSV files")

    by_n_parser = subparsers.add_parser(
        "by-n", help="CPU time vs n, one subplot per density (BM_Algo/n/density)"
    )
    by_n_parser.add_argument("csvs", nargs="+", help="Google Benchmark CSV files")

    heatmap_parser = subparsers.add_parser(
        "heatmap", help="Heatmap of SPFA/Dijkstra ratio over n × density"
    )
    heatmap_parser.add_argument("csvs", nargs="+", help="Google Benchmark CSV files")

    args = parser.parse_args()
    stem = Path(args.csvs[0]).stem

    if args.mode == "raw":
        df = load_csvs(args.csvs)
        df, group_by = apply_filters(df, args)
        fig = build_raw_report(df, group_by)
        suffix = f"_load{args.load}" if args.load else (f"_{args.impl}" if args.impl else "")
        out = f"{stem}_raw{suffix}.html"
        fig.write_html(out)
        print(f"Report written to {out}")

    elif args.mode == "speedup":
        df = load_csvs(args.csvs)
        df, group_by = apply_filters(df, args)
        groups = list(df[group_by].unique())
        if args.baseline_type not in groups:
            print(
                f"Error: '{args.baseline_type}' not found in {group_by} column. "
                f"Available: {', '.join(str(g) for g in groups)}",
                file=sys.stderr,
            )
            sys.exit(1)
        fig = build_speedup_report(df, args.baseline_type, group_by)
        suffix = f"_load{args.load}" if args.load else (f"_{args.impl}" if args.impl else "")
        out = f"{stem}_speedup{suffix}.html"
        fig.write_html(out)
        print(f"Report written to {out}")

    elif args.mode == "density":
        df = load_density_csvs(args.csvs)
        fig = build_density_report(df)
        out = f"{stem}_density.html"
        fig.write_html(out)
        print(f"Report written to {out}")

    elif args.mode == "by-n":
        df = load_density_csvs(args.csvs)
        fig = build_by_n_report(df)
        out = f"{stem}_by_n.html"
        fig.write_html(out)
        print(f"Report written to {out}")

    elif args.mode == "heatmap":
        df = load_density_csvs(args.csvs)
        fig = build_heatmap_report(df)
        out = f"{stem}_heatmap.html"
        fig.write_html(out)
        print(f"Report written to {out}")


if __name__ == "__main__":
    main()
