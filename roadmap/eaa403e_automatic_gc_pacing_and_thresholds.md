# Milestone: Automatic GC Pacing & Allocation Thresholds

**ID:** `eaa403e`\
**Status:** Planned\
**Focus:** Transition from purely manual collection calls to an automatic, threshold-driven GC pacing engine triggered during memory allocation.\
**Prerequisites:** [Garbage Collector Telemetry & Allocation Statistics](330a2b1_gc_telemetry_and_metrics.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Implement configurable allocation thresholds within `vm_t`; track cumulative heap allocations since the last cycle collection; automatically trigger cycle sweeps when thresholds are crossed during `vm_new_object()`; provide user-facing control APIs (`vm_gc_enable()`, `vm_gc_disable()`, `vm_gc_set_threshold()`).
1. **Scope Boundaries**: Multi-generational nursery promotion thresholds and adaptive heap resizing heuristics are deferred to Milestone 16.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Threshold tracking and control counters embedded in `vm_t`:
     ```
     [vm_new_object() Allocation Call]
                   │
                   ▼
     [Allocation Counter Check]
       allocs_since_gc >= alloc_threshold ?
            ├── YES ──> [Invoke gc_collect()] ──> Reset Counter to 0
            └── NO  ──> Increment Counter & Return Object
     ```
1. **Core Systems Invariants**:
   - Re-entrancy prevention invariant: The garbage collector must never trigger recursively while a collection pass is already in progress (`is_collecting == true`).
   - In-flight root protection invariant: If an automatic collection fires during an allocation, all partially initialized objects and temporary operands must be shielded from premature sweep.
   - Idempotent manual control invariant: Disabling automatic collection (`vm_gc_disable()`) must strictly suppress background triggers while keeping manual collections (`vm_collect()`) fully operational.
1. **Architectural Trade-offs**: Background automatic collection keeps heap memory footprints bounded without manual developer intervention, but introduces periodic pause times during allocations; configurable thresholds allow tests to run deterministically while supporting long-running programs.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Garbage collection pacing; allocation rate vs reclamation rate; amortized time complexity; CPython's collection thresholds (`gc.get_threshold()`).
1. **Socratic Inquiries**:
   - What happens if the VM triggers an automatic collection in the middle of creating a list, before the list pointer has been stored into a frame root?
   - How does tuning the allocation threshold higher or lower affect program throughput versus peak memory usage?
   - Why must an automatic GC trigger check an `is_collecting` re-entrancy flag before calling the cycle collector?
1. **Failure Modes & Pitfalls**: Recursive GC invocation during internal deallocations; collecting partially-initialized objects that have not yet been anchored to a stack root; infinite collection loops if sweep fails to reclaim memory.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. Add threshold fields (`uint64_t alloc_threshold`, `uint64_t allocs_since_gc`, `bool gc_auto_enabled`, `bool is_collecting`) to `vm_t` in `include/vm.h`.
   1. Initialize default thresholds (e.g. 50 allocations) in `vm_new()` in `src/vm.c`.
   1. Implement threshold checking and conditional collection dispatch in `vm_new_object()` in `src/vm.c`.
   1. Expose control functions (`vm_gc_enable()`, `vm_gc_disable()`, `vm_gc_set_threshold()`) in `include/vm.h` and `src/vm.c`.
   1. Write adversarial unit tests in `tests/test_vm.c` verifying that cyclic structures are automatically collected under allocation churn without manual `vm_collect()` calls.
1. **File Touchpoints**:
   1. `include/vm.h`
   1. `src/vm.c`
   1. `tests/test_vm.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Create cyclic garbage in a tight loop without manual `vm_collect()` calls, asserting that the heap object count remains bounded as the automatic collector fires; test that disabling auto-GC suppresses collections.
1. **Zero-Leak Guarantee**: All automatically collected cycles and surviving objects are completely accounted for, exiting cleanly with `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero warnings under ASan/UBSan.
