# Data Structures & Algorithms

A learning repo organized around interview prep and performance engineering.
Topics are chosen for depth over breadth — the goal is to understand *why*,
not just implement.

---

## [Interview Staples](interview_staples/)

These come up constantly and require reps to stay sharp, even with strong fundamentals.

- **Dynamic programming** — patterns: top-down vs bottom-up, memoization, common subproblems
- **Graph algorithms** — BFS, DFS, Dijkstra, topological sort
- **Binary search variants** — not just textbook; boundary conditions, search on answer space
- **Two pointers / sliding window** — array and string problems

---

## [Performance-Oriented Structures](performance_oriented/)

Where systems instincts give an edge. The goal is to understand *why* one structure
beats another in practice, not just in asymptotic complexity.

- **Hash map internals** — open addressing, Robin Hood hashing, load factors, why `unordered_map` is slow
- **Cache-friendly data structures** — SoA vs AoS, B-trees vs BSTs, binary heap vs pointer-based heap, false sharing
- **Sorting** — radix sort, cache behavior of quicksort vs mergesort

---

## [Compiler / Systems-Relevant](compiler_systems/)

Topics that map directly to the roles being targeted.

- **Graphs as compiler IR** — dominance trees, topological sort on CFGs
- **Tries** — symbol tables, lexers, routing tables
- **Union-Find** — alias analysis, type inference, connected components

---

## [Concurrency](concurrency/)

The hardest bugs to find and the most expensive to ship.

- **Atomics & memory ordering** — relaxed, acquire/release, SeqCst; CAS as the lock-free foundation
- **Lock-free data structures** — Treiber stack, Michael-Scott queue, ABA problem
- **Synchronization primitives** — mutex vs spinlock, reader-writer locks, condition variables
- **Concurrent hash maps** — sharded locking, CAS-based designs

---

## Language

Implementations in C++ and/or Rust — both allow reasoning about memory layout,
which is the interesting angle here.
