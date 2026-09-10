# Milestone: Generational Garbage Collection

**ID:** `01be152`\
**Status:** Planned\
**Focus:** Implement multi-generation garbage collection leveraging the weak generational hypothesis, nursery allocations, survivor promotion, and write barriers.\
**Prerequisites:** [Automatic GC Pacing & Allocation Thresholds](eaa403e_automatic_gc_pacing_and_thresholds.md), [Weak References & Non-Owning Pointers (weakref_t)](a7ca40b_weak_references_and_non_owning_pointers.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Implement a two-generation garbage collector (Generation 0: Nursery, Generation 1: Mature), minor collections for Gen 0, survivor promotion age thresholds, and a write barrier with a remembered set.
1. **Scope Boundaries**: Compacting nurseries and copying collectors are deferred to advanced memory compaction milestones.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Generational metadata per object:
     ```c
     typedef struct {
       uint8_t generation;     // 0 = young, 1 = mature
       uint8_t survival_count; // Number of minor GCs survived
     } gc_meta_t;
     ```
   - Segregated VM tracking lists: `CURRENT_VM->gen0` and `CURRENT_VM->gen1`.
1. **Core Systems Invariants**:
   - Minor GC root set invariant: Minor collections trace only roots from active stack frames plus the **Remembered Set** (old-to-young pointers).
   - Write barrier invariant: Whenever an object in Gen 1 is modified to reference an object in Gen 0 (`old->field = young`), the runtime must invoke the write barrier to log the parent into the remembered set.
   - Promotion invariant: Objects that survive $K$ minor GC passes (e.g. $K = 2$) are promoted from Gen 0 to Gen 1.
1. **Architectural Trade-offs**: Minor GC pauses are orders of magnitude faster than full sweeps because only the nursery is examined, but every pointer mutation incurs write barrier overhead.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: The Weak Generational Hypothesis (most objects die young); minor vs major collection cycles; write barriers and card marking.
1. **Socratic Inquiries**:
   - Why does pure Mark-and-Sweep scale poorly as heap residency grows into millions of objects?
   - How does a write barrier prevent minor GCs from having to scan the entire mature generation?
   - How does CPython's generational GC track generations without moving objects in memory?
1. **Failure Modes & Pitfalls**: Missing a write barrier invocation on container mutations (causing premature collection of live young objects); remembered set bloat; nursery starvation.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Add generational tracking metadata to object headers in `src/object.h`.
   - Segregate VM tracking into `gen0` and `gen1` lists in `src/vm.h` and `src/vm.c`.
   - Implement the remembered set structure in `src/vm.c`.
   - Implement write barrier helper `gc_write_barrier(object_t *parent, object_t *child)`.
   - Implement `vm_collect_minor()` and survivor promotion in `src/vm.c`.
   - Insert write barriers into container mutations (`list_set`, `dict_set`, etc.).
   - Write unit tests in `tests/test_generational.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/vm.h`, `src/vm.c`
   - `bench/bench_gc.cpp`
   - `tests/test_generational.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify minor GC collects young dead objects without scanning mature objects, and survivor promotion elevates long-lived nodes.
1. **Zero-Leak Guarantee**: Inter-generational cycle and mutation tests confirm zero leaks with `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: Microbenchmark in `bench/bench_gc.cpp` demonstrates statistically significant pause-time reduction for transient churn.
