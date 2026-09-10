# Milestone: Garbage Collector Telemetry & Allocation Statistics

**ID:** `330a2b1`\
**Status:** Planned\
**Focus:** Instrument the VM with a telemetry stats structure (`gc_stats_t`), tracking total allocations, freed bytes, sweep counts, and live object counts for runtime observability.\
**Prerequisites:** [Comprehensive Runtime Source Documentation & Doxygen Annotations](f06ad6f_document_entire_source.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Add a lightweight profiling and telemetry structure `gc_stats_t` to `vm_t`, tracking lifetime allocations, sweep counts, total bytes allocated and reclaimed, and peak object counts.
1. **Scope Boundaries**: Exporting metrics to Prometheus or external monitoring agents is out of scope; telemetry is exposed via C API and diagnostic dump functions.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Telemetry struct integrated into `vm_t`:
     ```c
     typedef struct {
       size_t total_allocations;
       size_t total_deallocations;
       size_t total_gc_runs;
       size_t total_objects_swept;
       size_t current_live_objects;
       size_t peak_live_objects;
       size_t bytes_allocated;
       size_t bytes_freed;
     } gc_stats_t;

     struct VM {
       ...
       gc_stats_t stats;
     };
     ```
1. **Core Systems Invariants**:
   - Conservation invariant: At any time, $\\text{current_live_objects} == \\text{total_allocations} - \\text{total_deallocations}$.
   - Non-intrusive invariant: Updating metrics counters must occur exclusively on allocation and deallocation code paths without adding branches to hot-path pointer reads.
   - Deterministic reset: `vm_new()` zeros all telemetry counters; `vm_free()` leaves zero uncollected accounting discrepancies.
1. **Architectural Trade-offs**: Incrementing counters on `new_object()` and `object_free()` adds a negligible CPU cost (\<1%) while providing immediate empirical visibility into GC behavior and allocation churn.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Systems telemetry and runtime observability; accounting invariants; measuring GC pause efficiency and throughput.
1. **Socratic Inquiries**:
   - How can you measure the duration of a GC cycle in microseconds ($\\mu\\text{s}$) using POSIX monotonic clocks (`clock_gettime(CLOCK_MONOTONIC, ...)` or macOS `mach_absolute_time`)?
   - How can telemetry detect a memory leak early during integration testing?
   - What is the difference between measuring live heap object count vs total resident heap bytes?
1. **Failure Modes & Pitfalls**: Using wall-clock time (`gettimeofday`) which can jump backwards due to NTP adjustments; integer overflow on byte counters; forgetting to increment/decrement counters on specialized allocation paths.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Define `gc_stats_t` in `src/vm.h`.
   - Add `gc_stats_t stats;` to `struct VM` in `src/vm.h`.
   - Instrument `new_object()` in `src/new.c` to increment `total_allocations`, `current_live_objects`, and update `peak_live_objects`.
   - Instrument `object_free()` in `src/object.c` to increment `total_deallocations` and decrement `current_live_objects`.
   - Instrument `vm_collect_garbage()` in `src/vm.c` to track `total_gc_runs` and `total_objects_swept`.
   - Implement `gc_stats_t vm_get_stats(void)` and `void vm_stats_print(void)` in `src/vm.c` and `src/vm.h`.
   - Write unit tests in `tests/test_vm.c`.
1. **File Touchpoints**:
   - `src/vm.h`, `src/vm.c`
   - `src/new.c`
   - `src/object.c`
   - `tests/test_vm.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify that allocating 100 objects and running GC accurately reflects 100 allocations, the exact number of objects swept, and the remaining live count.
1. **Zero-Leak Guarantee**: Telemetry calls introduce zero heap allocations and produce zero leaks verified via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero errors.
