# Milestone: Variadic Object Packing Constructors

**ID:** `5b538ed`\
**Status:** Planned\
**Difficulty:** 1 / 5\
**Focus:** Introduce variadic constructors (`new_tuple_pack`, `new_list_pack`) using `<stdarg.h>`, replacing hardcoded fixed-arity constructors (`new_tuple_1`, `new_tuple_2`, `new_tuple_3`) and mastering variadic unpacking and cleanup safety.\
**Prerequisites:** [Heap-Allocated Variable-Length Tuple](f9c475f_heap_allocated_variable_length_tuple.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Implement `object_t *new_tuple_pack(size_t count, ...);` accepting an arbitrary number of `object_t*` arguments.
   - Implement `object_t *new_list_pack(size_t count, ...);` packing an arbitrary number of elements into a resizable list.
   - Deprecate or refactor hardcoded `new_tuple_1()`, `new_tuple_2()`, and `new_tuple_3()` as light inline wrappers around `new_tuple_pack()`.
1. **Scope Boundaries**:
   - Keyword argument packing (dictionaries) is deferred to Milestone `5895af9_hash_maps_and_dictionaries.md`.
   - Formatted string packing is handled in Milestone `4911b8b_dynamic_string_builder.md`.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Variadic argument stack traversal:
     ```
     Caller Frame:
       [count = 3]  [ptr_arg1]  [ptr_arg2]  [ptr_arg3]
           |             |           |           |
           |             \-----------+-----------/
           v                         v
     new_tuple_pack:             va_arg(args, object_t*) -> element slots
     ```
1. **Core Systems Invariants**:
   - Argument type safety: Every variadic argument passed up to `count` must be an `object_t*`.
   - Cleanup invariant on allocation failure: If tuple allocation or element tracking fails midway through unpacking, already unpacked elements must have their reference counts decremented to avoid leaks before returning `NULL`.
   - `va_end` pairing: Every `va_start` invocation must be strictly paired with a corresponding `va_end` before the function returns on all control flow branches.
1. **Architectural Trade-offs**: Variadic functions in C sacrifice compile-time argument type checking in exchange for flexible, ergonomic constructors. Strict count parameters and defensive NULL assertions mitigate runtime type mismatches.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: The `<stdarg.h>` header; calling conventions and ABI argument passing (stack registers vs frame stack); default argument promotions in variadic calls; undefined behavior when `va_arg` type does not match caller argument type.
1. **Socratic Inquiries**:
   - Why can the compiler not verify whether the number of arguments passed matches the `count` parameter? What happens in memory if `count` is greater than the actual arguments supplied?
   - Why must `va_end` always be called before returning, even on error return paths?
   - How does Python's `(*args)` unpacking compare to C's `va_list`?
1. **Failure Modes & Pitfalls**: Passing an incorrect argument count leading to stack reads of garbage memory; omitting `va_end` on early error exits; failing to clean up reference counts if an allocation fails mid-construction.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Include `<stdarg.h>` in `src/new.h` and `src/new.c`.
   - Declare `object_t *new_tuple_pack(size_t count, ...);` and `object_t *new_list_pack(size_t count, ...);` in `src/new.h`.
   - Implement `new_tuple_pack` in `src/new.c` using `va_start`, looping `count` times with `va_arg(args, object_t*)`, and concluding with `va_end`.
   - Implement `new_list_pack` in `src/new.c` similarly.
   - Refactor `new_tuple_1`, `new_tuple_2`, and `new_tuple_3` to delegate to `new_tuple_pack`.
   - Add unit tests in `tests/test_tuple.c` testing packs of size 0, 1, 5, and 10.
1. **File Touchpoints**:
   - `src/new.h`, `src/new.c`
   - `tests/test_tuple.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Test packing 0 elements (yielding empty tuple singleton), single element, and multiple elements; verify proper reference count increments; test mid-loop allocation failure simulation.
1. **Zero-Leak Guarantee**: Packed tuples and lists cleanly freed with `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______________________________________________________________________

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [ISO C stdarg.h Variadic Function Rules](https://en.cppreference.com/w/c/variadic): Mechanisms of `va_list`, `va_start`, `va_arg`, and `va_end` across standard architectures.
   - [SEI CERT C MSC39-C: Correct Use of va_arg](https://wiki.sei.cmu.edu/confluence/display/c/MSC39-C.+Do+not+call+va_arg%28%29+on+an+uninitialized+va_list): Preventing undefined behavior when consuming variadic argument streams.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython Python/modsupport.c Argument Building](https://github.com/python/cpython/blob/main/Python/modsupport.c): How `Py_BuildValue` parses format strings and variadic parameters to instantiate tuples and lists.
   - [System V AMD64 ABI Calling Conventions](https://en.wikipedia.org/wiki/X86_calling_conventions#System_V_AMD64_ABI): How registers and stack frames cooperate during variadic function calls at the machine code level.
