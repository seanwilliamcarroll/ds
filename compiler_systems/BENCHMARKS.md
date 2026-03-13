# Benchmarks

All benchmarks run with Google Benchmark on Apple Silicon (10-core, L2 4096 KiB per core).

## Union-Find: Path Compression vs Union by Rank

Four variants compared:

- **None** — no optimizations
- **Rank only** — union by rank, no path compression
- **Compression only** — path compression, no union by rank
- **Both** — path compression + union by rank

### Adversarial chain: single find after worst-case construction

Builds a chain via `unite(i, i-1)` for all `i`, then calls `find` once on the deepest node.

**Debug:**

| Variant | 1K | 32K | 256K |
|---------|------|------|------|
| None | 8.5us | 270us | 2.1ms |
| Rank only | 4.6us | 131us | 1.0ms |
| Compression only | 12.9us | 410us | 3.3ms |
| Both | 4.6us | 131us | 1.0ms |

**Release:**

| Variant | 1K | 32K | 256K |
|---------|------|------|------|
| None | 1.3us | 12.5us | 83us |
| Rank only | 424ns | 1.1us | 1.2us |
| Compression only | 2.5us | 24us | 146us |
| Both | 422ns | 1.1us | 1.2us |

Rank dominates here. It prevents the deep chain from forming, so the single `find` has
less to walk. Compression only is actually *slower* than no optimization because it pays
the cost of rewriting parent pointers during the walk but only gets called once — the
flattening never pays off.

Release is ~6-25x faster than debug overall, but the relative story is the same. At 256K
with rank, the find is essentially constant-time (~1.2us regardless of 32K or 256K) because
the tree is kept shallow.

### Repeated find: same node queried many times after chain construction

Same chain setup, but `find` is called once before timing to trigger compression (if enabled),
then measured on subsequent calls.

**Debug:**

| Variant | 1K | 32K | 256K |
|---------|------|------|------|
| None | 4.2us | 135us | 1.1ms |
| Rank only | 3.7ns | 3.7ns | 3.7ns |
| Compression only | 5.9ns | 5.9ns | 5.9ns |
| Both | 3.8ns | 3.9ns | 3.9ns |

**Release:**

| Variant | 1K | 32K | 256K |
|---------|------|------|------|
| None | 267ns | 8.5us | 67us |
| Rank only | 0.25ns | 0.25ns | 0.25ns |
| Compression only | 0.53ns | 0.58ns | 0.54ns |
| Both | 0.40ns | 0.37ns | 0.38ns |

The most dramatic result. Without optimization, every `find` re-walks the full chain — time
scales linearly with `n`. With either optimization, repeated finds are effectively O(1)
regardless of size. Rank keeps trees shallow from the start; compression flattens them after
the first walk.

In release, the optimized variants hit sub-nanosecond times (~0.25ns for rank only),
suggesting the compiler may be partially hoisting the result. Regardless, the O(1) vs
O(n) gap is unmistakable.

### Random workload: n unites + n finds with varied indices

Typical usage pattern — not adversarial.

**Debug:**

| Variant | 1K | 32K | 256K |
|---------|------|------|------|
| None | 71us | 2.4ms | 19.6ms |
| Rank only | 30us | 971us | 7.8ms |
| Compression only | 26us | 802us | 6.3ms |
| Both | 26us | 778us | 6.2ms |

**Release:**

| Variant | 1K | 32K | 256K |
|---------|------|------|------|
| None | 6.4us | 248us | 2.0ms |
| Rank only | 2.3us | 79us | 622us |
| Compression only | 2.1us | 61us | 471us |
| Both | 2.2us | 68us | 527us |

Compression matters more than rank for random workloads. Random unite order doesn't build
worst-case trees, so rank has less to prevent. Compression still benefits from flattening
repeated find paths.

Release shows ~10x speedup across the board — the optimizer helps uniformly since
Union-Find is just array operations (no abstraction overhead to eliminate).

### Union-Find Takeaways

- **Rank alone** gives the biggest improvement for adversarial/worst-case inputs by preventing
  deep trees from forming.
- **Compression alone** gives the biggest improvement for workloads with repeated queries on
  the same nodes.
- **Both together** is the standard choice — rank prevents worst-case tree shapes, compression
  amortizes repeated lookups. The combination achieves O(α(n)) amortized per operation.
- For a single find on a pathological input, compression is overhead with no payoff. The
  optimization only helps when you query the same paths multiple times.
- **Debug vs release** is a uniform ~10x speedup. Union-Find uses flat arrays, so there's no
  abstraction overhead for the optimizer to strip — it just generates tighter loop code.

---

## Trie: Arena Allocation vs unique_ptr Children

Two trie implementations compared:

- **ArenaTrie** — nodes stored in a flat `vector<unique_ptr<Node>>`, children are raw `Node*` pointers
- **PtrTrie** — each node owns its children via `array<unique_ptr<Node>, 26>`

Dense words share long prefixes (digits mod 10); sparse words spread across the full alphabet (mod 26).

### Insert

**Debug:**

| Benchmark | 256 | 4K | 64K |
|-----------|-----|-----|------|
| Arena Dense | 73us | 941us | 11.6ms |
| Ptr Dense | 265us | 3.1ms | 41.5ms |
| Arena Sparse | 81us | 1.1ms | 16.2ms |
| Ptr Sparse | 317us | 4.1ms | 58.8ms |

**Release:**

| Benchmark | 256 | 4K | 64K |
|-----------|-----|-----|------|
| Arena Dense | 19us | 243us | 3.1ms |
| Ptr Dense | 44us | 542us | 12.1ms |
| Arena Sparse | 21us | 303us | 4.7ms |
| Ptr Sparse | 51us | 677us | 19.8ms |

Arena is ~3.5x faster in debug, ~4x faster in release. Each PtrTrie insert does a separate
heap allocation per new node (`make_unique`). ArenaTrie pushes into a contiguous vector — fewer
allocations and better locality during construction. This gap is fundamental (allocation
strategy), not something the optimizer can erase.

### Search (hits and misses)

**Debug:**

| Benchmark | 256 | 4K | 64K |
|-----------|-----|-----|------|
| Arena Hit | 16us | 269us | 7.8ms |
| Ptr Hit | 18us | 305us | 8.2ms |
| Arena Miss | 6.6us | 104us | 1.6ms |
| Ptr Miss | 7.0us | 110us | 1.8ms |

**Release:**

| Benchmark | 256 | 4K | 64K |
|-----------|-----|-----|------|
| Arena Hit | 1.0us | 20us | 1.1ms |
| Ptr Hit | 1.1us | 19us | 993us |
| Arena Miss | 84ns | 1.3us | 20us |
| Ptr Miss | 77ns | 1.1us | 17us |

Search performance is nearly identical in both builds. Each lookup follows a single path
through the trie — the access pattern is sequential pointer chasing either way. In release,
PtrTrie is actually marginally faster on misses. The compiler optimizes `unique_ptr::get()`
to the same code as a raw pointer dereference. Misses are faster than hits because they bail
early at the root (searching "zzzzzzz" in a trie of "a"-prefixed words).

Overall search is ~7-8x faster in release vs debug.

### getWordsWithPrefix

**Debug:**

| Benchmark | 256 | 4K | 64K |
|-----------|-----|-----|------|
| Arena | 744ns | 2.9us | 22us |
| Ptr | 854ns | 3.3us | 25us |

**Release:**

| Benchmark | 256 | 4K | 64K |
|-----------|-----|-----|------|
| Arena | 73ns | 317ns | 2.1us |
| Ptr | 73ns | 314ns | 2.1us |

In debug, arena is ~13% faster — the DFS visits many nodes, and unoptimized unique_ptr
operations add overhead. In release, the gap disappears entirely. The optimizer eliminates
the unique_ptr abstraction cost, making traversal patterns equivalent. Both are ~10x faster
in release vs debug.

### Trie Takeaways

- **Arena wins on construction** — ~3.5-4x faster inserts due to fewer heap allocations. This
  holds across debug and release builds because it's a fundamental difference in allocation
  strategy, not something the optimizer can erase.
- **Search is equivalent** in both builds. The compiler optimizes `unique_ptr::get()` to a
  raw pointer dereference, so the traversal cost is identical.
- **Prefix collection is equivalent in release.** The debug-build advantage for arena was
  just overhead from unoptimized unique_ptr operations, not true cache locality benefits.
- Arena's downside: removed nodes can't be freed individually (zombie nodes leak memory).
  PtrTrie can prune dead branches on removal.
- **Choose arena** for build-once-query-many workloads (dictionaries, autocomplete indices).
  **Choose unique_ptr** when the trie is long-lived with frequent insert/remove churn.
