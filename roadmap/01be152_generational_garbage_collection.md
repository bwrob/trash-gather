# Milestone: Dual-Generation Tracking & Survivor Promotion

**ID:** `01be152`\
**Status:** Planned\
**Difficulty:** 3 / 5\
**Focus:** Introduce multi-generation object tracking (Gen 0 Nursery, Gen 1 Mature), survivor age counters on object headers, and automated survivor promotion during full garbage collection sweeps.\
**Prerequisites:** [Automatic GC Pacing & Allocation Thresholds](eaa403e_automatic_gc_pacing_and_thresholds.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Add generational tracking metadata to `struct Object`: `uint8_t generation` (0 = young nursery, 1 = mature) and `uint8_t survival_count` (number of collections survived).
   - Segregate the VM object tracking list into two distinct lists: `vm->gen0_objects` and `vm->gen1_objects`.
   - Update `vm_track_object()` so all newly allocated objects initially enter `gen0_objects`.
   - Implement survivor promotion logic: during full garbage collection sweeps, increment `survival_count` for surviving Gen 0 objects; if `survival_count >= PROMOTION_THRESHOLD` (e.g. 2), unlink the object from `gen0_objects` and promote it to `gen1_objects`.
   - Instrument VM telemetry to report generation counts (`gen0_count`, `gen1_count`, `promoted_count`).
1. **Scope Boundaries**:
   - Write barriers and minor nursery-only collections are deferred to Milestone `034b527_generational_write_barriers_and_minor_gc.md`.
   - Compacting memory moves are an explicit non-goal.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Segregated generation lists in `vm_t`:
     ```
     vm_t
       +---> gen0_objects: [Newly allocated transient objects]
       |       (Refreshed frequently; high mortality rate)
       |
       +---> gen1_objects: [Long-lived objects & Singletons]
               (Accumulates promoted survivors)
     ```
   - Survivor promotion lifecycle:
     $$\\text{Alloc} \\rightarrow (\\text{gen} = 0, \\text{surv} = 0) \\xrightarrow{\\text{Sweep 1}} (\\text{gen} = 0, \\text{surv} = 1) \\xrightarrow{\\text{Sweep 2}} (\\text{gen} = 1, \\text{surv} = 2)$$
1. **Core Systems Invariants**:
   - Segregated partition invariant: Every live object belongs to exactly one tracking list (`gen0` XOR `gen1`), never both and never neither.
   - Singleton maturity: Immortal singletons (`None`, `True`, `False`, small integers) are directly initialized in `gen1_objects` with mature status.
   - Sweep completeness: In this milestone, full collections continue to trace all roots across both generations, guaranteeing no live object is prematurely collected.
1. **Architectural Trade-offs**: Adding generation and survival counters to headers slightly increases object state, but provides the empirical foundation for generational GC without yet adding the runtime overhead of write barriers.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: The Weak Generational Hypothesis (in dynamic languages, most objects have short lifespans); generational segregated tracking; survivor tenure and threshold tuning.
1. **Socratic Inquiries**:
   - Why do transient objects (temporary strings, function results, loop tuples) overwhelmingly die during their very first collection pass?
   - What is the danger of setting the promotion threshold too low (e.g., $K = 0$)? What happens to the mature generation?
   - How does segregated list tracking simplify moving objects between generations compared to physical memory compaction?
1. **Failure Modes & Pitfalls**: Losing an object from tracking during promotion unlinking/re-linking; double counting objects in telemetry; failing to untrack promoted objects when freed.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Add `uint8_t generation;` and `uint8_t survival_count;` to `struct Object` in `src/object.h`.
   - Replace `vm_stack_t *objects;` in `vm_t` (`src/vm.h`) with `vm_stack_t *gen0_objects;` and `vm_stack_t *gen1_objects;`.
   - Update `vm_new` and `vm_free` in `src/vm.c` to allocate and tear down both generational lists.
   - Update `vm_track_object` to push to `gen0_objects`.
   - Update `sweep()` in `src/vm.c` to sweep both lists, increment `survival_count`, and move promoted objects from `gen0` to `gen1`.
   - Add unit tests in `tests/test_generational.c` verifying promotion thresholds and telemetry.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/vm.h`, `src/vm.c`
   - `tests/test_generational.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify transient objects die in Gen 0; verify objects retained across 2 GC passes promote to Gen 1; verify singletons start in Gen 1; verify zero dangling pointers during promotion.
1. **Zero-Leak Guarantee**: Verify full deallocation of both generations via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______________________________________________________________________

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [The Weak Generational Hypothesis](https://en.wikipedia.org/wiki/Generational_garbage_collection#Weak_generational_hypothesis): Empirical observation that most allocated objects have extremely short lifespans.
   - [The Garbage Collection Handbook: Generational Systems](https://gchandbook.org/): Theoretical foundations of multi-generation partition collectors and survivor tenure.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython Generational Garbage Collector Internals](https://devguide.python.org/internals/garbage-collector/): How Python segregates objects into generations (Gen 0, 1, 2) and tunes collection frequencies.
   - [V8 Generational Garbage Collection (Trash Talk)](https://v8.dev/blog/trash-talk): How Google Chrome's V8 engine designs young and old generation heaps and optimizes pause times.
