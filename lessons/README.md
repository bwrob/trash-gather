# Learning Log & Engineering Lessons (`lessons/`)

This directory documents the core engineering concepts, architectural trade-offs, debugging insights, and mental models discovered during work on each milestone branch (Merge Request / PR).

As a solo learning project, the primary deliverable is **deep understanding**. Each lesson records what went right, what went wrong, and why certain architectural decisions were made.

______________________________________________________________________

## Index of Lessons

| #      | Topic / Branch                                                                                 | Key Concepts Explored                                                                                                                                       |
| :----- | :--------------------------------------------------------------------------------------------- | :---------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **01** | [Foundational Mark-and-Sweep VM](01_mark_and_sweep_basics.md)                                  | Tri-color marking, tracing object graphs, stack frames as roots, memory leak tracking with `bootlib`.                                                       |
| **02** | [Hybrid RC + Mark-and-Sweep & De-sneking](02_hybrid_gc_and_desneking.md)                       | POSIX header collisions, reference counting vs cycle sweeping, the Single-Pass Deallocation Trap (ASan use-after-free), two-phase reclamation.              |
| **03** | [Variable-Length Tuples & Allocation Rollback](03_arbitrary_tuples_and_allocation_rollback.md) | Flexible array layouts, two-phase allocation rollback (ASan UAF), container reference count parity, realloc pointer traps, and adversarial test heuristics. |
