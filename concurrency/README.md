# Concurrency

Concurrency bugs are the hardest to reproduce and the most expensive to ship.
Systems and compiler roles expect you to reason about memory ordering, lock-free
algorithms, and synchronization tradeoffs — not just "use a mutex."

---

## Atomics & Memory Ordering

The foundation of all concurrent programming. Without understanding memory
ordering, lock-free code is just a race condition you haven't found yet.

**Why memory ordering exists:**
CPUs and compilers reorder instructions for performance. On x86, stores can be
reordered past other stores; on ARM/RISC-V, almost anything can be reordered.
Memory orderings are the programmer's contract with the hardware about what
reorderings are allowed.

**The orderings (weakest to strongest):**
- **Relaxed** (`memory_order_relaxed` / `Ordering::Relaxed`) — no ordering
  guarantees beyond atomicity. Good for counters where you just need the final
  value. Compiles to a plain load/store on x86.
- **Acquire/Release** — a release store synchronizes-with an acquire load.
  Everything written before the release is visible after the acquire. This is
  the building block for mutexes and message passing.
- **SeqCst** (`memory_order_seq_cst` / `Ordering::SeqCst`) — total order across
  all SeqCst operations. Easiest to reason about, but may add fence instructions.
  Default in both C++ and Rust for good reason — start here, weaken only when
  profiling shows you need to.

**Compare-and-swap (CAS):**
The fundamental lock-free primitive: atomically check that a value is what you
expect, and if so, replace it. All lock-free data structures are built on CAS
loops. In C++: `atomic::compare_exchange_weak/strong`. In Rust:
`AtomicPtr::compare_exchange`.

`compare_exchange_weak` can spuriously fail (returns false even if the value
matched) — cheaper on LL/SC architectures (ARM), use it in retry loops.

**What to understand:**
- Draw the "happens-before" relationship for an acquire/release pair
- Why a relaxed increment is safe for a counter but not for a flag
- The difference between `compare_exchange_weak` and `strong`

---

## Lock-Free Data Structures

Lock-free means "at least one thread makes progress in a finite number of
steps" — no thread can block all others by holding a lock. This matters in
latency-sensitive systems (trading, game engines, kernel schedulers).

**Treiber stack (lock-free stack):**
A singly linked list where push/pop use CAS on the head pointer. Push: create
node, set its next to current head, CAS head from old to new. Pop: read head,
CAS head from old to old->next.

**Michael-Scott queue (lock-free queue):**
Separate head and tail pointers. Enqueue CASes the tail; dequeue CASes the
head. Sentinel node simplifies the empty case. This is the textbook lock-free
queue — `java.util.concurrent.ConcurrentLinkedQueue` is based on it.

**The ABA problem:**
Thread 1 reads A, gets preempted. Thread 2 pops A, pushes B, pushes A back.
Thread 1's CAS succeeds (value is still A) but the structure has changed
underneath.

**Solutions to ABA:**
- **Tagged pointers:** pack a version counter into unused pointer bits (or use
  a double-width CAS). Each CAS increments the tag.
- **Hazard pointers:** each thread publishes which nodes it's currently
  accessing; reclamation skips those nodes.
- **Epoch-based reclamation (EBR):** threads enter/exit epochs; memory is freed
  only when no thread is in the epoch that freed it. Used by `crossbeam-epoch`
  in Rust.

**What to understand:**
- Why naive CAS on a pointer is insufficient (ABA)
- Tradeoffs: hazard pointers (bounded memory) vs EBR (better throughput, unbounded in pathological cases)
- Why Rust's ownership model makes lock-free code safer but not easy

---

## Synchronization Primitives

When lock-free is overkill (it usually is), pick the right lock.

**Mutex vs spinlock:**
- **Mutex** (`std::mutex`, `std::sync::Mutex`) — puts the thread to sleep on
  contention. Good when critical sections are long or contention is high.
  Context switch cost: ~1-10μs.
- **Spinlock** — busy-waits in a loop. Good when critical sections are very
  short (< 1μs) and contention is low. Wastes CPU if the holder gets preempted.
  Never use in userspace unless you've measured.

**Reader-writer locks:**
`std::shared_mutex` / `std::sync::RwLock`. Multiple readers OR one writer.
Sounds ideal for read-heavy workloads, but writer starvation and cache-line
bouncing (readers still atomically update the reader count) make it slower than
a plain mutex in many cases. Measure first.

**Condition variables:**
Wait for a condition to become true without spinning. Always used with a mutex
and a predicate check in a loop (spurious wakeups are allowed by the spec).
C++: `std::condition_variable`. Rust: `std::sync::Condvar`.

**Rust's key difference:**
Rust's `Mutex<T>` *owns* the data it protects — you can't access the data
without holding the lock. This eliminates the most common mutex bug (accessing
shared data without locking) at compile time. C++ mutexes are advisory — the
data and the lock are separate, and the programmer must remember to lock.

**False sharing in threading context:**
Two threads writing to adjacent fields that share a cache line cause constant
invalidation traffic between cores. Fix: `alignas(64)` in C++ or
`#[repr(align(64))]` in Rust to pad each field to its own cache line. (Also
covered in performance_oriented/ from the data structure angle.)

**What to practice:**
- Implement a thread-safe bounded queue using mutex + condition variable
- Implement a simple spinlock using `atomic_flag` / `AtomicBool`

---

## Concurrent Hash Maps

Connects directly to the hash map internals covered in performance_oriented/ —
now add thread safety without destroying performance.

**Sharded/striped locking:**
Divide the table into N shards, each with its own lock. A lookup locks only
one shard. Throughput scales roughly linearly with shard count (up to the point
where lock overhead dominates). Java's `ConcurrentHashMap` (pre-Java 8) used
this approach with 16 segments.

**Modern ConcurrentHashMap (Java 8+):**
Uses CAS on individual bins + a per-bin lock for structural changes (tree
conversion, resize). Read-heavy workloads are essentially lock-free.

**Lock-free approaches:**
Possible but complex — split-ordered lists (used in some research
implementations) provide a lock-free hash map by ordering elements so that
bucket splits don't move them.

**In Rust — `dashmap` and `crossbeam`:**
`DashMap` uses sharded `RwLock`s internally. For finer control, `crossbeam`
provides epoch-based memory reclamation that underpins many concurrent
structures in the ecosystem.

**What to understand:**
- Why a single global lock on a hash map is catastrophic at scale
- The shard count tradeoff: more shards = better concurrency but more memory
  and overhead
- How this builds on the open addressing / Robin Hood knowledge from
  performance_oriented/
