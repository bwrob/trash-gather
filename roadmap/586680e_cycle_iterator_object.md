# Milestone: Cycle Iterator Object

**ID:** `586680e`\
**Status:** Planned\
**Focus:** Implement an `itertools.cycle`-style circular iterator object that holds a reference to an underlying sequence, cycles through elements indefinitely via modular arithmetic, and integrates with the cycle collector.\
**Prerequisites:** [Polymorphic Sequence Length Protocol](b81f9a7_polymorphic_sequence_length.md), [Boolean Immortal Singletons & Truthiness](6c3a989_bool_singletons_and_truthiness.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Introduce `OBJ_CYCLE_ITER` kind to the runtime object model in `object_kind_t`.
   - Define an inline payload `cycle_iter_t` in `object_data_t` holding an owning reference `object_t *sequence` and an unsigned cursor `size_t index`.
   - Implement factory constructor `object_t *new_cycle_iter(vm_t *vm, object_t *sequence)` that validates sequence compatibility and increments the sequence's reference count.
   - Implement iteration step accessor `object_t *cycle_iter_next(object_t *iter)` that yields elements sequentially, wraps around using modular arithmetic (`index % length`), and returns `NULL` on empty sequences.
   - Implement cursor control `bool cycle_iter_reset(object_t *iter)` allowing callers to rewind the iteration cursor to index `0`.
   - Integrate `OBJ_CYCLE_ITER` into mark-and-sweep cycle collection (`trace_blacken_object`) and teardown (`object_free_payload`).
   - Support self-referential and mutually cyclic structures (e.g. `list[0] = cycle_iter`), verifying automatic detection and reclamation by the mark-and-sweep cycle collector.
1. **Scope Boundaries**:
   - General Python generator functions (`yield` expressions) and state-machine coroutines are deferred to generator runtime milestones.
   - User-defined iterable class protocols (`__iter__` and `__next__` method dispatch) are deferred to object-oriented method lookup milestones.
   - Bounded or step-limited iteration wrappers for infinite streams are deferred to higher-level tooling and REPL evaluation.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Payload definition embedded directly inside `object_data_t`:
     ```c
     typedef struct {
       object_t *sequence; /* Owning pointer to target sequence (List or Tuple) */
       size_t index;       /* Current cursor position */
     } cycle_iter_t;
     ```
   - Memory layout of `object_t` with `OBJ_CYCLE_ITER`:
     ```
     +-------------------------------------------------------+
     |                       object_t                        |
     +-----------------+-------------------+-----------------+
     |  is_marked: 1   |   refcount: N     |  tracker_id: ID |
     +-----------------+-------------------+-----------------+
     |  kind: OBJ_CYCLE_ITER                                 |
     +-------------------------------------------------------+
     |  data.v_cycle_iter (16 bytes inline)                  |
     |    *sequence  ------------------------+               |
     |    index: 0                           |               |
     +---------------------------------------+---------------+
                                             |
                                             v
                              +-----------------------------+
                              |    Target Sequence Object   |
                              |  (kind: OBJ_LIST/OBJ_TUPLE) |
                              +-----------------------------+
     ```
   - Mutual reference cycle topology:
     ```
     +---------------------+           +---------------------+
     |   List Object       |  owns [0] |   Cycle Iterator    |
     |  refcount: 1        | --------> |  refcount: 1        |
     |  elements[0]        | <-------- |  sequence           |
     +---------------------+ owns seq  +---------------------+
     ```
1. **Core Systems Invariants**:
   - **Ownership Balance**: Creating a cycle iterator increments the sequence refcount (`refcount_inc(sequence)`). Freeing the iterator payload releases that reference (`refcount_dec(sequence)`).
   - **Zero Secondary Allocations**: Because `sizeof(cycle_iter_t)` is 16 bytes (identical to `sizeof(list_t)`), it resides by value inside the tagged union `object_data_t`, requiring zero auxiliary heap blocks beyond the `object_t` container itself.
   - **Dynamic Mutation Resilience**: `cycle_iter_next()` dynamically queries `object_len(sequence)` on each step. If a mutable list shrinks or grows, modular arithmetic (`index % len`) guards against buffer overruns and segmentation faults.
   - **Empty Container Safety**: If `len <= 0`, `cycle_iter_next()` returns `NULL` without mutating `index` and without triggering division-by-zero faults.
   - **Strict Type Validation**: Passing non-sequence objects (Integers, Booleans, None, NULL) to `new_cycle_iter()` must be defensively rejected, returning `NULL`.
   - **GC Tracing Completeness**: `trace_blacken_object()` must visit `iter->data.v_cycle_iter.sequence` during mark passes to prevent premature collection of the target container.
1. **Architectural Trade-offs**:
   - Live sequence reference vs snapshot buffer: Capturing a reference to the live container reflects dynamic mutations immediately and requires zero allocation overhead. In contrast, copying elements into an internal buffer would snapshot state but incur $O(N)$ allocation churn and duplicate memory.
   - Inline tagged union storage vs pointer indirection: Storing `cycle_iter_t` inline in `object_data_t` exploits existing union padding, avoids extra `malloc`/`free` calls, and maximizes CPU cache locality during iteration steps.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   - The Iterator Pattern (Gang of Four) in low-level runtime implementations.
   - Circular traversal via modular arithmetic: maintaining an infinite index sequence ($i \\leftarrow (i + 1) \\pmod N$) across finite contiguous memory.
   - Cyclic reference topologies: why immediate reference counting fails on mutual circular dependencies (`container[0] = iterator`) and why mark-and-sweep graph traversal is necessary for reclamation.
1. **Socratic Inquiries**:
   - If a list contains a cycle iterator that points back to that same list, what happens to their reference counts when the stack frame is popped? Why can't reference counting alone free them?
   - In C, what happens if you evaluate `index % len` when `len == 0`? How must division-by-zero traps be guarded in iterator step implementations?
   - If code appends an element to a list while a cycle iterator is active, what should `cycle_iter_next()` do on subsequent iterations?
1. **Failure Modes & Pitfalls**:
   - Division-by-zero CPU trap on empty sequences (`index % 0`).
   - Omitting `refcount_inc(sequence)` in `new_cycle_iter()`, causing a dangling pointer if the caller drops their handle to the sequence.
   - Memory leak if `object_free_payload()` fails to call `refcount_dec(sequence)`.
   - Infinite loops if calling code expects every iterator to eventually return a terminating sentinel.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Add `OBJ_CYCLE_ITER` to `object_kind_t` in `src/object.h`.
   - Add `cycle_iter_t` struct definition and `cycle_iter_t v_cycle_iter;` field to `object_data_t` in `src/object.h`.
   - Declare `object_t *new_cycle_iter(vm_t *vm, object_t *sequence);` in `src/new.h`.
   - Declare `object_t *cycle_iter_next(object_t *iter);` and `bool cycle_iter_reset(object_t *iter);` in `src/object.h`.
   - Implement `new_cycle_iter()` in `src/new.c`, validating sequence types (`OBJ_LIST`, `OBJ_TUPLE`), incrementing sequence refcount, and registering with `vm_track_object()`.
   - Implement `cycle_iter_next()` and `cycle_iter_reset()` in `src/object.c`.
   - Update `object_free_payload()` in `src/object.c` to call `refcount_dec(obj->data.v_cycle_iter.sequence)`.
   - Update `trace_blacken_object()` in `src/gc.c` to mark `obj->data.v_cycle_iter.sequence`.
   - Add comprehensive unit and adversarial tests in `tests/test_object.c` and `tests/test_gc.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/new.h`, `src/new.c`
   - `src/gc.c`
   - `tests/test_object.c`, `tests/test_gc.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**:
   - Sequential cycling validation: verify cycling through a 3-element list `[10, 20, 30]` produces `10, 20, 30, 10, 20, 30, 10` across 7 consecutive `cycle_iter_next()` calls.
   - Tuple compatibility: verify identical cyclical behavior over variable-length tuples (`tuple_t`).
   - Empty sequence handling: verify `cycle_iter_next()` on an empty list `[]` or empty tuple `()` safely returns `NULL`.
   - Non-sequence rejection: verify `new_cycle_iter()` returns `NULL` when passed an Integer, Float, Boolean, None, or `NULL`.
   - Dynamic sequence mutation: verify that appending an element to a list during iteration adjusts the wrap-around modulus dynamically.
   - Cursor rewind: test `cycle_iter_reset()` correctly resets the iteration cursor to index `0`.
   - Adversarial cycle test: construct a mutual reference cycle (`list_set(lst, 0, citer)`), verify reference counts remain positive when stack roots are cleared, run `vm_collect_garbage()`, and assert complete reclamation.
1. **Zero-Leak Guarantee**:
   - All test suites must verify `assert(boot_all_freed())`, ensuring both linear iterations and circular reference meshes are completely freed.
1. **Tooling Quality Gates**:
   - `just test` (100% pass rate under AddressSanitizer and UndefinedBehaviorSanitizer).
   - `just lint` (`clang-tidy`, Doxygen docstrings, and `just lint-roadmap`).
   - `just check` (all pre-commit hooks clean).
1. **Milestone Completion & Lesson Extraction**:
   - Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson file in `lessons/` following the `lesson-extraction` skill.
