# Milestone: Hybrid Reference Counting & Cycle Collection Runtime

**ID:** `393f420`\
**Status:** Completed\
**Focus:** Implement immediate Reference Counting alongside Mark-and-Sweep cycle collection, resolving the single-pass deallocation trap and POSIX header collisions.\
**Prerequisites:** [Learning-Friendly Modern Tooling & Safety Infrastructure](d9c6780_learning_friendly_setup.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Upgrade the Mark-and-Sweep VM to a CPython-style hybrid memory management model, resolve POSIX namespace collisions (de-sneking), and implement a two-phase sweep reclamation pipeline.
1. **Scope Boundaries**: Arbitrary-length sequence containers are deferred to subsequent milestones.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**: Add `size_t refcount;` to `struct Object`. Directed reference edges increment target refcounts upon assignment and decrement upon detachment.
1. **Core Systems Invariants**:
   - Two-phase sweep invariant: Phase 1 decrements child references across dead objects (`live_only = true`), severing cyclic links; Phase 2 deallocates dead nodes, guaranteeing no child is freed while a parent still references its pointer.
   - Namespace hygiene invariant: Avoid leading underscores on non-static functions in C headers to prevent collisions with POSIX/ISO C system headers.
1. **Architectural Trade-offs**: Hybrid GC adds runtime cost on pointer mutation (`refcount_inc`/`dec`) but enables immediate zero-pause deallocation for acyclic objects, reserving tracing sweeps for cycles.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Reference counting vs graph reachability; cyclic digraph unreachability; the Single-Pass Deallocation Trap (ASan `heap-use-after-free`).
1. **Socratic Inquiries**:
   - Why does deallocating parent and child in arbitrary order in a single sweep pass lead to use-after-free under ASan?
   - Why is pure reference counting insufficient for self-referencing cycles ($A \\to B \\to A$)?
1. **Failure Modes & Pitfalls**: Cyclic memory leaks, double-frees during cascading decrefs, and symbol collisions with `<stdio.h>`.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Add `refcount` to `struct Object` in `src/object.h`.
   - Implement `refcount_inc()` and `refcount_dec()` in `src/object.c` and `src/object.h`.
   - Refactor functions with leading underscores (`_stack_*`) to clean public names.
   - Implement two-phase sweep in `src/vm.c`.
   - Write unit tests in `tests/test_refcount.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/vm.h`, `src/vm.c`
   - `src/stack.h`, `src/stack.c`
   - `tests/test_refcount.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify acyclic deallocation, nested freeing, and self-referencing cycles.
1. **Zero-Leak Guarantee**: Confirm 100% reclamation via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass with zero sanitizer warnings.
