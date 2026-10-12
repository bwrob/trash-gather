# Milestone: Sequence Membership & Linear Search

**ID:** `13e2377`\
**Status:** Planned\
**Difficulty:** 1 / 5\
**Focus:** Implement polymorphic sequence membership testing (`seq_contains`), element index lookup (`seq_index`), and frequency counting (`seq_count`) across lists and tuples, mastering pointer identity fast-paths and value comparison.\
**Prerequisites:** [Polymorphic Sequence Length Protocol](b81f9a7_polymorphic_sequence_length.md), [Python-Style Sequence Negative Indexing](b0c1d8b_python_sequence_negative_indexing.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   1. Implement `bool seq_contains(object_t *seq, object_t *target)` determining whether an element exists within a tuple or list, directly mirroring Python's `target in seq` operator.
   2. Implement `int64_t seq_index(object_t *seq, object_t *target)` returning the 0-based index of the first occurrence of `target`, or returning `-1` if not found.
   3. Implement `size_t seq_count(object_t *seq, object_t *target)` returning the total number of occurrences of `target` in the sequence.
   4. Establish an identity-first comparison fast path: checking instant pointer equality (`elem == target`) before invoking deep value equality (`object_equal`).
2. **Scope Boundaries**:
   1. Substring searching within string buffers is deferred to string-specific milestones.
   2. Associative hash table key lookups (`dict_contains`) are deferred to Milestone `5895af9`.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:

   ```text
   Sequence (list_t or tuple_t)
   ┌──────────┬──────────┬──────────┬──────────┐
   │ data[0]  │ data[1]  │ data[2]  │ data[3]  │
   └────┬─────┴────┬─────┴────┬─────┴────┬─────┘
        │          │          │          │
        ▼          ▼          ▼          ▼
     object_t   object_t   object_t   object_t
     (ptr !=)   (ptr !=)   (ptr ==)   (unvisited)
        │          │          │
        ▼          ▼          │
     compare    compare       └─► Early exit: Return true / index 2
     (false)    (false)
   ```

2. **Core Systems Invariants**:
   1. **Identity-First Fast Path Invariant**: For every element visited, if `data[i] == target` (identical pointer addresses), match is confirmed immediately without calling value equality functions.
   2. **Non-Mutating Inspection Invariant**: Searching is strictly read-only. Pointer traversal must never increment, decrement, or modify reference counts, flags, or data slots of either the sequence or the target.
   3. **Safe Sentinel Return Invariant**: When an item is not found, `seq_index` returns `-1` without raising memory traps or reading beyond sequence bounds.
   4. **Bounds Invariant**: Traversal strictly terminates at index `count - 1` (or `length - 1`), guarding against reads into uninitialized capacity slots.
3. **Architectural Trade-offs**:
   1. Linear $O(N)$ scanning incurs zero auxiliary memory overhead and optimal CPU cache locality across contiguous pointer arrays, making it faster than hashing for small sequence sizes.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   1. Linear search complexity and cache prefetching across contiguous pointer arrays.
   2. Pointer identity (`is` in Python) vs value equality (`==` in Python).
   3. Sentinel values in C APIs (e.g. `-1` vs out-of-band status codes).
2. **Socratic Inquiries**:
   1. Why should `seq_contains` test `elem == target` before evaluating object values? How does this dramatically accelerate checks on immortal singletons like `None` or cached integers?
   2. In Python, what is the exact semantic difference between `x is y` and `x == y`? How does `element_matches` reflect both semantics in C?
   3. If a list contains a reference to itself, why does an identity check prevent potential infinite recursion during search comparisons?
   4. Why is `int64_t` chosen as the return type for `seq_index` rather than `size_t`?
3. **Failure Modes & Pitfalls**:
   1. Iterating past the logical length into uninitialized capacity in dynamic lists (`count` vs `capacity`).
   2. Segmentation faults caused by dereferencing NULL sequence or target pointers.
   3. Accidental reference count leaks when comparing target objects.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. Declare `seq_contains`, `seq_index`, and `seq_count` in `src/object.h`.
   2. Implement static comparison helper `static bool objects_match(object_t *a, object_t *b)` in `src/object.c`.
   3. Implement `seq_contains(object_t *seq, object_t *target)` supporting both `KIND_TUPLE` and `KIND_LIST`.
   4. Implement `seq_index(object_t *seq, object_t *target)` returning the matching index or `-1`.
   5. Implement `seq_count(object_t *seq, object_t *target)`.
   6. Write comprehensive adversarial unit tests in `tests/test_object.c`.
2. **File Touchpoints**:
   1. `src/object.h`
   2. `src/object.c`
   3. `tests/test_object.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**:
   1. Membership verification: Find elements located at head, middle, and tail of both lists and tuples.
   2. Absence verification: Confirm `seq_contains` returns false, `seq_index` returns `-1`, and `seq_count` returns 0 for non-existent items.
   3. Multiplicity verification: Confirm `seq_count` accurately counts duplicate elements, and `seq_index` returns the earliest occurrence index.
   4. Pointer identity verification: Confirm fast-path matching for immortal singletons (`vm_get_none()`).
   5. Empty container edge case: Test empty list and empty tuple returning false / `-1` / 0 immediately.
2. **Zero-Leak Guarantee**: Searching allocates zero heap memory and performs zero refcount mutations; verified via `assert(boot_all_freed())`.
3. **Tooling Quality Gates**: `just test` (100% pass under ASan/UBSan), `just lint`, and `just check` pass cleanly.
4. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update writeup and roadmap status, and generate educational lesson in `lessons/` following the `lesson-extraction` skill.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [CPython Sequence Methods Protocol (sq_contains)](https://docs.python.org/3/c-api/typeobj.html#c.PySequenceMethods.sq_contains): Official specification for `PySequenceMethods.sq_contains` powering Python's `in` operator.
   - [Linear Search and Short-Circuit Evaluation](https://en.wikipedia.org/wiki/Linear_search): Foundational algorithms for sequential search and loop termination.
2. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython list_contains in listobject.c](https://github.com/python/cpython/blob/main/Objects/listobject.c): Production C implementation of Python's `list.__contains__`.
   - [Python Comparisons and Identity Data Model](https://docs.python.org/3/reference/datamodel.html#comparisons): Language specification distinguishing identity tests from value equality.
