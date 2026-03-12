# Data Structures & Algorithms

A learning repo organized around interview prep and performance engineering.
Topics are chosen for depth over breadth — the goal is to understand *why*,
not just implement.

---

## Interview Staples

These come up constantly and require reps to stay sharp, even with strong fundamentals.

- **Dynamic programming** — patterns: top-down vs bottom-up, memoization, common subproblems
- **Graph algorithms** — BFS, DFS, Dijkstra, topological sort
- **Binary search variants** — not just textbook; boundary conditions, search on answer space
- **Two pointers / sliding window** — array and string problems

---

## Performance-Oriented Structures

Where systems instincts give an edge. The goal is to understand *why* one structure
beats another in practice, not just in asymptotic complexity.

- **Hash map internals** — open addressing, Robin Hood hashing, load factors, why `unordered_map` is slow
- **Cache-friendly data structures** — SoA vs AoS, B-trees vs BSTs, heap vs linked list in practice
- **Sorting** — radix sort, cache behavior of quicksort vs mergesort

---

## Compiler / Systems-Relevant

Topics that map directly to the roles being targeted.

- **Graphs as compiler IR** — dominance trees, topological sort on CFGs
- **Tries** — string matching, symbol tables
- **Union-Find** — alias analysis, type inference, connected components

---

## Language

Implementations in C++ and/or Rust — both allow reasoning about memory layout,
which is the interesting angle here.
