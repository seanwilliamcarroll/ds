# Benchmarking Tries, Part 2: What "Arena" Actually Means

**TL;DR:** My first "arena" trie wasn't an arena at all — fixing that revealed real cache locality effects I'd only assumed before.

---

## The mistake

In the [previous post](trie_post.md), I compared two trie implementations: an "arena" that stored nodes in a `vector<unique_ptr<Node>>`, and a `PtrTrie` where each node owns its children via `unique_ptr`. I attributed the arena's insert speedup partly to cache locality — nodes being contiguous in memory.

The problem: `vector<unique_ptr<Node>>` is not an arena. Each `make_unique<Node>()` is a separate heap allocation. The vector holds the *`unique_ptr` objects* contiguously — 8-byte pointers packed together — but the *nodes themselves* are scattered across the heap, exactly like PtrTrie. There may have been some partial cache locality benefit from the contiguous pointer array itself (sequential pointer loads could prefetch well), but the nodes those pointers referred to had no locality guarantees whatsoever. The insert speedup was real (fewer allocation events due to vector's geometric growth vs individual `make_unique` calls), but the cache locality story I told about node layout was wrong.

Once I realized this, the obvious question: what happens when you build a *real* arena?

---

## Three implementations

I built two genuine arena strategies alongside the existing PtrTrie:

**IndexArenaTrie:** Nodes live directly in a `vector<Node>` — actual contiguous memory. Children store `size_t` indices into the vector rather than pointers. Indices survive reallocation, so the vector can grow freely. This is the "real" arena: nodes are physically adjacent in memory. A missing child is represented by a sentinel value (`numeric_limits<size_t>::max()`) — more on this choice [later](#sentinel-vs-zero-as-null).

**DequeArenaTrie:** Nodes live in a `deque<Node>`. A deque allocates in fixed-size chunks — nodes within a chunk are contiguous, but chunks themselves may be scattered. Children store `Node*` pointers, which are safe because `deque` guarantees pointer stability on `push_back`. A middle ground: better locality than scattered heap allocations, but not fully contiguous.

**PtrTrie:** The baseline. Each node owns its children via `array<unique_ptr<Node>, 26>`. Every new node is an individual heap allocation. Nodes end up wherever the allocator puts them.

A side benefit of the index-based approach: it eliminated the `const_cast` pattern I used in the other implementations. Since `find_last_node_index` returns a `size_t`, there's no const/non-const pointer overload to deduplicate.

Raw benchmark data: [release](../reports/trie_bench_release.csv), [debug](../reports/trie_bench_debug.csv)

---

## Insert: arenas win, deque wins biggest

**Release:**

| Benchmark | 256 | 4K | 64K |
|-----------|-----|-----|------|
| Index Dense (sentinel) | 16μs | 152μs | 3.4ms |
| Deque Dense | 12μs | 151μs | 1.8ms |
| Ptr Dense | 44μs | 546μs | 11.5ms |
| Index Sparse (sentinel) | 17μs | 236μs | 3.9ms |
| Deque Sparse | 13μs | 190μs | 2.8ms |
| Ptr Sparse | 50μs | 682μs | 19.3ms |

Both arenas beat PtrTrie convincingly — IndexArena by ~3.4x, DequeArena by ~6.3x at 64K dense. The surprise is that DequeArena is the fastest inserter, nearly twice as fast as IndexArena at 64K.

Why? IndexArena's `vector<Node>` occasionally reallocates and copies the entire array when it grows. Each Node is 216 bytes[^1] so at 64K nodes that's ~13.5MB being copied on resize. DequeArena's `deque` never moves existing nodes — it just allocates a new chunk and updates its internal bookkeeping. The reallocation cost outweighs the locality benefit during construction.

This is a tradeoff the old post missed entirely, because the old "arena" wasn't paying this cost either — it was just shuffling 8-byte pointers on resize, not 216-byte nodes.

---

## Search: cache locality is real (this time)

**Release:**

| Benchmark | 256 | 4K | 64K |
|-----------|-----|-----|------|
| Index Hit (sentinel) | 910ns | 24μs | 566μs |
| Deque Hit | 1.2μs | 21μs | 923μs |
| Ptr Hit | 1.1μs | 20μs | 998μs |

This is the result the old post was looking for but couldn't find. At 64K words, IndexArena searches are 1.8x faster than PtrTrie. The old post reported identical search times — because both implementations scattered nodes on the heap, there was no locality difference to measure.

Now there is. IndexArena's nodes are contiguous in a `vector<Node>`. When searching for a word, you follow a path of ~6 nodes. In PtrTrie, those 6 nodes could be anywhere in a ~13.5MB heap region. In IndexArena, they're within a contiguous ~13.5MB block — the CPU's hardware prefetcher can anticipate sequential access, and nodes created close in time (which often means structurally close in the trie) end up physically close in memory.

DequeArena falls in between, as expected: nodes within a chunk get locality benefits, but cross-chunk traversals don't.

The effect is size-dependent. At 256 words, the entire trie fits comfortably in L2 cache (4 MiB per core on this hardware) regardless of layout, so there's no meaningful difference. At 64K, the working set exceeds cache, and physical layout starts to matter.

There's a wrinkle, though. Looking at the full range of sizes, IndexArena is actually ~20-30% *slower* than PtrTrie at 512 and 4K, roughly tied at 32K, and only pulls ahead decisively at 64K. The disassembly confirms why[^2]. The inner loops:

**IndexArena:**
```asm
mov    w11, #216                  ; multiplier (hoisted before loop)
; loop body:
ldrsb  x12, [x9]                 ; load char
madd   x13, x0, x11, x8          ; node_addr = index * 216 + base
add    x12, x13, x12, lsl #3     ; + char_offset * 8
sub    x12, x12, #768            ; adjust for 'a' offset
ldr    x0, [x12]                 ; load child index
cmn    x0, #1                    ; compare to sentinel
b.eq   exit                      ; branch if NULL_INDEX
```

**PtrTrie:**
```asm
; loop body:
ldrsb  x10, [x8]                 ; load char
add    x10, x0, x10, lsl #3      ; node_ptr + char_offset * 8
sub    x10, x10, #768            ; adjust for 'a' offset
ldr    x0, [x10]                 ; load child pointer
cbz    x0, exit                  ; compare-and-branch-if-zero
```

IndexArena pays a `madd` (multiply-add by 216) every iteration to convert an index to an address. Since 216 isn't a power of 2, the compiler can't reduce this to a shift — it's a real multiply. PtrTrie skips this entirely: the pointer *is* the address. PtrTrie also gets `cbz` — a fused compare-and-branch-if-zero — where IndexArena needs `cmn` + `b.eq` (two instructions).

At mid-range sizes where everything fits in cache, this per-iteration overhead dominates. At 64K, cache locality overwhelms it.

| Benchmark | 256 | 4K | 64K |
|-----------|-----|-----|------|
| Index Miss (sentinel) | 128ns | 1.9μs | 31μs |
| Deque Miss | 111ns | 1.7μs | 27μs |
| Ptr Miss | 76ns | 1.1μs | 17μs |

Misses tell a different story: PtrTrie is fastest. This makes sense — a miss on "zzzzzzz" in a trie of "a"-prefixed words fails at the root node, one array lookup and done. The benchmark repeats this n times on the same hot node. No traversal means no cache effects.

My initial guess was that PtrTrie's `nullptr` comparison (compare to zero) might be cheaper than IndexArena's sentinel check (`== numeric_limits<size_t>::max()`). Disassembly[^3] says otherwise — the compiler is clever about both:

```asm
; nullptr check              ; sentinel check
cmp  x8, #0                  cmn  x0, #1
cset w0, eq                  cset w0, eq
```

`cmn x0, #1` adds 1 and checks for zero — true exactly when `x0 == 0xFFFF...FFFF`. One instruction each, no constant load needed. And disassembling the actual child access (`node->children[idx]`) produces identical code for both — an `add` with a left-shift-by-3 and a `ldr`. So the per-check cost is the same. The miss benchmark difference is likely a microarchitectural artifact of the benchmark loop rather than a fundamental cost difference.

### Sentinel vs zero-as-null

The disassembly above showed that the sentinel check (`cmn` + `b.eq`) costs two instructions where a zero comparison gets the fused `cbz`. Could we eliminate that overhead by using 0 as our null index instead?

I templated `IndexArenaTrie` with a `bool UseSentinelValue` parameter. When `false`, `NULL_INDEX = 0` and the root node lives at index 1 (reserving index 0 as "null"). This means children default-initialize to 0, and the null check becomes a comparison against zero — eligible for `cbz`.

Search hits and inserts showed no meaningful difference — the `madd` multiply and cache effects dominate. But the miss benchmark told a different story:

| Size | Sentinel | Zero-null | Speedup |
|------|----------|-----------|---------|
| 256 | 128ns | 70ns | 1.82x |
| 512 | 248ns | 134ns | 1.85x |
| 4K | 1.95μs | 1.03μs | 1.90x |
| 32K | 15.5μs | 7.9μs | 1.95x |
| 64K | 31.4μs | 16.7μs | 1.88x |

A consistent ~1.9x speedup — much larger than a single instruction should account for. But it makes sense: the miss benchmark searches for "zzzzzzz" in a trie of "a"-prefixed words. The lookup fails at the root node on the very first child access. The entire hot path is: load one child index, compare to null, repeat. The null comparison *is* the bottleneck, so `cbz` vs `cmn` + `b.eq` is the difference between one and two instructions on the critical path.

In debug builds the effect vanishes (sentinel 1.80ms vs zero-null 1.83ms at 64K) — unoptimized function call overhead masks the instruction-level difference, same pattern we saw with search hits.

For any real workload where misses traverse more than one node, this difference would be negligible. But it's a nice confirmation that the disassembly predictions hold up.

---

## Prefix collection: still identical

**Release:**

| Benchmark | 256 | 4K | 64K |
|-----------|-----|-----|------|
| Index (sentinel) | 75ns | 316ns | 2.2μs |
| Deque | 74ns | 314ns | 2.1μs |
| Ptr | 73ns | 309ns | 2.1μs |

`getWordsWithPrefix("aaa")` collects terminal nodes from a small subtree. The subtree is small enough that it fits in cache regardless of the overall trie's memory layout. No difference.

---

## Debug vs release

The debug results reveal what the optimizer is and isn't responsible for.

**Insert** ratios hold steady across debug and release: both arenas are ~3-4x faster than PtrTrie in both builds. The allocation strategy dominates — the optimizer can't change where memory comes from.

**Search hits** tell a subtler story. In debug at 64K, DequeArena (4.3ms) is fastest, followed by IndexArena (5.5ms), then PtrTrie (6.7ms). In release, IndexArena (566μs) leaps to first place while DequeArena (923μs) and PtrTrie (998μs) are closer together. The optimizer doesn't create cache locality — but it does strip away enough abstraction overhead to let cache effects become the dominant factor. In debug, the unoptimized function call overhead masks the memory layout signal.

**Search misses and prefix collection** show uniform ~10x debug-to-release slowdown across all three implementations, same as in the old post. These operations access so few nodes that memory layout doesn't matter.

---

## What changed from the old post

The old post's conclusion was: "the optimizer is excellent at removing abstraction overhead but can't change your data's memory layout." That's still true — but the irony is that I didn't have any data layout difference to measure. Both implementations scattered nodes on the heap, so of course search was identical.

With a real arena, the picture is more nuanced:

- **Insert:** Arenas still win on allocation cost. But the *fastest* inserter is DequeArena, not the fully contiguous IndexArena — because avoiding vector reallocation matters more than node contiguity during construction.
- **Search:** Cache locality is measurable, but only at scale. At 64K nodes, contiguous memory gives IndexArena a 1.8x search advantage. At 256 nodes, everything fits in cache and layout doesn't matter. And the sentinel vs zero-null experiment confirmed that disassembly predictions translate to measurable effects — when a single instruction is the hot path.
- **The tradeoff is three-way now:** IndexArena for query-heavy workloads where you can pay the insert cost, DequeArena for balanced insert/query workloads, PtrTrie when you need per-node memory reclamation.

The lesson I keep learning: measure before concluding. The old post had the right instinct about cache effects but the wrong evidence. It took building an actual arena to see whether the theory held up. It did — just not in the way I originally described.

---

## Footnotes

[^1]: 26 × 8-byte children (208) + 1-byte `bool` + 7 bytes alignment padding = 216. All three Node types are the same size despite storing different child types (`size_t`, `Node*`, `unique_ptr<Node>`) — they're all 8 bytes on this platform. Verified with:
    ```cpp
    struct IndexNode {
        bool is_end_of_word = false;
        std::array<size_t, 26> children;
    };
    printf("IndexNode: %zu bytes\n", sizeof(IndexNode));  // 216
    printf("  is_end_of_word offset: %zu\n", offsetof(IndexNode, is_end_of_word));  // 0
    printf("  children offset: %zu\n", offsetof(IndexNode, children));  // 8
    ```

[^2]: Compiled with `c++ -std=c++17 -O2 -S -o search_loop.s search_loop.cpp` on Apple Silicon, then extracted the inner loops with `awk '/^__Z10index_find/,/\.cfi_endproc/' search_loop.s` and `awk '/^__Z8ptr_find/,/\.cfi_endproc/' search_loop.s`:
    ```cpp
    __attribute__((noinline))
    size_t index_find(const std::vector<IndexNode>& nodes, const std::string& word) {
        size_t current = 0;
        for (char c : word) {
            size_t child = nodes[current].children[c - 'a'];
            if (child == NULL_INDEX) { return NULL_INDEX; }
            current = child;
        }
        return current;
    }

    __attribute__((noinline))
    const PtrNode* ptr_find(const PtrNode* root, const std::string& word) {
        const PtrNode* current = root;
        for (char c : word) {
            const PtrNode* child = current->children[c - 'a'];
            if (child == nullptr) { return nullptr; }
            current = child;
        }
        return current;
    }
    ```

[^3]: Compiled with `c++ -std=c++17 -O2 -S -o sentinel_check.s sentinel_check.cpp` on Apple Silicon, then extracted with `grep -A 8 'check_nullptr\|check_sentinel' sentinel_check.s`:
    ```cpp
    static constexpr size_t NULL_INDEX = std::numeric_limits<size_t>::max();

    __attribute__((noinline))
    bool check_nullptr(const std::unique_ptr<int>& ptr) {
        return ptr == nullptr;
    }

    __attribute__((noinline))
    bool check_sentinel(size_t index) {
        return index == NULL_INDEX;
    }
    ```
