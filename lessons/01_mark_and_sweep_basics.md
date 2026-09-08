# Lesson 01: Foundational Mark-and-Sweep Garbage Collection

**Branch:** `original-bootdev-course` (Merged in PR #1)
**Focus:** Stop-the-World Tracing Garbage Collector in C

---

## 1. Core Architecture & Concepts

### 1.1 Roots, Frames, and Reachability
- Garbage collection begins with a set of known live pointers called **roots**.
- In this VM runtime, roots are held inside **call frames** (`frame_t`). Each frame maintains a stack of references to objects currently active in that execution scope.
- If an object is not reachable via any path starting from the roots, it is dead and eligible for reclamation.

### 1.2 Tri-Color Marking Abstraction
Although implemented with boolean flags in this initial iteration, the algorithm follows the classical tri-color abstraction:
- **White:** Unvisited objects (potential garbage). Initially, all objects.
- **Gray:** Reachable objects whose referenced children have not yet been visited.
- **Black:** Reachable objects whose referenced children have all been visited.

### 1.3 The GC Pipeline: Mark, Trace, Sweep
1. **Mark (`mark()`):** Iterate through all active call frames in the VM. For each referenced object, set `obj->is_marked = true` (turns white roots to gray).
2. **Trace (`trace()`):** Using a temporary gray stack (`gray_objects`), iteratively pop objects, inspect their fields (`VECTOR3` components, `ARRAY` elements), and mark unvisited children (`trace_mark_object()`), pushing them onto the gray stack until it is empty (all reachable objects turn black).
3. **Sweep (`sweep()`):** Walk the global VM object allocation list (`vm->objects`).
   - If `obj->is_marked == true`: reset `obj->is_marked = false` for the next cycle.
   - If `obj->is_marked == false`: deallocate the object using `object_free(obj)` and set the slot to `NULL`.
   - Compact the list via `stack_remove_nulls()`.

---

## 2. Key Takeaways & Design Insights

- **Ownership in Pure Mark-and-Sweep:**
  In pure Mark-and-Sweep, the VM's global object registry owns *all* object allocations. Containers (arrays and vectors) do not need to manage the lifecycles of their children because the VM list guarantees that every child object is independently visited and freed.
- **Leak Detection with `bootlib`:**
  Intercepting `malloc`, `calloc`, `realloc`, and `free` via `bootlib` ensures that every allocated byte must be accounted for. `assert(boot_all_freed())` at the end of each test verifies zero memory leaks.
