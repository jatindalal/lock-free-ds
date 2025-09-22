# lfds

1. Foundations to Brush Up On

Memory Models: Learn what sequential consistency, acquire-release, and relaxed mean. You’ll need to know why memory_order_relaxed is unsafe for correctness, and where memory_order_acq_rel fits.

Atomics: Revisit std::atomic<T> in C++ (or equivalent in Rust/Java). Get very comfortable with compare_exchange_strong vs compare_exchange_weak.

The ABA Problem: Understand why popping from a stack can reuse the same pointer, tricking CAS into success incorrectly.

2. Patterns You’ll See in Lock-Free Structures

Single-word CAS Structures: Many lock-free algorithms rely on atomically swapping a pointer or integer.

Linked Nodes: Queues and stacks are typically implemented with linked nodes whose next pointers are CAS-ed.

Hazard Pointers / Epoch GC: For safe memory reclamation (so you don’t free a node still being read by another thread).

3. Start Small

Lock-free stack:

Core idea: head pointer is atomic.

Push = allocate node, CAS head from old → new.

Pop = CAS head to head->next.

Problem: ABA unless you handle reclamation.

Lock-free queue (Michael & Scott algorithm):

Uses two atomic pointers (head, tail).

Enqueue = swing tail forward with CAS, then link node.

Dequeue = advance head.

Trickiness comes from consistency between head/tail when multiple threads race.

Hashmap:

Much more advanced (usually built on top of lock-free buckets/linked lists).

Don’t start here until you’ve nailed stack/queue.

4. Correctness Tools

Linearizability: Each operation should appear atomic at some instant between invocation and return.

Stress Testing: Write randomized multi-threaded tests that push/pop/enqueue/dequeue in many threads, then check invariants.

Formal Tools:

TSAN (Thread Sanitizer) to catch data races.

Model checking tools like CDSChecker (if you want to go deep).

5. Benchmarking

Workload: Multiple threads doing N operations each.

Compare:

Naive mutex-protected version.

Lock-free version.

Metrics: throughput (ops/sec), scalability (#threads).

6. Incremental Approach

Implement with coarse-grained lock.

Replace with atomic + CAS.

Handle ABA/reclamation.

Add benchmarks.

Validate correctness with tests.

7. Extra Crumbs (if you want to push further)

Look at Michael & Scott Queue paper (classic lock-free queue).

Learn about RCU (Read-Copy-Update) and wait-free progress guarantees.

Explore hazard pointers or epoch-based reclamation for memory safety.

👉 My suggestion: start with a lock-free stack (the “hello world” of lock-free data structures), benchmark it against a std::stack protected with std::mutex, and then move to the Michael–Scott queue.

Do you want me to lay out a step-by-step micro-roadmap just for the stack (like milestone 1: implement atomic head, milestone 2: test linearizability, milestone 3: add hazard pointers…), or do you prefer this higher-level breadcrumb trail?
