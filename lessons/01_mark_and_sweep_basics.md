# Lesson 01: Foundational Tracing Garbage Collection (Mark-and-Sweep)

**Branch:** `original-bootdev-course` (Merged in PR #1)
**Focus:** Core mechanics of stop-the-world tracing collectors, tri-color reachability invariants, call stack safety via explicit worklists, and precise root management.

______________________________________________________________________

## 1. System Engineering & Core Concepts

### 1.1 The Graph Reachability Abstraction

Memory management in managed runtimes models the heap as a **directed graph** $G = (V, E)$:

- **Vertices ($V$):** Heap-allocated objects (`object_t`).
- **Edges ($E$):** Reference pointers held inside containers (`ARRAY` elements, `VECTOR3` fields).
- **Roots ($R \\subseteq V$):** Pointers directly accessible to the runtime execution engine without traversing the heap (active local variables held within call frames).

An object $v$ is defined as **live** if there exists a directed path from any root $r \\in R$ to $v$. All vertices unreachable from $R$ are dead and eligible for reclamation.

```text
Roots (Call Frames)
    |
    v
 [Root Obj A] --------> [Child Obj B] --------> [Child Obj C]
                                                    |
 [Dead Obj D] <-------> [Dead Obj E] (Cycle)        v
 (Unreachable from Roots: Collected!)            [Child Obj F]
```

### 1.2 Exact (Precise) Roots vs. Conservative Garbage Collection

Runtimes discover roots using one of two primary architectural strategies:

1. **Conservative Collection (e.g., Boehm GC):**
   - The runtime has no precise type information for stack frames.
   - It treats the CPU registers and raw C call stack as an arbitrary array of memory words, heuristically assuming that any value matching an allocated heap address *might* be a pointer.
   - **Trade-off:** Easy to drop into unmanaged C/C++, but prone to memory retention (integers that resemble pointers prevent deallocation) and cannot safely compact or move memory.
1. **Exact / Precise Collection (Our Architecture):**
   - The VM explicitly tracks all live references in managed call frames (`frame_t`).
   - Every pointer in `frame->references` is guaranteed to be a valid object pointer.
   - **Advantage:** Zero false retentions, strictly predictable lifetimes, and compatibility with compacting/relocating memory managers.

### 1.3 The Tri-Color Marking Abstraction & The Invariant

Tracing algorithms categorize all heap objects into three conceptual colors:

- **White:** Unvisited objects. At the start of GC, all objects are white (candidates for reclamation).
- **Gray:** Reachable objects that have been visited, but whose outgoing reference edges have not yet been inspected. Gray objects form the **frontier** of the reachability search.
- **Black:** Reachable objects whose outgoing reference edges have all been fully inspected. Black objects cannot be freed.

> [!IMPORTANT]
> **The Strong Tri-Color Invariant**:
> In any valid tracing garbage collector, **no Black object may hold a direct pointer to a White object without an intervening Gray object.**
> If this invariant is ever broken, the reachability frontier will miss the white object, causing the sweep phase to deallocate live memory.

In a **Stop-the-World (STW)** collector, mutator (application) execution is completely paused during GC. This guarantees that user code cannot modify pointers behind the collector's back, preserving the Tri-Color Invariant throughout the trace phase without requiring expensive read or write barriers.

______________________________________________________________________

## 2. Pitfalls, Failure Modes & Diagnosis

### 2.1 The Recursive Call Stack Overflow Trap

A naive implementation of graph tracing uses direct recursion:

```c
// DANGEROUS: Recursive traversal blows the OS call stack
void trace_mark_object_recursive(object_t *obj) {
  if (obj == NULL || obj->is_marked) return;
  obj->is_marked = true;
  if (obj->kind == ARRAY) {
    for (size_t i = 0; i < obj->data.v_array.size; i++) {
      trace_mark_object_recursive(obj->data.v_array.elements[i]);
    }
  }
}
```

- **The Failure Mode:** When traversing deep linear graphs (e.g., a linked list or deeply nested trees of $100,000$ elements), every reference pushes a native C stack frame. The thread exhausts its allocated OS stack space (typically 1MB–8MB), resulting in an immediate, uncatchable `SIGSEGV` stack overflow.
- **The Solution:** Replace C call stack recursion with an **explicit heap-allocated worklist** (a dedicated `gray_objects` stack). Graph traversal becomes strictly iterative and consumes $O(1)$ native stack space.

### 2.2 Dangling Roots Across Frame Pops

- **The Failure Mode:** If a call frame is popped from the execution stack (`vm_frame_pop()`), but its pointers remain registered in the VM root set or are not dereferenced, the GC treats dead local variables as live roots, causing insidious memory leaks.
- **The Requirement:** Call frames must manage their reference lists strictly in lockstep with VM scope entry and exit.

### 2.3 Premature Sweep (Frontier Incompletion)

- **The Failure Mode:** If the sweep phase begins while the gray worklist is non-empty, reachable objects downstream of the remaining gray frontier will still be marked White.
- **The Result:** The sweep phase frees reachable objects, corrupting active program memory into immediate `heap-use-after-free` crashes.

______________________________________________________________________

## 3. Architectural Solutions & Mental Models

### 3.1 The Three-Phase Mark-Trace-Sweep Pipeline

```text
[All Objects: White]
         |
         | 1. mark() (Inspect call frames)
         v
[Root Objects: Gray] <----+ (Push to gray stack)
         |                |
         | 2. trace()     | Repeat until
         v                | gray stack empty
[Inspect Children] -------+
         |
         | (All reachable objects now Black)
         v
   3. sweep()
  /          \
Reachable?    Unreachable?
  |               |
Keep Black     free() memory
Reset to White  Set slot to NULL
```

1. **Mark Phase (`mark()`):**
   - Scans all active call frames in `CURRENT_VM->frames`.
   - Every directly referenced object is marked (`obj->is_marked = true`) and pushed onto the gray frontier.
1. **Trace Phase (`trace()`):**
   - Iteratively pops objects from `gray_objects` (turning them from Gray to Black).
   - Traverses outgoing reference edges (`VECTOR3` components, `ARRAY` elements).
   - If a referenced child is unvisited (`!child->is_marked`), it is marked and pushed onto the gray stack.
   - Terminates when the gray stack is empty. All reachable objects are now Black; all unreachable objects remain White.
1. **Sweep Phase (`sweep()`):**
   - Linearly scans the global object pool (`CURRENT_VM->objects`).
   - If `obj->is_marked == true`: the object survived. Reset `obj->is_marked = false` (preparing for the next GC cycle).
   - If `obj->is_marked == false`: the object is dead. Free its private payload, free the object header, and clear the registry slot.
   - Compaction: Call `stack_remove_nulls()` to compact the tracking list.

### 3.2 Global Object Registry vs. Fragmented Free Lists

In pure Mark-and-Sweep, memory management ownership is completely centralized:

- Containers (`ARRAY`, `VECTOR3`) **never manage the lifecycles of their children**.
- The VM's global allocation list (`CURRENT_VM->objects`) acts as the single source of truth for all heap memory.
- Sweeping frees unreachable objects individually via the system allocator (`free()`). While tracking registry slots are compacted via `stack_remove_nulls()`, the underlying heap space is subject to memory fragmentation over time.

______________________________________________________________________

## 4. Performance & Systems Optimization

### 4.1 Throughput vs. Pause Latency Trade-offs

- **Allocation Throughput:**
  - Mark-and-Sweep provides extremely high allocation throughput during mutator execution.
  - Object creation requires only a single `calloc()` and an append to `CURRENT_VM->objects`.
  - There is **zero reference counting overhead** on variable assignment, pointer swaps, or parameter passing.
- **Stop-the-World Pauses:**
  - The collector pauses execution to trace the heap.
  - Pause duration is proportional to **$O(\\text{Live} + \\text{Dead})$** objects in the heap. As heap size grows, GC pauses become perceptible.

### 4.2 Cache Locality During Graph Traversal

- In an iterative gray worklist, popping from the top of the stack results in **Depth-First Search (DFS)** traversal, keeping recently visited parent/child clusters hot in the CPU L1/L2 data cache.
- Queuing objects into a FIFO worklist would result in **Breadth-First Search (BFS)** traversal, causing wide pointer hops across disjoint heap pages that increase CPU cache misses.

______________________________________________________________________

## 5. Tooling Insights & Workflow Takeaways

### 5.1 Allocator Interception via `bootlib`

- **Mechanism:** `bootlib` intercepts `malloc`, `calloc`, `realloc`, and `free` by prefixing every heap block with a hidden allocation header containing block size, magic markers, and unique allocation IDs.
- **Leak Detection:** At the end of every test execution, `assert(boot_all_freed())` inspects the active allocation registry. If even a single 8-byte pointer or string buffer is orphaned, the test fails with an exact byte count and allocation trace.

### 5.2 Dual Verification: ASan + Allocation Tracking

- AddressSanitizer (ASan) excels at detecting **spatial errors** (buffer overflows, out-of-bounds array access) and **temporal errors** (use-after-free, double frees).
- `bootlib` excels at detecting **silent resource leaks** (reachable or orphaned blocks that were never freed).
- Combining both tools guarantees that every test in the suite verifies memory correctness and resource reclamation simultaneously.
