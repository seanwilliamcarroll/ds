# Benchmarks

## Union-Find: Path Compression vs Union by Rank

Measured with Google Benchmark (debug build, Apple Silicon). Four variants compared:

- **None** — no optimizations
- **Rank only** — union by rank, no path compression
- **Compression only** — path compression, no union by rank
- **Both** — path compression + union by rank

### Adversarial chain: single find after worst-case construction

Builds a chain via `unite(i, i-1)` for all `i`, then calls `find` once on the deepest node.

| Variant | 1K | 32K | 1M |
|---------|------|------|------|
| None | 8.6us | 270us | 8.5ms |
| Rank only | 4.6us | 132us | 4.2ms |
| Compression only | 13.0us | 413us | 13.1ms |
| Both | 4.6us | 132us | 4.2ms |

Rank dominates here. It prevents the deep chain from forming, so the single `find` has
less to walk. Compression only is actually *slower* than no optimization because it pays
the cost of rewriting parent pointers during the walk but only gets called once — the
flattening never pays off.

### Repeated find: same node queried many times after chain construction

Same chain setup, but `find` is called once before timing to trigger compression (if enabled),
then measured on subsequent calls.

| Variant | 1K | 32K | 1M |
|---------|------|------|------|
| None | 4.3us | 136us | 4.4ms |
| Rank only | 3.6ns | 3.6ns | 3.7ns |
| Compression only | 5.9ns | 5.9ns | 6.1ns |
| Both | 3.8ns | 3.9ns | 3.8ns |

The most dramatic result. Without optimization, every `find` re-walks the full chain — time
scales linearly with `n`. With either optimization, repeated finds are effectively O(1)
regardless of size. Rank keeps trees shallow from the start; compression flattens them after
the first walk.

### Random workload: n unites + n finds with varied indices

Typical usage pattern — not adversarial.

| Variant | 1K | 32K | 1M |
|---------|------|------|------|
| None | 71us | 2.4ms | 78ms |
| Rank only | 30us | 980us | 33ms |
| Compression only | 27us | 813us | 26ms |
| Both | 26us | 783us | 25ms |

Compression matters more than rank for random workloads. Random unite order doesn't build
worst-case trees, so rank has less to prevent. Compression still benefits from flattening
repeated find paths.

### Takeaways

- **Rank alone** gives the biggest improvement for adversarial/worst-case inputs by preventing
  deep trees from forming.
- **Compression alone** gives the biggest improvement for workloads with repeated queries on
  the same nodes.
- **Both together** is the standard choice — rank prevents worst-case tree shapes, compression
  amortizes repeated lookups. The combination achieves O(a(n)) amortized per operation.
- For a single find on a pathological input, compression is overhead with no payoff. The
  optimization only helps when you query the same paths multiple times.
