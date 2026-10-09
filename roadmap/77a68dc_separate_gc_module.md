# Milestone: Separate Garbage Collection from Virtual Machine Module

**ID:** `77a68dc`\
**Status:** Planned\
**Difficulty:** 1 / 5\
**Focus:** Decouple cyclic mark-and-sweep garbage collection routines into a dedicated `gc.h` and `gc.c` module, separating memory reclamation mechanics from virtual machine runtime state and execution frame management.\
**Prerequisites:** [Hybrid GC Runtime](393f420_hybrid_gc_runtime.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Extract tri-color mark, trace, and sweep algorithms from `src/vm/vm.c` and `src/vm/vm.h` into dedicated `src/vm/gc.c` and `src/vm/gc.h`.
   - Define clean, minimal public collector entry points (`gc_collect()`, `gc_mark()`, `gc_trace()`, `gc_sweep()`).
   - Encapsulate internal traversal routines (`trace_blacken_object()`, `trace_mark_object()`) as static helpers inside `src/vm/gc.c`, removing them from the public header surface.
   - Retain runtime execution responsibilities (VM lifecycle `vm_new`/`vm_free`, stack frames, immortal singletons, object tracking lists) strictly in `src/vm/vm.c` and `src/vm/vm.h`.
   - Update tests, header DAG generator, and documentation to reflect the modularized architecture.
1. **Scope Boundaries**:
   - Generation tracking and pause-time pacing are deferred to `01be152_dual_generation_tracking.md` and `eaa403e_automatic_gc_pacing.md`.
   - GC telemetry counters and allocation metrics are deferred to `330a2b1_gc_telemetry_metrics.md`.

______

## 2. Architectural Design & Invariants

1. **Modular Architecture & Header DAG**:
   - Target dependency layering:

     ```text
     +-------------------------------------------------------------+
     |                          gc.h / gc.c                        |
     |   - gc_collect()                                            |
     |   - gc_mark() / gc_trace() / gc_sweep()                     |
     |   - static trace_blacken_object() / trace_mark_object()      |
     +------------------------------+------------------------------+
                                    |
                                    v (queries roots & tracker)
     +-------------------------------------------------------------+
     |                          vm.h / vm.c                        |
     |   - vm_new() / vm_free()                                    |
     |   - vm_frame_push() / vm_frame_pop() / frame_t              |
     |   - vm_track_object() / vm_untrack_object()                 |
     |   - vm_get_current() / immortals                            |
     +------------------------------+------------------------------+
                                    |
                                    v
     +-------------------------------------------------------------+
     |                       vm_objects / stack                    |
     |   - object_t, tuple_t, list_t, vm_stack_t                   |
     +-------------------------------------------------------------+
     ```

2. **Core Systems Invariants**:
   - Single-Direction Dependency Invariant: `gc.c` depends on `vm.h` to discover GC roots (`vm->frames`) and tracked heap allocations (`vm->objects`), but `vm.h` MUST NOT depend on `gc.h` (strict acyclic DAG).
   - Zero-Leak Invariant: Moving GC logic into a separate compilation unit must preserve 100% equivalence in heap reclamation, ensuring `assert(boot_all_freed())` holds across the entire test suite.
   - Header Self-Containment: `src/vm/gc.h` must be strictly self-contained and compile cleanly in isolation under `gcc -Wall -Wextra -Werror -std=c17 -fsyntax-only` (`just lint-headers-standalone`).
3. **Architectural Trade-offs**:
   - Exposing runtime root inspection to `gc.c` vs. keeping `CURRENT_VM` monolithic: Splitting modules increases modularity and compilation speed, but requires `gc.c` to access VM root abstractions cleanly via `vm_get_current()`.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Single Responsibility Principle in systems programming; C translation unit isolation; separation of runtime execution state (mutator) from memory reclamation subsystem (collector); CPython's architecture (`Modules/gcmodule.c` vs `Python/pystate.c`).
2. **Socratic Inquiries**:
   - In CPython, why is `gcmodule.c` separated from `pystate.c` and the core evaluation loop in `ceval.c`?
   - Why is exposing internal traversal helpers like `trace_blacken_object()` in a public header file an encapsulation leak, and how does making them `static` inside `gc.c` improve API surface stability?
   - How does separating collector logic into its own translation unit prepare the engine for generational collection and allocation pacing?
3. **Failure Modes & Pitfalls**:
   - Creating circular header includes (`vm.h` including `gc.h` while `gc.h` includes `vm.h`).
   - Breaking existing unit tests that directly assert individual phases (`vm_mark()`, `vm_sweep()`) without updating test call sites or providing backward-compatible aliases if desired.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - 1. Create `src/vm/gc.h` declaring the garbage collector interface (`gc_collect`, `gc_mark`, `gc_trace`, `gc_sweep`).
   - 2. Create `src/vm/gc.c` moving `vm_mark`, `vm_trace`, `vm_sweep`, `vm_collect_garbage`, `trace_blacken_object`, and `trace_mark_object` from `src/vm/vm.c`.
   - 3. Make `trace_blacken_object` and `trace_mark_object` `static` helper functions in `src/vm/gc.c`.
   - 4. Clean up `src/vm/vm.h` and `src/vm/vm.c` to focus purely on VM instance lifecycle, stack frames, immortals, and tracking registry.
   - 5. Update unit test suites in `tests/` to target the `gc_*` interfaces and include `gc.h`.
   - 6. Regenerate header DAG (`just update-header-dag`) and verify DAG validity with `just lint-header-dag`.
   - 7. Verify standalone header self-containment with `just lint-headers-standalone`.
   - 8. Verify line coverage (`just coverage`) and leak tracking (`just test`).
2. **File Touchpoints**:
   - `src/vm/gc.h`: Collector declarations.
   - `src/vm/gc.c`: Mark-and-sweep implementation.
   - `src/vm/vm.h`: Pruned VM and stack frame definitions.
   - `src/vm/vm.c`: VM runtime lifecycle and frame operations.
   - `tests/test_gc.c`: Updated test fixtures calling `gc_*`.
   - `docs/header_dag.mmd` & `README.md`: Updated header architecture DAG.

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: All existing GC tests in `tests/test_gc.c` run cleanly and exercise the refactored `gc_*` routines.
2. **Zero-Leak Guarantee**: Leak tracking with `assert(boot_all_freed())` passes with 0 bytes retained across all tests.
3. **Tooling Quality Gates**:
   - `just build`: Sandboxed binary builds cleanly.
   - `just test`: 100% test pass rate under ASan/UBSan.
   - `just coverage`: 100.00% line coverage maintained across all files in `src/` including both `vm.c` and `gc.c`.
   - `just lint-headers-standalone`: Both `vm.h` and `gc.h` pass self-containment checks.
   - `just lint-header-dag`: Header dependency graph remains a strict, cycle-free DAG.
   - `just check`: All pre-commit hooks pass.
4. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson file in `lessons/` following the `lesson-extraction` skill.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Robert C. Seacord — *Effective C: An Introduction to Professional C Programming* (No Starch Press, 2020)](../resources/seacord_2020_effective_c.pdf): Chapter 10 (Program Structure, §Componentization & Linkage), detailing modular translation unit design, opaque struct pointers, and decoupling runtime subsystems.
   - [Richard Jones, Antony Hosking, Eliot Moss — *The Garbage Collection Handbook* (CRC Press, 2nd Ed, 2023)](../resources/jones_2023_garbage_collection_handbook.pdf): Chapter 1 (Introduction, §1.2 Mutators and Collectors), defining the formal interface boundary between mutator VM and collector.
   - [The Garbage Collection Handbook (Jones, Hosking, Moss)](https://gchandbook.org/): Chapters 1-3 on Mark-Sweep collector decoupling from runtime state.
   - [CPython Internal GC Architecture (`Modules/gcmodule.c`)](https://github.com/python/cpython/blob/main/Modules/gcmodule.c): Study how CPython isolates cyclic garbage collection into a dedicated module that inspects interpreter state.
2. **After Implementation (Deep Dives & Systems Context)**:
   - [Lua 5.4 Garbage Collector Architecture (`lgc.c`)](https://www.lua.org/source/5.4/lgc.c.html): Reference implementation of an incremental mark-and-sweep collector decoupled from the virtual machine core (`lvm.c`).
