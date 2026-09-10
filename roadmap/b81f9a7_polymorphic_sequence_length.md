# Milestone: Polymorphic Sequence Length Protocol

**ID:** `b81f9a7`\
**Status:** Planned\
**Focus:** Implement a polymorphic sequence length protocol (`object_len`) unifying length queries across Strings, Lists, and Tuples with $O(1)$ complexity.\
**Prerequisites:** [Heap-Allocated Variable-Length Tuple](f9c475f_heap_allocated_variable_length_tuple.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Implement a polymorphic sequence query function: `int64_t object_len(const object_t *obj)`.
   - Dispatch length resolution uniformly across all runtime sequence types: Strings (`OBJ_STRING`), Lists (`OBJ_LIST`), and Tuples (`OBJ_TUPLE`).
   - Safely return an error sentinel (`-1`) when queried on non-sequence objects (e.g. Integers, Booleans, None, and `NULL` pointers).
   - Establish the sequence foundation required by downstream milestones, including negative indexing (`b0c1d8b`) and container truthiness (`6c3a989`).
1. **Scope Boundaries**:
   - Dynamic user-defined `__len__()` method dispatch on user-defined classes is deferred to object-oriented method lookup milestones.
   - Sequence slicing and index mutation operations are deferred to negative indexing and dynamic slices milestones.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Polymorphic dispatch inspects the discriminator `kind` and accesses container length fields directly without indirection:
     ```
     object_len(const object_t *obj)
             |
             +---> OBJ_STRING -> obj->as.string.length
             |
             +---> OBJ_LIST   -> obj->as.list.size
             |
             +---> OBJ_TUPLE  -> obj->as.tuple->count  (or obj->as.tuple.count)
             |
             +---> Otherwise  -> -1 (type error / non-sequence)
     ```
1. **Core Systems Invariants**:
   - **$O(1)$ Constant-Time Retrieval**: Sequence length must be read directly from pre-computed header fields; strings must never traverse characters with `strlen()`.
   - **NULL Safety**: Passing `NULL` must return `-1` safely without dereferencing invalid memory.
   - **Type Safety**: Non-sequence objects must return `-1` rather than `0`, maintaining a strict distinction between an empty sequence and an object that lacks length semantics.
   - **Const Preservation**: `object_len()` must accept `const object_t *obj` and guarantee no mutation of headers, payloads, or reference counts.
1. **Architectural Trade-offs**:
   - Tagged union switch dispatch vs vtable indirection: Direct switch dispatch keeps memory overhead to zero bytes per object, optimizes branch prediction for a closed set of primitive types, and avoids function pointer indirection cache misses.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   - The Sequence Protocol in dynamically typed runtime architectures (CPython `PySequence_Size` / `sq_length`).
   - Length-prefixed vs null-terminated data structures (Pascal strings vs C strings): Why storing explicit length enables embedded null bytes (`\0`) and guarantees $O(1)$ length operations.
   - Polymorphism via tagged union discriminators in pure C.
1. **Socratic Inquiries**:
   - In Python, why does `len(42)` raise a `TypeError` instead of returning `0` or `1`?
   - If a string object contains binary data with embedded null bytes, why would `strlen()` return an incorrect truncated length while `string->length` remains exact?
   - How does a polymorphic `object_len()` function simplify the implementation of negative sequence indexing (`b0c1d8b`)?
1. **Failure Modes & Pitfalls**:
   - Dereferencing `obj->kind` before verifying `obj != NULL`, causing segmentation faults.
   - Returning `0` on invalid types, causing non-sequences to falsely appear as empty containers to callers.
   - Signed integer overflow if casting an unsigned container size (`size_t`) to `int64_t` without upper-bound validation.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Declare `int64_t object_len(const object_t *obj);` in `src/object.h`.
   - Implement `object_len()` in `src/object.c` using discriminator matching across `OBJ_STRING`, `OBJ_LIST`, and `OBJ_TUPLE`.
   - Add defensive NULL and non-sequence error checks returning `-1`.
   - Add unit tests in `tests/test_object.c` covering all sequence and non-sequence types.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `tests/test_object.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**:
   - Empty container validation: verify `object_len()` returns `0` for empty string `""`, empty list `[]`, and empty tuple `()`.
   - Populated sequence validation: verify correct length on multi-element lists, variable-length tuples, and strings.
   - Binary string validation: verify length on strings containing embedded null characters (`"foo\0bar"`).
   - Non-sequence rejection: verify `object_len()` returns `-1` for Integer, None, Boolean, and `NULL`.
1. **Zero-Leak Guarantee**:
   - All tests must pass with `assert(boot_all_freed())`.
1. **Tooling Quality Gates**:
   - `just test` (100% pass rate under AddressSanitizer and UndefinedBehaviorSanitizer).
   - `just lint` (`clang-tidy`, Doxygen docstring checks, and `just lint-roadmap`).
   - `just check` (all pre-commit git hooks clean).
1. **Milestone Completion & Lesson Extraction**:
   - Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson file in `lessons/` following the `lesson-extraction` skill.
