# Milestone: Sequence Reversal & Shallow Copy

**ID:** `37c67d7`\
**Status:** Planned\
**Difficulty:** 1 / 5\
**Focus:** Implement in-place sequence reversal (`list_reverse`) and shallow container cloning (`tuple_copy`, `list_copy`), mastering two-pointer in-place pointer swapping and reference count propagation.\
**Prerequisites:** [Polymorphic Sequence Length Protocol](b81f9a7_polymorphic_sequence_length.md), [Dynamic Resizable List Mutations](d7b5feb_dynamic_resizable_list.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   1. Implement `bool list_reverse(object_t *list)` reversing the order of elements in a mutable list in-place using a two-pointer algorithm with zero additional heap allocations.
   2. Implement `object_t *list_copy(object_t *list)` creating a new shallow copy of a list, allocating a new item buffer and incrementing the reference count (`object_incref`) of each contained element.
   3. Implement `object_t *tuple_copy(object_t *tuple)` supporting shallow copying for immutable tuples.
   4. Implement CPython's immutable tuple optimization: because tuples are strictly immutable, `tuple_copy(t)` increments the tuple's refcount and returns the existing tuple pointer rather than duplicating memory.
2. **Scope Boundaries**:
   1. Deep copying (recursive duplication of nested objects) is deferred to Milestone `dac4aea`.
   2. In-place sorting (`list_sort`) is deferred to Milestone `3f1132d`.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:

   In-place two-pointer reversal:

   ```text
   Initial:    [ A ]   [ B ]   [ C ]   [ D ]
                 ▲                       ▲
               left                    right
                 │       swap(A, D)      │
                 └───────────────────────┘

   Next step:  [ D ]   [ B ]   [ C ]   [ A ]
                         ▲       ▲
                       left    right
                         │ swap(B, C)│
                         └───────────┘

   Finished:   [ D ]   [ C ]   [ B ]   [ A ]  (left >= right, stop)
   ```

   Shallow copy reference graph:

   ```text
   Original List               Copied List
   ┌───────────┐               ┌───────────┐
   │ count = 3 │               │ count = 3 │
   │ data      │               │ data      │
   └─────┬─────┘               └─────┬─────┘
         │                           │
         ▼                           ▼
      [0] ───► object_t A (refcount = 2) ◄─── [0]
      [1] ───► object_t B (refcount = 2) ◄─── [1]
      [2] ───► object_t C (refcount = 2) ◄─── [2]
   ```

2. **Core Systems Invariants**:
   1. **Refcount Invariance on Reversal**: In-place reversal rearranges existing pointers within the list's storage array without altering the set of referenced objects. No element reference counts are modified.
   2. **Reference Propagation on Copy**: Every element pointer written into a newly allocated list copy must have its refcount incremented via `object_incref`.
   3. **Rollback on Allocation Failure**: If allocation fails during `list_copy`, any partially copied element references must be rolled back via `object_decref` before releasing memory and returning NULL.
   4. **Immutable Identity Invariant**: Calling `tuple_copy(tuple)` must return the exact same pointer `tuple` with `object_incref(tuple)` applied, preserving memory efficiency.
3. **Architectural Trade-offs**:
   1. In-place reversal operates in $O(1)$ auxiliary space and $O(N)$ time, but cannot be applied to immutable tuples. Reversing a tuple requires allocating a new tuple.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   1. The classic two-pointer algorithm and symmetry in array reversal.
   2. Shallow copy vs deep copy in garbage-collected and reference-counted runtimes.
   3. Immutable object reuse optimizations in production virtual machines.
2. **Socratic Inquiries**:
   1. Why does `list_reverse` require zero calls to `object_incref` or `object_decref`?
   2. In Python, why does `tuple(t)` or `copy.copy(t)` on a tuple return `t` itself, whereas `list(l)` always allocates a fresh list?
   3. If `list_copy` allocates a new list of 5 elements, successfully copies 3, and then encounters an out-of-memory condition, what steps must occur to avoid memory leaks?
   4. How many pointer swaps are executed when reversing a list of length $N$? What happens when $N$ is odd versus even?
3. **Failure Modes & Pitfalls**:
   1. Off-by-one errors causing the middle element to be mishandled or skipped.
   2. Forgetting to `object_incref` copied elements, resulting in a double-free or use-after-free when either list is destroyed.
   3. Dereferencing NULL pointers when passed uninitialized sequences.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. Declare `list_reverse`, `list_copy`, and `tuple_copy` in `src/object.h`.
   2. Implement `list_reverse(object_t *list)` in `src/object.c` using symmetric pointer swapping.
   3. Implement `list_copy(object_t *list)` in `src/object.c` with allocation failure rollback.
   4. Implement `tuple_copy(object_t *tuple)` in `src/object.c` implementing the immutable pointer reuse fast path.
   5. Author comprehensive unit tests in `tests/test_object.c`.
2. **File Touchpoints**:
   1. `src/object.h`
   2. `src/object.c`
   3. `tests/test_object.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**:
   1. In-place reversal testing with odd-length (5 items), even-length (4 items), single-element (1 item), and empty (0 items) lists.
   2. Double-reversal invariant: verify `reverse(reverse(list))` restores exact original element order.
   3. Independent mutation test: mutating a copied list (`list_set`) does not affect the original list, but contained items are shared correctly.
   4. Destruction independence: destroy the original list and verify all elements in the copy remain fully valid and accessible without memory faults.
   5. Immutable tuple copy test: verify `tuple_copy(t) == t` and refcount is incremented by 1.
2. **Zero-Leak Guarantee**: All allocated copy buffers and element refcounts are verified clean via `assert(boot_all_freed())`.
3. **Tooling Quality Gates**: `just test` (100% pass under ASan/UBSan), `just lint`, and `just check` pass cleanly.
4. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update writeup and roadmap status, and generate educational lesson in `lessons/` following the `lesson-extraction` skill.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Two-Pointer Algorithmic Technique](https://en.wikipedia.org/wiki/Two-pointer_technique): Foundational algorithm for in-place array manipulation.
   - [Python Shallow vs Deep Copy Semantics](https://docs.python.org/3/library/copy.html): Python documentation detailing shallow reference sharing.
2. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython list_reverse in listobject.c](https://github.com/python/cpython/blob/main/Objects/listobject.c): The exact C implementation of `list.reverse()` in CPython.
   - [CPython Tuple Copy Optimization](https://github.com/python/cpython/blob/main/Objects/tupleobject.c): CPython's optimization returning identical instances for immutable tuple copying.
