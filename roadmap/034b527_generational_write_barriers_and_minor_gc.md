# Milestone: Remembered Sets, Write Barriers & Minor Generational GC

**ID:** `034b527`\
**Status:** Planned\
**Difficulty:** 4 / 5\
**Focus:** Implement the remembered set for old-to-young cross-generational pointers, instrument container mutations with write barriers, and execute lightning-fast minor garbage collections (`vm_collect_minor`) targeting only the young nursery.\
**Prerequisites:** [Dual-Generation Tracking & Survivor Promotion](01be152_generational_garbage_collection.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Implement the **Remembered Set** data structure (`remembered_set_t`) in `src/vm.c` tracking mature objects (Gen 1) that hold pointers to young objects (Gen 0).
   - Implement the write barrier inline helper: `gc_write_barrier(object_t *old_obj, object_t *young_obj)` which logs `old_obj` into the remembered set when an old-to-young reference is established.
   - Instrument all container mutation operations (`list_set`, `list_append`, `list_insert`, `dict_set`) with the write barrier.
   - Implement minor garbage collection `vm_collect_minor()`: trace roots solely from active stack frames and the Remembered Set, sweeping exclusively `gen0_objects` without touching mature objects.
   - Clear and rebuild the remembered set during minor collection cycles.
1. **Scope Boundaries**:
   - Card table bit arrays (for large heap page boundaries) are deferred to advanced generational tuning.
   - Multi-threaded concurrent write barriers are an explicit non-goal.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Old-to-Young Pointer and Remembered Set:

     ```text
     Gen 1 (Mature)                  Gen 0 (Nursery)
     +-------------------+           +-------------------+
     | Mature Dict (A)   | --------> | Young String (B)  |
     +-------------------+           +-------------------+
               ^
               | (Logged by write barrier)
     +-------------------+
     | Remembered Set    |
     | [ Dict A, ... ]   |
     +-------------------+
     ```

   - Minor Collection Root Set:
     $$\\text{Minor Roots} = \\text{Active Stack Frame Roots} \\cup \\text{Remembered Set (Gen 1 } \\rightarrow \\text{ Gen 0)}$$
1. **Core Systems Invariants**:
   - Write barrier invariant: Whenever an object in Gen 1 is mutated to store a pointer to an object in Gen 0, the write barrier must be invoked immediately before the mutation completes.
   - Minor collection safety: No live object in Gen 0 may be swept during a minor collection. Any Gen 0 object reachable from an uncollected Gen 1 object via the remembered set must be marked and preserved.
   - Deduplication invariant: An object in the remembered set must not be processed multiple times in the same minor GC pass.
1. **Architectural Trade-offs**: Minor collections reduce GC pause times from linear in total heap size ($O(N\_{\\text{heap}})$) to linear in nursery size ($O(N\_{\\text{nursery}})$), at the expense of a slight CPU branch penalty on every container pointer mutation.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Cross-generational pointer tracking; Dijkstra and Steele write barriers; card tables vs object-level remembered sets; stop-the-world minor collection pauses.
1. **Socratic Inquiries**:
   - What would happen during a minor collection if a mature list contains the *only* reference to a young string, but that mature list is not in the remembered set?
   - Why do young-to-old pointers (Gen 0 pointing to Gen 1) NOT need to be tracked in the remembered set?
   - When can an object be safely removed from the remembered set?
1. **Failure Modes & Pitfalls**: Missing a write barrier on an obscure mutation path (e.g. `list_insert` or `dict_set`) leading to catastrophic premature collection of live young objects; remembered set bloat if objects are not cleared when references are overwritten.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Implement `remembered_set_t` in `src/vm.h` and `src/vm.c`.
   - Implement `void gc_write_barrier(object_t *parent, object_t *child)` in `src/vm.c`.
   - Insert write barrier calls into `list_set`, `list_append`, `list_insert`, and `dict_set` in `src/object.c`.
   - Implement `void vm_collect_minor(void)` in `src/vm.c`.
   - Update automatic GC pacing to trigger minor collections frequently and major collections infrequently.
   - Add unit and stress tests in `tests/test_generational.c`.
1. **File Touchpoints**:
   - `src/vm.h`, `src/vm.c`
   - `src/object.h`, `src/object.c`
   - `tests/test_generational.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify minor collections reclaim dead nursery objects without scanning mature objects; verify old-to-young references correctly protect young objects from minor sweep; benchmark minor GC pause times showing substantial speedup over major sweeps.
1. **Zero-Leak Guarantee**: Verify zero memory leaks across minor and major collections via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Dijkstra and Steele Write Barriers in Tracing Collectors](https://en.wikipedia.org/wiki/Write_barrier): Detecting mutations of pointer references to ensure garbage collector correctness.
   - [Remembered Sets and Card Tables](https://gchandbook.org/): Recording old-to-young pointers so minor collections avoid scanning mature heap generations.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython Generational GC Write Barriers and Tracking](https://devguide.python.org/internals/garbage-collector/): How CPython maintains generational invariants and schedules minor vs major sweeps.
   - [V8 Minor GC (Scavenger) Concurrent Marking](https://v8.dev/blog/concurrent-marking): Production write barrier design and remembered set tracking in modern high-speed runtimes.
