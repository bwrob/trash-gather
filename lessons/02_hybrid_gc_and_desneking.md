# Lesson 02: Hybrid Memory Management (Reference Counting + Tracing Cycle Collector)

**Branch:** `hybrid-gc` (Merged in PR #2)
**Focus:** Architectural foundations of hybrid memory management, reference graph invariants, solving the Single-Pass Deallocation Trap, and platform-level C systems pitfalls.

______________________________________________________________________

## 1. System Engineering: C Namespaces & OS Header Collisions

### The POSIX Darwin `stack_t` Collision

- **The Pitfall:** Renaming types to concise generic names (e.g., `stack_t`, `list_t`, `node_t`) frequently breaks on Unix-like operating systems.
- **The Mechanism:** On POSIX / macOS Darwin systems, `<signal.h>` (often pulled in transitively by `<stdlib.h>` or test frameworks) defines:
  ```c
  typedef struct sigaltstack stack_t;
  ```
- **Transferable Principle:** C has a single global namespace for type identifiers. Library and runtime data structures must **always** carry a subsystem prefix (e.g., `vm_stack_t`, `gc_node_t`, `py_tuple_t`). Never assume short, common words are available in C headers.

______________________________________________________________________

## 2. Hybrid Memory Architecture: Throughput vs. Completeness

Automatic memory management generally falls into two paradigms, each with fundamental trade-offs:

| Characteristic         | Immediate Reference Counting                             | Tracing Garbage Collection (Mark-and-Sweep)            |
| :--------------------- | :------------------------------------------------------- | :----------------------------------------------------- |
| **Reclamation Timing** | Deterministic (exact instant refcount hits 0)            | Deferred (during periodic GC pause phases)             |
| **Pause Times**        | Smooth, distributed across execution                     | Stop-the-world pauses proportional to heap graph       |
| **Locality**           | Excellent cache locality for short-lived data            | Can cause page-thrashing during mark/sweep walks       |
| **Fatal Flaw**         | Cannot collect reference cycles ($A \\leftrightarrow B$) | Traversal overhead even for simple temporary variables |

### The Hybrid Synthesis (The CPython Model)

1. **Immediate Reference Counting** acts as the primary allocator:
   - Over 90% of objects in typical runtimes are short-lived, acyclic values (strings, integers, intermediate expressions). Immediate refcounting destroys them instantly with zero tracing overhead and prompt destructor execution.
1. **Periodic Tracing Cycle Collector** acts as the safety net:
   - Runs occasionally to detect unreachable islands of circular references that refcounting cannot collect alone.
1. **The Primitive Filtering Optimization**:
   - Primitives (integers, floats, strings) have no outgoing reference pointers and **can never participate in cycles**.
   - A high-performance cyclic collector should only register and track **container types** (arrays, tuples, dicts, instances). Untracking primitives keeps GC pause times minimal by dramatically shrinking the active graph.

______________________________________________________________________

## 3. The Single-Pass Deallocation Trap

### The Conflation of Resource Deallocation and Graph Management

In a pure tracing collector, an object's `free()` function only needs to return the object's own heap memory (payload buffers and header).

In a reference-counted runtime, deallocation has two conflicting jobs:

1. **Resource Reclamation:** Freeing private heap allocations (e.g., string buffers, dynamic arrays).
1. **Graph Ownership Relinquishment:** Cascading decrements to children (`refcount_dec(child)`).

### The Inherent Failure of Linear Traversal

When a runtime iterates over a collection of objects (during full VM shutdown or cycle sweeping):

- In any realistic program, object creation order is arbitrary. A child may be allocated *before* its container (`v = new_vector(x, y)`), or *after* its container (`arr = new_array(2); arr[0] = new_string(...)`).
- **Backward iteration:** Frees children created before their parents first. When the loop visits the parent, the parent cascades a decref to an already-freed child pointer $\\rightarrow$ **ASan `heap-use-after-free`**.
- **Forward iteration:** Frees children created after their parents first. When the loop visits the parent, the parent cascades a decref to an already-freed child pointer $\\rightarrow$ **ASan `heap-use-after-free`**.

> [!IMPORTANT]
> **Fundamental Theorem of Graph Teardown**:
> No single linear pass over an arbitrary graph can safely interleave object destruction with cascading reference decrements.

______________________________________________________________________

## 4. Architectural Solutions & Invariants

### Pattern A: Decoupling Payload Destruction from Graph Release

Object destruction must be split into two separate concerns:

1. `object_free_payload(obj)` (CPython's `tp_free` / buffer deallocation):
   - Only frees private heap memory owned by this struct.
   - Never inspects or decrements child references.
1. `object_decref_children(obj, live_only)` (CPython's `tp_clear` / edge breaking):
   - Only manages graph ownership by decrementing references to children.

### Pattern B: Two-Phase Bulk VM Teardown

During total runtime shutdown, all remaining objects are unconditionally doomed. Simulating graph edge decrements is both a performance penalty and an algorithmic liability.

- **Pass 1 (Resource Release):** Iterate all objects linearly; call `object_free_payload(obj)` to release all owned buffers.
- **Pass 2 (Header Release):** Iterate all objects linearly; call `free(obj)`.
- **Complexity:** Strict $O(N)$ linear memory access, zero pointer chasing, zero risk of use-after-free.

### Pattern C: Selective Cycle Sweeping (The Live-Child Invariant)

When the cycle collector sweeps unreachable (unmarked) objects:

- **If a child is unmarked (dead):** The child is part of the dead cycle being collected in this same sweep. Cascading a decref to it is redundant and triggers use-after-free if the child was already visited.
- **If a child is marked (live):** The child is rooted in an active call stack. Because the dead container is being destroyed, the container **must** release its reference to the surviving child (`refcount_dec(live_child)`).
- **The Invariant:**
  ```c
  if (obj == NULL) return;
  if (!live_only || obj->is_marked) {
    refcount_dec(obj);
  }
  ```

### Pattern D: Mark Bit Phase Integrity

- Mark bits cannot be cleared on surviving objects while dead containers are still inspecting child mark bits.
- Cycle sweeping must be strictly phased:
  1. **Phase 1:** Sever references from dead containers to live children (`live_only == true`), and free dead payloads.
  1. **Phase 2:** Deallocate dead headers, and unmark surviving objects (`obj->is_marked = false`) for the next GC generation.
  1. **Phase 3:** Compact the surviving object registry and re-index tracker IDs.

______________________________________________________________________

## 5. Performance Engineering: Hot vs. Cold Path Optimization

### Where Inlining Belongs in Reference Counting

- **The Hot Path (Inline aggressively):**
  - `refcount_inc()` and the non-zero branch of `refcount_dec()`:
    ```c
    obj->refcount--;
    if (obj->refcount > 0) return;
    ```
  - In a production VM, this executes billions of times. Inlining it (as a macro or `static inline` header function) eliminates function call overhead and branch prediction penalties.
- **The Cold Path (Keep out-of-line):**
  - The zero branch (`object_free(obj)`):
  - Object deallocation happens only once per object lifecycle. It performs system allocator calls (`free()`) and graph traversal. The function call overhead is completely unmeasurable against `free()`, while keeping it out-of-line keeps the instruction cache (I-cache) compact for hot application code.

______________________________________________________________________

## 6. Systems Programming & Tooling Insights

### 1. Unsigned Integer Loop Underflow

- **The Trap:** Writing `for (size_t i = 0; i < count; i--)` instead of `i++`.
- **The Result:** Because `size_t` is unsigned, `0 - 1` wraps to `SIZE_MAX` (18,446,744,073,709,551,615). The condition `SIZE_MAX < count` evaluates to `false` immediately, aborting the loop after a single iteration without emitting compiler warnings, resulting in silent memory leaks.

### 2. AddressSanitizer (ASan) Shadow Memory

- ASan does not just alert on illegal memory access; its shadow memory maps track exactly:
  - Where the memory block was allocated (`calloc`/`malloc` callstack).
  - Where the memory block was freed (`free` callstack).
  - Where the illegal dereference occurred (`heap-use-after-free`).
- When diagnosing graph deallocation issues, comparing the `freed by thread` callstack directly with the `READ of size 8` callstack immediately reveals inverted parent-child traversal dependencies.

### 3. Adversarial Testing Across GC Boundaries

- A hybrid memory manager must be tested at the seam between reference counting and tracing:
  1. Verify that cycles trapped with `refcount >= 1` are correctly identified and collected by tracing.
  1. Verify that self-referencing containers ($A \\rightarrow A$) traverse cleanly without infinite recursion.
  1. Verify that dead cycles pointing to live rooted values accurately decrement the survivor's reference count without prematurely freeing it.
