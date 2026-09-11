# Milestone: Heap-Allocated Variable-Length Tuple

**ID:** `f9c475f`\
**Status:** Completed\
**Focus:** Implement Python-style arbitrary-length immutable tuples via heap-allocated `tuple_t` with a C99 flexible array member, contiguous allocation math, and GC lifecycle integration.\
**Prerequisites:** [Hybrid Reference Counting & Cycle Collection Runtime](393f420_hybrid_gc_runtime.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Replace the fixed 3-element tuple representation with an arbitrary-capacity, heap-allocated `tuple_t` payload using a C99 flexible array member (struct hack), implement `new_tuple(size_t size)`, and integrate into GC tracing and sweeping.
1. **Scope Boundaries**: Eliminating the tagged union and unifying `object_t` with the tuple payload into a single allocation is deferred to Milestone 04.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Contiguous payload structure:
     ```c
     typedef struct {
       size_t size;
       object_t *items[];
     } tuple_t;
     ```
   - In `object_data_t`, `v_tuple` is stored as a pointer (`tuple_t *v_tuple`), preserving uniform `sizeof(object_t)`.
1. **Core Systems Invariants**:
   - Allocation math invariant: Payload size is computed as $\\text{sizeof(tuple_t)} + \\text{size} \\times \\text{sizeof(object_t\*)}$.
   - Slot safety invariant: All slots in `items[0 .. size-1]` must be initialized to `NULL` before returning.
   - Reclamation invariant: Sweeping a dead `TUPLE` must invoke `free(obj->data.v_tuple)` inside `object_free_payload()` prior to deallocating the parent `object_t`.
1. **Architectural Trade-offs**: Storing `tuple_t *` as a pointer in `object_data_t` requires two allocations per tuple, but avoids altering the uniform `object_t` size and tracking infrastructure.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: C99 flexible array members (§6.7.2.1); union constraints; contiguous memory layout vs pointer arrays.
1. **Socratic Inquiries**:
   - Why can't a flexible array member appear directly inside a union?
   - How does tuple allocation differ from list allocation in terms of buffer resizing and immutability?
   - How does the runtime handle allocation failure rollback if `new_object` succeeds but `malloc(payload)` fails?
1. **Failure Modes & Pitfalls**: Uninitialized slot dereferences during GC tracing; memory leaks on failed allocation paths; integer overflow during size multiplication.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Update `tuple_t` in `src/object.h` with `size_t size;` and `object_t *items[];`.
   - Update `object_data_t` in `src/object.h` to declare `tuple_t *v_tuple;`.
   - Implement `new_tuple(size_t size)` in `src/new.c` and `src/new.h`.
   - Implement `tuple_set()` and `tuple_get()` accessors in `src/object.c` and `src/object.h`.
   - Update `trace_blacken_object()` in `src/vm.c` to iterate over tuple items.
   - Update `object_decref_children()` and `object_free_payload()` in `src/object.c`.
   - Create adversarial tests in `tests/test_new.c` and `tests/test_object.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/new.h`, `src/new.c`
   - `src/vm.c`
   - `tests/test_new.c`, `tests/test_object.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify empty tuples ($N = 0$), singletons ($N = 1$), large tuples ($N = 1{,}000$), and simulated allocation failures via `bootlib`.
1. **Zero-Leak Guarantee**: Confirm `assert(boot_all_freed())` passes at the end of all test cases.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass with zero errors.
