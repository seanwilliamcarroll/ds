#!/usr/bin/env python3
"""Compare old (Apple Clang) vs new (LLVM-21) benchmark numbers.

Highlights significant changes (>5% difference) to guide narrative updates
in the analysis markdown files.
"""

# Old Apple Clang numbers (from previous scoreboard, before compiler switch)
OLD = {
    # Core scenarios at N=65536
    # Insert
    ("Insert", "LP", 0.5): 110000, ("Insert", "LP", 0.75): 116000, ("Insert", "LP", 0.9): 121000,
    ("Insert", "RH", 0.5): 183000, ("Insert", "RH", 0.75): 196000, ("Insert", "RH", 0.9): 203000,
    ("Insert", "RH-V2", 0.5): 203000, ("Insert", "RH-V2", 0.75): 222000, ("Insert", "RH-V2", 0.9): 240000,
    ("Insert", "RH-bool", 0.5): 264000, ("Insert", "RH-bool", 0.75): 311000, ("Insert", "RH-bool", 0.9): 337000,
    ("Insert", "ChainV2", 0.5): 305000, ("Insert", "ChainV2", 0.75): 334000, ("Insert", "ChainV2", 0.9): 350000,
    ("Insert", "std", 0.5): 1220000, ("Insert", "std", 0.75): 1170000, ("Insert", "std", 0.9): 1200000,
    ("Insert", "Chain", 0.5): 1810000, ("Insert", "Chain", 0.75): 2230000, ("Insert", "Chain", 0.9): 2400000,
    # FindHit
    ("FindHit", "LP", 0.5): 45400, ("FindHit", "LP", 0.75): 45400, ("FindHit", "LP", 0.9): 45400,
    ("FindHit", "ChainV2", 0.5): 46600, ("FindHit", "ChainV2", 0.75): 44000, ("FindHit", "ChainV2", 0.9): 48800,
    ("FindHit", "std", 0.5): 97300, ("FindHit", "std", 0.75): 113600, ("FindHit", "std", 0.9): 97400,
    # FindMiss
    ("FindMiss", "RH", 0.5): 32500, ("FindMiss", "RH", 0.75): 32500, ("FindMiss", "RH", 0.9): 32500,
    ("FindMiss", "RH-V2", 0.5): 32400, ("FindMiss", "RH-V2", 0.75): 32400, ("FindMiss", "RH-V2", 0.9): 32600,
    ("FindMiss", "RH-bool", 0.5): 34100, ("FindMiss", "RH-bool", 0.75): 34100, ("FindMiss", "RH-bool", 0.9): 34100,
    ("FindMiss", "LP", 0.5): 35800, ("FindMiss", "LP", 0.75): 44700, ("FindMiss", "LP", 0.9): 48600,
    ("FindMiss", "Chain", 0.5): 36400, ("FindMiss", "Chain", 0.75): 36300, ("FindMiss", "Chain", 0.9): 35800,
    ("FindMiss", "ChainV2", 0.5): 36400, ("FindMiss", "ChainV2", 0.75): 36500, ("FindMiss", "ChainV2", 0.9): 35800,
    ("FindMiss", "std", 0.5): 61000, ("FindMiss", "std", 0.75): 86000, ("FindMiss", "std", 0.9): 86100,
    # EraseAndFind
    ("EraseAndFind", "LP", 0.75): 20900,
    ("EraseAndFind", "Chain", 0.75): 296000,
    ("EraseAndFind", "std", 0.75): 339000,
    # Hash quality
    ("Insert_Random", "LP", 0.75): 727000,
    ("Insert_Random", "ChainV2", 0.75): 986000,
    ("FindHit_Random", "LP", 0.75): 115000,
    ("FindHit_Random", "RH", 0.75): 155000,
    ("FindHit_Strided", "std", 0.75): 98700,
    # Large scale
    ("Insert_Large", "Chain", 0.75, 4194304): 148000000,
    ("FindHit_Random_Large", "LP", 0.75, 4194304): 35900000,
    ("FindHit_Random_Large", "Chain", 0.75, 4194304): 46600000,
    ("FindHit_Random_Large", "std", 0.75, 4194304): 103000000,
}

# New LLVM-21 numbers (from bench_extract.py output)
NEW = {
    # Insert
    ("Insert", "LP", 0.5): 112124, ("Insert", "LP", 0.75): 117440, ("Insert", "LP", 0.9): 124948,
    ("Insert", "RH", 0.5): 185044, ("Insert", "RH", 0.75): 197062, ("Insert", "RH", 0.9): 203755,
    ("Insert", "RH-V2", 0.5): 201433, ("Insert", "RH-V2", 0.75): 223829, ("Insert", "RH-V2", 0.9): 238973,
    ("Insert", "RH-bool", 0.5): 265410, ("Insert", "RH-bool", 0.75): 312293, ("Insert", "RH-bool", 0.9): 338333,
    ("Insert", "ChainV2", 0.5): 308028, ("Insert", "ChainV2", 0.75): 339022, ("Insert", "ChainV2", 0.9): 355445,
    ("Insert", "std", 0.5): 1195600, ("Insert", "std", 0.75): 1153150, ("Insert", "std", 0.9): 1195330,
    ("Insert", "Chain", 0.5): 1837350, ("Insert", "Chain", 0.75): 2274990, ("Insert", "Chain", 0.9): 2544960,
    # FindHit
    ("FindHit", "LP", 0.5): 45429, ("FindHit", "LP", 0.75): 45423, ("FindHit", "LP", 0.9): 45433,
    ("FindHit", "ChainV2", 0.5): 46648, ("FindHit", "ChainV2", 0.75): 46281, ("FindHit", "ChainV2", 0.9): 46617,
    ("FindHit", "std", 0.5): 97343, ("FindHit", "std", 0.75): 98990, ("FindHit", "std", 0.9): 102250,
    # FindMiss
    ("FindMiss", "RH", 0.5): 32446, ("FindMiss", "RH", 0.75): 32450, ("FindMiss", "RH", 0.9): 32454,
    ("FindMiss", "RH-V2", 0.5): 32464, ("FindMiss", "RH-V2", 0.75): 32459, ("FindMiss", "RH-V2", 0.9): 32464,
    ("FindMiss", "RH-bool", 0.5): 32583, ("FindMiss", "RH-bool", 0.75): 32482, ("FindMiss", "RH-bool", 0.9): 32486,
    ("FindMiss", "LP", 0.5): 48650, ("FindMiss", "LP", 0.75): 38356, ("FindMiss", "LP", 0.9): 35700,
    ("FindMiss", "Chain", 0.5): 36133, ("FindMiss", "Chain", 0.75): 35690, ("FindMiss", "Chain", 0.9): 36263,
    ("FindMiss", "ChainV2", 0.5): 35237, ("FindMiss", "ChainV2", 0.75): 36096, ("FindMiss", "ChainV2", 0.9): 36197,
    ("FindMiss", "std", 0.5): 48685, ("FindMiss", "std", 0.75): 76659, ("FindMiss", "std", 0.9): 80244,
    # EraseAndFind
    ("EraseAndFind", "LP", 0.75): 20918,
    ("EraseAndFind", "Chain", 0.75): 312021,
    ("EraseAndFind", "std", 0.75): 347278,
    # Hash quality
    ("Insert_Random", "LP", 0.75): 703675,
    ("Insert_Random", "ChainV2", 0.75): 1043650,
    ("FindHit_Random", "LP", 0.75): 118838,
    ("FindHit_Random", "RH", 0.75): 151345,
    ("FindHit_Strided", "std", 0.75): 100877,
    # Large scale
    ("Insert_Large", "Chain", 0.75, 4194304): 156430000,
    ("FindHit_Random_Large", "LP", 0.75, 4194304): 35890000,
    ("FindHit_Random_Large", "Chain", 0.75, 4194304): 47180000,
    ("FindHit_Random_Large", "std", 0.75, 4194304): 103000000,
}


def main():
    print("Significant changes (>5%) from Apple Clang to LLVM-21:")
    print()

    significant = []
    for key in sorted(OLD.keys()):
        if key in NEW:
            o, n = OLD[key], NEW[key]
            pct = (n - o) / o * 100
            if abs(pct) > 5:
                significant.append((key, o, n, pct))

    for key, o, n, pct in significant:
        label = f"{key[0]} {key[1]} load={key[2]}"
        if len(key) > 3:
            label += f" N={key[3]}"
        print(f"  {label}: {o:.0f} -> {n:.0f} ({pct:+.1f}%)")

    print()
    print("NARRATIVE IMPACT:")
    print()

    # Check FindMiss LP pattern
    print("FindMiss LP load sensitivity REVERSED:")
    for lf in [0.5, 0.75, 0.9]:
        o = OLD.get(("FindMiss", "LP", lf))
        n = NEW.get(("FindMiss", "LP", lf))
        if o and n:
            print(f"  load {lf}: {o:.0f} -> {n:.0f}")
    print("  Old: LP degraded 35.8K -> 48.6K at higher load (clustering)")
    print("  New: LP is 48.6K at LOW load, 35.7K at HIGH load (inverted!)")
    print()

    # Check FindMiss RH-bool
    print("FindMiss RH-bool now tied with RH:")
    for lf in [0.5, 0.75, 0.9]:
        o = OLD.get(("FindMiss", "RH-bool", lf))
        n = NEW.get(("FindMiss", "RH-bool", lf))
        if o and n:
            print(f"  load {lf}: {o:.0f} -> {n:.0f}")
    print("  Old: RH-bool was 34.1K (5% slower than RH)")
    print("  New: RH-bool is 32.5K (tied with RH)")
    print()

    # Check FindHit std
    print("FindHit std more consistent across loads:")
    for lf in [0.5, 0.75, 0.9]:
        o = OLD.get(("FindHit", "std", lf))
        n = NEW.get(("FindHit", "std", lf))
        if o and n:
            print(f"  load {lf}: {o:.0f} -> {n:.0f}")
    print("  Old: 97K / 114K / 97K (anomalous spike at 0.75)")
    print("  New: 97K / 99K / 102K (consistent)")


if __name__ == "__main__":
    main()
