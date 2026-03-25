#!/usr/bin/env python3
"""Extract and summarize hash map benchmark data from CSV files."""

import csv
import re
from collections import defaultdict

def parse_name(name):
    """Parse benchmark name into (scenario, impl, N)"""
    m = re.match(r'"?BM_(\w+)<(.+?)>/(\d+)"?', name)
    if not m:
        return None, None, None
    scenario = m.group(1)
    impl_str = m.group(2)
    n = int(m.group(3))
    return scenario, impl_str, n

def short_name(impl):
    """Shorten implementation name for table display."""
    replacements = [
        ('ChainingPoolHashMap', 'ChainPool'),
        ('ChainingHashMap', 'Chain'),
        ('RobinHoodStoredDistHashMap', 'RobinStoredDist'),
        ('RobinHoodHashMap', 'Robin'),
        ('LinearProbingMergedStructHashMap', 'LinearMerged'),
        ('LinearProbingHashMap', 'Linear'),
        ('StdUnorderedMapAdapter', 'std::umap'),
    ]
    s = impl
    for old, new in replacements:
        s = s.replace(old, new)
    return s

def load_of(impl):
    """Extract load factor from impl string."""
    m = re.search(r'<(\d+\.?\d*)', impl)
    if m:
        return m.group(1)
    return '?'

# ---- Load data ----
timing = {}
with open('final_hashmap_bench_release.csv') as f:
    reader = csv.DictReader(f)
    for row in reader:
        scenario, impl, n = parse_name(row['name'])
        if scenario is None:
            continue
        timing[(scenario, impl, n)] = float(row['cpu_time'])

memory = {}
with open('final_hashmap_mem_bench_release.csv') as f:
    reader = csv.DictReader(f)
    for row in reader:
        scenario, impl, n = parse_name(row['name'])
        if scenario is None:
            continue
        # Find bytes_per_entry column (may have quotes in key)
        bpe = None
        for k, v in row.items():
            if 'bytes_per_entry' in k and v:
                bpe = float(v)
                break
        if bpe is not None:
            memory[(scenario, impl, n)] = bpe

# ---- Discover structure ----
all_impls = sorted(set(impl for _, impl, _ in timing.keys()))
all_scenarios = sorted(set(s for s, _, _ in timing.keys()))

print("ALL IMPLEMENTATIONS:")
for impl in all_impls:
    print(f"  {impl}")

print("\nALL SCENARIOS:")
for s in all_scenarios:
    ns = sorted(set(n for sc, _, n in timing.keys() if sc == s))
    print(f"  {s}: N={ns}")

# ---- Helper to print markdown table ----
def print_table(scenario, impls, n_val, data_dict, value_fmt="{:.0f}"):
    """Print a markdown table for a single scenario at a given N."""
    rows = []
    for impl in impls:
        val = data_dict.get((scenario, impl, n_val))
        if val is not None:
            rows.append((short_name(impl), val))
    if not rows:
        return
    rows.sort(key=lambda x: x[1])
    best = rows[0][1]
    print(f"| Implementation | cpu_time (ns) | vs best |")
    print(f"|---|---:|---:|")
    for name, val in rows:
        ratio = val / best if best > 0 else 0
        print(f"| {name} | {value_fmt.format(val)} | {ratio:.2f}x |")

def print_multi_n_table(scenario, impls, ns, data_dict):
    """Print a markdown table across multiple N values."""
    # Filter to impls that have data
    valid_impls = [impl for impl in impls if any(data_dict.get((scenario, impl, n)) is not None for n in ns)]
    if not valid_impls:
        return
    header = "| Implementation | " + " | ".join(f"N={n//1024}K" if n >= 1024 else f"N={n}" for n in ns) + " |"
    sep = "|---|" + "|".join("---:" for _ in ns) + "|"
    print(header)
    print(sep)
    for impl in valid_impls:
        vals = []
        for n in ns:
            v = data_dict.get((scenario, impl, n))
            if v is not None:
                if v >= 1e6:
                    vals.append(f"{v/1e6:.2f}M")
                elif v >= 1e3:
                    vals.append(f"{v/1e3:.1f}K")
                else:
                    vals.append(f"{v:.0f}")
            else:
                vals.append("-")
        print(f"| {short_name(impl)} | " + " | ".join(vals) + " |")

# ---- Categorize implementations by load factor ----
impls_075 = [i for i in all_impls if '0.75' in i or ('0.7' in i and '0.75' in i)]
impls_05 = [i for i in all_impls if '<0.5' in i or ', 0.5' in i]
impls_09 = [i for i in all_impls if '0.9' in i]

# If no 0.75, check what loads exist
if not impls_075:
    loads = set()
    for impl in all_impls:
        m = re.search(r'[\d.]+', impl)
        if m:
            loads.add(m.group())
    print(f"\nAvailable loads: {loads}")
    # Try the most common load
    impls_075 = [i for i in all_impls if '0.75' in i]

print("\n\n# Hash Map Benchmark Results\n")

# ========================================
# SECTION 1: Core scenarios at N=65536, load 0.75
# ========================================
core_scenarios = ['Insert', 'FindHit', 'FindMiss', 'EraseAndFind', 'EraseChurn']

print("## Core Scenarios at N=65536, Load Factor 0.75\n")
for scenario in core_scenarios:
    available = [(s, i, n) for (s, i, n) in timing.keys() if s == scenario and n == 65536 and '0.75' in i]
    if available:
        print(f"### {scenario}\n")
        print_table(scenario, impls_075, 65536, timing)
        print()

# ========================================
# SECTION 1b: Core scenarios at other loads
# ========================================
for load_label, impls_load in [("0.5", impls_05), ("0.9", impls_09)]:
    print(f"## Core Scenarios at N=65536, Load Factor {load_label}\n")
    for scenario in core_scenarios:
        available = [(s, i, n) for (s, i, n) in timing.keys() if s == scenario and n == 65536 and i in impls_load]
        if available:
            print(f"### {scenario}\n")
            print_table(scenario, impls_load, 65536, timing)
            print()

# ========================================
# SECTION 2: Access pattern scenarios at N=65536, load 0.75
# ========================================
pattern_scenarios = ['Insert_Random', 'FindHit_Random', 'Insert_Strided', 'FindHit_Strided']

print("## Access Pattern Scenarios at N=65536, Load Factor 0.75\n")
for scenario in pattern_scenarios:
    available = [(s, i, n) for (s, i, n) in timing.keys() if s == scenario and n == 65536 and '0.75' in i]
    if available:
        print(f"### {scenario}\n")
        print_table(scenario, impls_075, 65536, timing)
        print()

# ========================================
# SECTION 3: Large-scale scenarios
# ========================================
large_scenarios = ['Insert_Large', 'FindHit_Large', 'FindHit_Random_Large']

print("## Large-Scale Scenarios (Load Factor 0.75)\n")
for scenario in large_scenarios:
    ns_for_scenario = sorted(set(n for (s, i, n) in timing.keys() if s == scenario and '0.75' in i))
    if ns_for_scenario:
        print(f"### {scenario}\n")
        print_multi_n_table(scenario, impls_075, ns_for_scenario, timing)
        print()

# ========================================
# SECTION 4: Memory (bytes_per_entry at N=65536)
# ========================================
print("## Memory: bytes_per_entry at N=65536\n")

# Get all impls that have memory data at N=65536
mem_impls = sorted(set(i for (s, i, n) in memory.keys() if n == 65536))
if mem_impls:
    print("| Implementation | bytes_per_entry |")
    print("|---|---:|")
    rows = []
    for impl in mem_impls:
        # Use Insert scenario for memory measurement
        val = memory.get(('Insert', impl, 65536))
        if val is not None:
            rows.append((short_name(impl), val))
    rows.sort(key=lambda x: x[1])
    for name, val in rows:
        print(f"| {name} | {val:.1f} |")
    print()

# Also show memory at multiple N values for 0.75 load
print("## Memory: bytes_per_entry across N values (Load Factor 0.75)\n")
mem_ns = sorted(set(n for (s, i, n) in memory.keys() if '0.75' in i and s == 'Insert'))
if mem_ns:
    mem_impls_075 = sorted(set(i for (s, i, n) in memory.keys() if '0.75' in i and s == 'Insert'))
    print("| Implementation | " + " | ".join(f"N={n//1024}K" if n >= 1024 else f"N={n}" for n in mem_ns) + " |")
    print("|---|" + "|".join("---:" for _ in mem_ns) + "|")
    for impl in mem_impls_075:
        vals = []
        for n in mem_ns:
            v = memory.get(('Insert', impl, n))
            vals.append(f"{v:.1f}" if v is not None else "-")
        print(f"| {short_name(impl)} | " + " | ".join(vals) + " |")
    print()
