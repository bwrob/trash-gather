# Milestone: Rich Comparisons & In-Place List Sorting

**ID:** `3f1132d`\
**Status:** Planned\
**Difficulty:** 2 / 5\
**Focus:** Implement the three-way comparison protocol (`object_compare`), rich boolean comparisons (`object_equal`, `object_less_than`), and in-place list sorting (`list_sort()`) using comparator callback function pointers and standard library `qsort`.\
**Prerequisites:** [Dynamic Resizable List Mutations](d7b5feb_dynamic_resizable_list.md), [Boolean Immortal Singletons & Truthiness](6c3a989_bool_singletons_and_truthiness.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Implement three-way comparison `int object_compare(object_t *a, object_t *b, int *out_result)` returning `< 0` (less), `0` (equal), or `> 0` (greater), with error return if types are mutually incomparable.
   - Implement convenience predicates: `bool object_equal(object_t *a, object_t *b)` and `bool object_less_than(object_t *a, object_t *b)`.
   - Support lexicographical comparison for strings, tuples, and lists.
   - Implement `bool list_sort(object_t *list, int (*custom_cmp)(const void *, const void *));` sorting list elements in-place using standard library `qsort` or custom stable merge sort.
1. **Scope Boundaries**:
   - Timsort implementation is deferred to advanced optimization milestones.
   - Key-function extraction (`sort(key=...)`) is deferred to Milestone `a0c00e1_closures_and_lexical_environments.md`.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Three-way comparison dispatch:
     ```
     object_compare(a, b)
       |
       +---> Fast path: if (a == b) return 0 (pointer identity)
       +---> Type check: compare kinds or coerce (int vs float)
       +---> Dispatch comparison:
               INT vs INT:        (a->v_int > b->v_int) - (a->v_int < b->v_int)
               STRING vs STRING:  strcmp(a->v_string, b->v_string)
               TUPLE vs TUPLE:    element-by-element lexicographical recursion
     ```
1. **Core Systems Invariants**:
   - Reflexivity, symmetry, and transitivity: If `cmp(a, b) < 0`, then `cmp(b, a) > 0`. If `cmp(a, b) == 0` and `cmp(b, c) == 0`, then `cmp(a, c) == 0`.
   - Float NaN handling: Comparison with IEEE 754 NaN values must not produce undefined behavior; define total ordering where NaN is considered less or greater than numbers.
   - In-place mutation invariant: `list_sort` reorders existing `object_t*` pointers within `list->elements` without altering reference counts or leaking elements.
1. **Architectural Trade-offs**: Using C standard library `qsort` provides quick, zero-allocation sorting, but is not guaranteed to be stable (equal elements may swap order).

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Total vs partial ordering relations; lexicographical sequence comparison; the C standard library `qsort` interface and `void*` comparator callbacks; function pointer casting rules in ISO C.
1. **Socratic Inquiries**:
   - Why does `qsort` pass elements as `const void *` to the comparator function, and why do you need to cast to `object_t * const *` (a pointer to an object pointer) inside the callback?
   - How can you implement `a - b` comparison without triggering integer subtraction overflow when `a = INT_MAX` and `b = INT_MIN`?
   - How does Python's `sort()` handle sorting a list containing incomparable types (e.g. integer vs list)?
1. **Failure Modes & Pitfalls**: Integer subtraction overflow in comparators (`a - b` wrapping to positive); dereferencing the wrong pointer indirection level in `qsort` comparator; infinitely recursing on circular lists.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Declare `object_compare`, `object_equal`, and `object_less_than` in `src/object.h`.
   - Implement three-way comparison logic in `src/object.c` supporting numeric types, strings, and sequences.
   - Implement `static int default_list_comparator(const void *p1, const void *p2)` in `src/object.c`.
   - Implement `bool list_sort(object_t *list, int (*custom_cmp)(const void *, const void *));` in `src/object.c` using `qsort`.
   - Add comprehensive unit tests in `tests/test_compare.c` testing comparisons across types, edge cases (empty lists, negative numbers), and in-place list sorting.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `tests/test_compare.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify sorting an empty list, 1-element list, sorted list, reverse-sorted list, and randomized 1,000-element list; verify lexicographical string and tuple comparisons; verify integer overflow safety with `INT_MIN` / `INT_MAX`.
1. **Zero-Leak Guarantee**: Verify zero memory leaks during sorting and comparisons via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______________________________________________________________________

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [ISO C Standard Library qsort Specification](https://en.cppreference.com/w/c/algorithm/qsort): Comparator function pointer signatures and casting rules for void pointer callbacks.
   - [Python Rich Comparison Protocols](https://docs.python.org/3/reference/datamodel.html#object.__lt__): Three-way comparisons, total ordering, and handling incomparable types.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython listobject.c List Sorting Implementation](https://github.com/python/cpython/blob/main/Objects/listobject.c): How Python manages list sorting and handles callback errors during element comparisons.
   - [Timsort Algorithm Design and Invariants](https://en.wikipedia.org/wiki/Timsort): Adaptive, stable sorting exploiting natural runs in real-world data.
