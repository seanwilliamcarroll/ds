# Compiler / Systems-Relevant Data Structures

These structures appear directly in compiler infrastructure, OS internals, and
systems tooling. Understanding them is both interview signal and practical
knowledge for the roles being targeted.

---

## Graphs as Compiler IR

A compiler doesn't work on text after parsing — it works on graphs. Most
classical compiler optimizations are graph algorithms in disguise.

**Control Flow Graph (CFG):**
Each node is a basic block (a maximal sequence of instructions with no branches
inside). Edges represent possible control flow: an unconditional jump, both
branches of a conditional, fall-through. The CFG is the foundation for nearly
all compiler analyses.

**Dominance:**
Node A *dominates* node B if every path from the entry to B passes through A.
The *immediate dominator* of B is the closest dominator — this gives you the
*dominator tree*, a tree structure over the CFG where each node's parent is its
idom.

Why it matters: dominance drives SSA construction, loop identification, and
code motion. Computing dominators is a classic graph algorithm (Cooper et al.'s
simple iterative algorithm, or Lengauer-Tarjan for large CFGs).

**SSA form (Static Single Assignment):**
Each variable is defined exactly once. At join points where control flow merges,
a φ (phi) function selects the value from the appropriate predecessor.

SSA makes dataflow analysis trivial — use-def chains are explicit and direct.
Most modern compilers (LLVM, GCC, JVM JIT) work primarily in SSA form.
Construction requires the dominator tree (specifically, dominance frontiers).

**Topological sort on CFGs:**
Many analyses need to process blocks in a specific order. For acyclic graphs,
topological order ensures you process a block only after all its predecessors.
For graphs with back edges (loops), reverse post-order (RPO) is the standard
— it approximates topological order while handling cycles gracefully.

**What to understand:**
- How to compute dominators iteratively (dataflow equation: `dom(n) = {n} ∪ ∩ dom(pred)`)
- Why SSA phi functions appear exactly at dominance frontiers
- How loop detection uses back edges (a back edge goes from a node to a dominator)

---

## Tries

A trie (prefix tree) stores strings by sharing common prefixes. Each node
represents a prefix; the path from root to a node spells out that prefix.

**Structure:**
- Each node has up to |alphabet| children (26 for lowercase English, 256 for bytes)
- A node is marked as terminal if it's the end of an inserted string
- Insert, search, and prefix-query are all O(m) where m is the string length —
  independent of the number of strings stored

**Where tries appear in compilers and systems:**
- Symbol tables and identifier lookup — trie gives O(m) lookup vs O(m log n)
  for a sorted structure
- Lexers — keyword recognition can be compiled into a trie/DFA
- IP routing tables — longest prefix match is a trie query
- Autocomplete, spell checking

**Compressed trie (Patricia trie / radix tree):**
Collapse chains of single-child nodes into a single edge labeled with a string
segment. Reduces space from O(total characters) to O(number of keys). Used in
the Linux kernel's radix tree and in routing tables.

**Ternary search trie:**
Each node has three children: less-than, equal, greater-than. More
cache-friendly than a 26-way trie for sparse alphabets (no large sparse arrays
of child pointers). Competitive with hash maps for string keys.

**What to practice:**
- Implement a basic trie (insert, search, startsWith)
- Word search II (trie + DFS — classic hard problem)
- Longest common prefix using a trie

---

## Union-Find (Disjoint Set Union)

Maintains a partition of a set into disjoint subsets. Supports two operations:
- **Find(x):** which subset does x belong to? (returns a representative)
- **Union(x, y):** merge the subsets containing x and y

**Naive implementation:** each element points to its parent; root is the
representative. Find follows parent pointers to the root. Union links one root
to the other.

**Two critical optimizations:**

*Union by rank (or size):* always attach the smaller tree under the larger.
Keeps tree height O(log n).

*Path compression:* after Find(x), make every node on the path point directly
to the root. Future finds on those nodes are O(1).

Together: nearly O(1) amortized per operation — formally O(α(n)) where α is the
inverse Ackermann function, which is ≤ 4 for any n you'll ever encounter.

**Where Union-Find appears:**
- **Compiler:** alias analysis (are these two pointers in the same alias set?),
  type unification in type inference (Hindley-Milner), equivalence class merging
  during optimizations
- **Systems:** network connectivity, Kruskal's MST algorithm
- **Interviews:** number of connected components, redundant connection, accounts
  merge, number of islands (alternative to DFS)

**What to practice:**
- Number of connected components in an undirected graph
- Redundant connection (detect cycle via union-find)
- Accounts merge (union by shared email, then group)
- Minimum spanning tree via Kruskal's (union-find is the core data structure)
