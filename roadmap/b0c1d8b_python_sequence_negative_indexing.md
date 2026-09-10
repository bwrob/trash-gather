# Milestone: Python-Style Sequence Negative Indexing

**ID:** `b0c1d8b`\
**Status:** Planned\
**Focus:** Support signed integer offsets (`int64_t`) across list and tuple accessors, enabling Python-style negative indexing with robust bounds validation.\
**Prerequisites:** [Polymorphic Sequence Length Protocol](b81f9a7_polymorphic_sequence_length.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Update `list_get()`, `list_set()`, and `tuple_get()` to accept signed integer indices (`int64_t`), allowing negative indices where `-1` maps to the last element, `-2` to the second-to-last, etc.
1. **Scope Boundaries**: Full slice syntax (`seq[start:stop:step]`) is deferred to Milestone 11 (Dynamic Slices).

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Index translation logic:
     ```
     For a sequence of length N:
       Index:   0    1    2   ...   N-1
       Negative: -N -(N-1) ...   -2    -1
     ```
   - Normalization formula:
     $$\\text{normalized} = \\begin{cases} \\text{idx} & \\text{if } \\text{idx} \\ge 0 \\ \\text{size} + \\text{idx} & \\text{if } \\text{idx} < 0 \\end{cases}$$
1. **Core Systems Invariants**:
   - Bounds invariant: An access is valid if and only if $0 \\le \\text{normalized} < \\text{size}$.
   - Underflow rejection: Any negative index where $\\text{idx} < -\\text{size}$ (e.g. `list[-10]` on a 5-element list) must be rejected safely without reading or writing out-of-bounds memory.
   - Const-correctness: Negative index reading does not mutate the sequence header or element pointers.
1. **Architectural Trade-offs**: Switching accessor signatures from unsigned `size_t` to signed `int64_t` simplifies user ergonomics and mirrors Python, while requiring explicit underflow checks to guard against sign-conversion bugs.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Signed vs unsigned integer representation in C (two's complement); integer overflow and underflow vulnerabilities; arithmetic promotion rules.
1. **Socratic Inquiries**:
   - In C, what happens if you compare a negative signed integer (`int64_t index = -1`) directly with an unsigned `size_t size = 5`? Why does `-1` implicitly convert to a massive positive number (`SIZE_MAX`)?
   - How can you safely convert a signed negative offset without triggering integer underflow if `index == INT64_MIN`?
   - Why do Python programmers consider `seq[-1]` an essential language idiom?
1. **Failure Modes & Pitfalls**: Implicit unsigned conversion causing wild out-of-bounds heap reads; buffer overruns on negative write operations; off-by-one errors on boundary indices `0` and `-size`.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Update function declarations in `src/object.h`:
     - `bool list_set(object_t *list, int64_t index, object_t *value);`
     - `object_t *list_get(object_t *list, int64_t index);`
     - `object_t *tuple_get(object_t *tuple, int64_t index);`
   - Implement index normalization helper `static bool normalize_index(int64_t index, size_t size, size_t *out_idx)` in `src/object.c`.
   - Update accessor implementations in `src/object.c` using the normalization helper.
   - Add comprehensive unit tests in `tests/test_object.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `tests/test_object.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify valid negative access (`seq[-1]`, `seq[-size]`), edge bounds (`seq[-size - 1]` rejected), and positive bounds (`seq[size]` rejected) across both lists and tuples.
1. **Zero-Leak Guarantee**: Negative index mutation (`list_set(list, -1, val)`) properly manages reference counts without leaks via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
