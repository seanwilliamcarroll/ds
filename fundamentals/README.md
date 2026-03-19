# Fundamentals

Core topics that come up constantly in practice. The goal isn't to memorize
solutions — it's to recognize problem shapes and apply the right pattern.

---

## Dynamic Programming

The one area where "knowing the theory" doesn't substitute for reps. Most people
can explain memoization but freeze on a novel problem.

**Core ideas:**
- Optimal substructure: the optimal solution to the problem contains optimal solutions to subproblems
- Overlapping subproblems: naive recursion recomputes the same subproblems — DP avoids this

**Two equivalent formulations:**
- **Top-down (memoization):** recursive, cache results as you go — easier to derive, slightly more overhead
- **Bottom-up (tabulation):** iterative, fill a table in dependency order — no recursion overhead, cache-friendlier

**Pattern recognition — common shapes:**
- 1D: `dp[i]` depends on a fixed lookback (Fibonacci, climbing stairs, house robber)
- 2D grid: `dp[i][j]` depends on neighbors (unique paths, min path sum)
- Two sequences: `dp[i][j]` over prefixes of two strings (LCS, edit distance)
- Interval: `dp[i][j]` over subarrays (matrix chain, burst balloons)
- Knapsack: pick items subject to a capacity constraint (0/1 knapsack, coin change)
- Bitmask DP: subset enumeration over small state spaces (TSP, assignment problems)

**Practice plan — pattern-based progression:**

For each problem: identify the state, write the recurrence, find base cases,
implement bottom-up, then optimize space if possible.

| # | Pattern | Problem | Why | Status |
|---|---------|---------|-----|--------|
| 1 | 1D | Coin Change | Clean unbounded knapsack recurrence, not just "look back 1-2 steps" | done |
| 2 | 1D | House Robber | "Take or skip" decision pattern | done |
| 3 | 2D grid | Min Path Sum | Introduces 2D state naturally | done |
| 4 | Two sequences | Longest Common Subsequence | Canonical "two string" DP | done |
| 5 | Two sequences | Edit Distance | Adds operations to LCS pattern | done |
| 6 | Knapsack | 0/1 Knapsack | Template for a huge family of problems | done |
| 7 | Interval | Longest Palindromic Substring | Gentler intro to interval DP | done |

---

## Graph Algorithms

Come up constantly, often disguised as non-graph problems (e.g., word ladders,
course scheduling, island counting).

**Representations:**
- Adjacency list — O(V+E) space, standard choice for sparse graphs
- Adjacency matrix — O(V²) space, fast edge lookup, good for dense graphs or Floyd-Warshall

**Traversal:**
- **BFS** — explores level by level; shortest path in unweighted graphs; uses a queue
- **DFS** — explores depth-first; cycle detection, topological sort, connected components; uses a stack (or recursion)

**Key algorithms:**
- **Topological sort** — ordering of nodes in a DAG such that all edges point forward; two approaches: DFS post-order reversal, or Kahn's algorithm (iterative, BFS-based). Critical for dependency resolution, compiler scheduling.
- **Min-heap** — complete binary tree in a flat array; `bubble_up`/`bubble_down` give O(log n) push/pop. Built from scratch as a prerequisite for Dijkstra.
- **Dijkstra's** — single-source shortest path in weighted graphs with non-negative edges; O((V+E) log V) with a min-heap. Lazy deletion (stale entry skip) instead of decrease-key.
- **Cycle detection** — DFS with a "in-stack" visited set (directed); union-find or DFS coloring (undirected)
- **Connected components / SCC** — union-find for undirected; Tarjan's or Kosaraju's for SCCs in directed graphs

**Practice plan — build up graph intuition and representations:**

| # | Concept | Problem | Why | Status |
|---|---------|---------|-----|--------|
| 1 | BFS/DFS on implicit graph | Number of Islands | Grid as graph — no class needed, focus on traversal | done |
| 2 | Adjacency list, deep copy | Clone Graph | Forces you to build and traverse a neighbor-list representation | done |
| 3 | Topological sort, cycle detection | Course Schedule I & II | Directed graph, in-degree tracking, Kahn's algorithm | done |
| 4 | Weighted shortest path | Network Delay Time (SPFA + Dijkstra) | Min-heap from scratch, SPFA vs Dijkstra benchmarking across graph density | done |

---

## Binary Search Variants

The textbook version is trivial. The real skill is applying binary search to
non-obvious problem shapes.

**The template:** maintain an invariant — `lo` is always a valid candidate or
below it, `hi` is always above. Close the gap until `lo == hi`.

**Beyond sorted arrays — binary search on the answer space:**
When the answer is monotonic (if X works, X-1 also works), binary search on X directly.
Examples: "find the minimum capacity such that you can ship in D days" — you don't
search an array, you search the range of possible answers.

**Variants:**
- Find first/last occurrence of a value (leftmost/rightmost insertion point)
- Search in rotated sorted array
- Find minimum in rotated sorted array
- Binary search on a 2D matrix
- Answer-space search (capacity, kth element, etc.)

**Common bugs:**
- Off-by-one on `lo`/`hi` initialization
- Infinite loop when `mid` rounds down and `lo = mid` (use `mid = lo + (hi - lo + 1) / 2` when needed)
- Wrong convergence condition (`<` vs `<=`)

**What to practice:**
- Search in rotated sorted array
- Find first and last position of element
- Koko eating bananas (answer-space)
- Median of two sorted arrays (hard — worth understanding the approach)

---

## Two Pointers / Sliding Window

A family of techniques for linear-time array and string problems that would
otherwise be O(n²).

**Two pointers — opposite ends:**
Works when the array is sorted and you're looking for a pair satisfying some
condition. Move the left pointer right to increase the sum, move the right
pointer left to decrease it.

- Two sum (sorted input), three sum, container with most water

**Two pointers — same direction (fast/slow):**
One pointer advances faster than the other. Classic use: cycle detection in a
linked list (Floyd's algorithm). Also: find the middle of a list, remove nth
from end.

**Sliding window:**
Maintain a window `[lo, hi]` over a sequence. Expand `hi` to include new
elements, shrink `lo` to restore a constraint. Track the window state
incrementally to avoid recomputation.

- **Fixed size:** max sum subarray of size k
- **Variable size:** longest substring without repeating characters, minimum
  window substring, longest substring with at most k distinct characters

**Key insight:** sliding window works when the constraint is monotonic — adding
an element can only make things worse (or only better), never both. This lets
you move `lo` forward without missing valid windows.

**What to practice:**
- Longest substring without repeating characters
- Minimum window substring
- Three sum
- Trapping rain water (two pointer variant)
- Linked list cycle detection (fast/slow)
