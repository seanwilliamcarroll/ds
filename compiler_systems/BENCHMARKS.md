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

## Trie: Arena Allocation vs unique_ptr Children (Debug Build)

Two trie implementations compared:

- **ArenaTrie** — nodes stored in a flat `vector<unique_ptr<Node>>`, children are raw `Node*` pointers
- **PtrTrie** — each node owns its children via `array<unique_ptr<Node>, 26>`

Dense words share long prefixes (digits mod 10); sparse words spread across the full alphabet (mod 26).

### Insert

| Benchmark | 256 | 4K | 64K |
|-----------|-----|-----|------|
| Arena Dense | 72us | 936us | 11.7ms |
| Ptr Dense | 263us | 3.1ms | 40.4ms |
| Arena Sparse | 81us | 1.1ms | 16.3ms |
| Ptr Sparse | 299us | 4.1ms | 56.8ms |

Arena is ~3.5x faster on inserts across the board. Each PtrTrie insert does a separate heap
allocation per new node (`make_unique`). ArenaTrie pushes into a contiguous vector — fewer
allocations and better locality during construction.

### Search (hits and misses)

| Benchmark | 256 | 4K | 64K |
|-----------|-----|-----|------|
| Arena Hit | 16us | 270us | 6.9ms |
| Ptr Hit | 18us | 288us | 6.9ms |
| Arena Miss | 6.5us | 104us | 1.7ms |
| Ptr Miss | 6.9us | 110us | 1.8ms |

Search performance is nearly identical. Each lookup follows a single path through the trie —
the access pattern is sequential pointer chasing either way, so the arena's contiguous layout
doesn't help much. Misses are faster because they bail early at the root (searching "zzzzzzz"
in a trie of "a"-prefixed words).

### getWordsWithPrefix

| Benchmark | 256 | 4K | 64K |
|-----------|-----|-----|------|
| Arena | 724ns | 3.0us | 21us |
| Ptr | 824ns | 3.2us | 24us |

Arena is ~15% faster. This DFS visits many nodes, so cache locality starts to matter. The gap
would likely widen with larger tries where more cache lines are touched.

### Takeaways

- **Arena wins on construction** — 3.5x faster inserts due to fewer heap allocations.
- **Search is equivalent** — single-path traversal doesn't benefit much from layout.
- **Prefix collection shows a small arena advantage** from cache locality during DFS.
- Arena's downside: removed nodes can't be freed individually (zombie nodes leak memory).
  PtrTrie can prune dead branches on removal.
- **Choose arena** for build-once-query-many workloads (dictionaries, autocomplete indices).
  **Choose unique_ptr** when the trie is long-lived with frequent insert/remove churn.
