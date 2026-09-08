# Lesson 02: Hybrid RC + Mark-and-Sweep & De-Sneking

**Branch:** `hybrid-gc`
**Focus:** Refactoring to a Clean Runtime API and Implementing Hybrid Reference Counting + Mark-and-Sweep Cycle Detection (CPython Model)

---

## 1. System Engineering: C Namespace & Header Collisions

### The macOS Darwin `stack_t` Collision
- **Problem:** When de-sneking types (`snek_stack_t` -> `stack_t`), builds on macOS immediately broke with obscure compiler errors claiming `stack_t` was already declared.
- **Root Cause:** In POSIX / Darwin systems, `<signal.h>` (often included indirectly via standard library headers) defines `typedef struct sigaltstack stack_t;` for managing alternate signal stacks.
- **Lesson:** C does not have namespaces. Generic names like `stack_t`, `list_t`, or `node_t` are prone to collisions with OS headers and standard libraries. Always prefix runtime types with a subsystem identifier (e.g. `vm_stack_t`).

---

## 2. Global VM Singleton Architecture

- **Evolution:** The original API passed `vm_t *vm` explicitly to almost every constructor and operation (`new_snek_integer(vm, 42)`).
- **Refactoring:** Introduced a thread-local / static `CURRENT_VM` singleton managed by `vm_new()` and `vm_free()`.
- **Outcome:** Clean, ergonomic zero-argument constructors (`new_integer(42)`, `new_array(5)`) mirroring real-world runtime APIs (like Lua or CPython), reducing boilerplate across tests.

---

## 3. The Hybrid Model: Reference Counting + Tracing GC

### Why Pure Reference Counting is Not Enough
- Reference Counting provides immediate, deterministic reclamation as soon as an object's `refcount` hits 0.
- **Limitation:** Cyclic references (e.g., `A -> B -> A`) cannot be collected by RC alone. Even if detached from all roots, their reference counts remain $\ge 1$, causing permanent memory leaks.

### The Role of Mark-and-Sweep in a Hybrid Model
- RC handles short-lived, acyclic objects immediately with zero pause time.
- Tracing GC runs periodically as a **cycle collector**, finding unreachable reference islands that RC missed.

---

## 4. The Single-Pass Deallocation Trap (AddressSanitizer)

### The Dual Nature of `object_free()`
In pure Mark-and-Sweep, `object_free()` only frees the object's own internal payload (e.g., `free(array->elements)`). It never touches children.

In Reference Counting, `object_free()` must cascade ownership release to children:
```c
case ARRAY: {
  for (size_t i = 0; i < arr.size; i++) {
    refcount_dec(arr.elements[i]); // Drops child refcount
  }
  free(arr.elements);
  break;
}
```

### The Conflict During Global Iteration
When a linear loop iterates over `CURRENT_VM->objects` and calls `object_free()` directly on each object:

1. **If iterating backwards (`vm_free()`):**
   - Assumes parents were allocated *after* children.
   - If an array is allocated *before* its elements (`arr = new_array(2); elem = new_string(...);`), the backward loop frees `elem` first.
   - When the loop reaches `arr`, `object_free(arr)` calls `refcount_dec(elem)` on the already-freed pointer.
   - **Result:** `AddressSanitizer: heap-use-after-free` in `refcount_dec`.

2. **If iterating forwards (`sweep()`):**
   - Assumes parents were allocated *before* children.
   - If elements are allocated *before* a vector (`i1 = new_integer(...); v = new_vector3(i1, ...);`), the forward loop frees `i1` first.
   - When the loop reaches `v`, `object_free(v)` calls `refcount_dec(i1)` on the already-freed pointer.
   - **Result:** `AddressSanitizer: heap-use-after-free` in `refcount_dec`.

### The Fundamental Rule
**No single linear pass can safely reclaim an arbitrary object graph if `object_free()` simultaneously destroys the object and cascades decrements to its children.**

---

## 5. Architectural Solutions

### A. Two-Phase VM Teardown (`vm_free()`)
At VM shutdown, all remaining objects in `CURRENT_VM->objects` are known to be dead. Cascading `refcount_dec` is neither necessary nor safe:
- **Phase 1 (Break References / Free Buffers):** Walk all objects; free payload buffers (`v_string`, `v_array.elements`) without calling `refcount_dec()`.
- **Phase 2 (Deallocate Headers):** Walk all objects; call `free(obj)`.

### B. Two-Phase Cycle Sweep (`sweep()`)
When sweeping unreachable objects:
- **Phase 1 (Selective Decref & Buffer Release):** For each unmarked object:
  - If a child is **marked (live)**: call `refcount_dec(child)` because the dead container is releasing its reference to an active object.
  - If a child is **unmarked (dead)**: do *not* decref it; it is part of the dead cycle and will be freed in Phase 2.
  - Free the container's payload buffer.
- **Phase 2 (Deallocate Dead Headers):** For each unmarked object: `free(obj)`.

### C. The CPython GC Rule: Don't Track Primitives
- Integers, floats, and strings cannot hold references to other objects; they can never participate in reference cycles.
- Only container types (`ARRAY`, `VECTOR3`) should ever be registered in `CURRENT_VM->objects`.
- Primitives should be managed exclusively by immediate reference counting.

### D. Array Compaction Invariant
- When calling `stack_remove_nulls(CURRENT_VM->objects)` after a sweep, all surviving objects shift to lower indices.
- Any cached tracker index (`obj->tracker_id`) must be re-synchronized to its new index in the stack.

---

## 6. Development Workflow & Tooling

- **Sanitizers as Learning Accelerators:** ASan did not just say "segmentation fault"—it provided the exact callstack of where the memory was allocated, where it was freed, and where the illegal read occurred.
- **Targeted Test Execution:** Using `just test-filter <name>` allows immediate, focused feedback on individual failing tests without waiting for the full 72-test suite.
