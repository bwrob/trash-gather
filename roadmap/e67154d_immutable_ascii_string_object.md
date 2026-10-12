# Milestone: Immutable ASCII String Object

**ID:** `e67154d`\
**Status:** Planned\
**Difficulty:** 1 / 5\
**Focus:** Implement an immutable, length-prefixed ASCII string object (`string_t`) with a C99 flexible array member, mastering safe string allocation, $O(1)$ length queries, and C standard library interoperability.\
**Prerequisites:** [Hybrid Reference Counting & Cycle Collection Runtime](393f420_hybrid_gc_runtime.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   1. Introduce `KIND_STRING` and the `string_t` structure featuring a C99 flexible array member `char data[]` storing null-terminated ASCII characters.
   2. Implement `object_t *new_string(const char *str)` allocating the string structure and its text payload in a single contiguous memory block: `sizeof(string_t) + length + 1`.
   3. Implement accessors `size_t string_len(object_t *obj)` returning precomputed length in $O(1)$ and `const char *string_cstr(object_t *obj)` returning a safe, null-terminated C string pointer.
   4. Integrate `KIND_STRING` into the sequence protocol (`object_len`) and the garbage collector deallocation path (`object_free_payload`).
   5. Implement string value equality (`string_equal`) with fast length-mismatch short-circuiting.
2. **Scope Boundaries**:
   1. Dynamic string mutation, appending, and buffer growth are deferred to Milestone `4911b8b` (Dynamic String Builder).
   2. Multi-byte UTF-8 decoding, codepoint indexing, and Unicode normalization are deferred to future milestones.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:

   ```text
   object_t (Header)
   ┌───────────────────────────┐
   │ kind = KIND_STRING        │
   │ refcount = 1              │
   │ is_marked = false         │
   │ value.string ─────────────┼─► string_t (Contiguous Allocation)
   └───────────────────────────┘   ┌─────────────────────────────────────────┐
                                   │ size_t length = 5                       │
                                   │ char data[] (Flexible Array Member)     │
                                   │ ['H', 'e', 'l', 'l', 'o', '\0']         │
                                   └─────────────────────────────────────────┘
   ```

   `string_t` definition:

   ```c
   typedef struct String {
       size_t length;
       char data[]; // C99 flexible array member
   } string_t;
   ```

2. **Core Systems Invariants**:
   1. **Single Contiguous Allocation Invariant**: `string_t` and its character array are allocated together via `malloc(sizeof(string_t) + length + 1)`, guaranteeing maximum cache locality and single-pointer cleanup.
   2. **Null-Termination Invariant**: The byte at offset `data[length]` must always be `\0`. This ensures `string_cstr()` can be passed directly to `printf`, `strcmp`, and C standard library routines with zero allocation or conversion overhead.
   3. **Immutability Invariant**: Following construction, string payload characters must never be modified.
   4. **$O(1)$ Length Invariant**: The string length is cached upon construction in `length`, ensuring `object_len()` executes in $O(1)$ without scanning the buffer via `strlen`.
3. **Architectural Trade-offs**:
   1. Storing both an explicit length and a trailing null terminator costs 1 additional byte per string, but provides the $O(1)$ speed of Pascal strings alongside complete compatibility with standard C string APIs.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   1. Pascal strings (length-prefixed) vs C strings (null-terminated).
   2. C99 flexible array members (FAM) and contiguous memory allocation.
   3. Cache locality and single vs two-pointer allocation patterns.
   4. Fast-path string comparison via length mismatch checking.
2. **Socratic Inquiries**:
   1. What is the algorithmic complexity difference between calling `object_len()` on a length-prefixed string versus calling `strlen()` on a plain C string?
   2. Why do production runtimes like CPython (`PyASCIIObject`) and Redis (`sds`) retain the trailing `\0` byte on length-prefixed strings? What would occur if a string without a null terminator were passed to `printf("%s", ...)`?
   3. Why is `memcpy` preferred over `strcpy` when initializing a string object whose length has already been computed?
   4. If two strings have different `length` values, why can string equality comparison immediately return `false` without inspecting any characters?
3. **Failure Modes & Pitfalls**:
   1. Forgetting the `+ 1` null-terminator in allocation sizing, causing buffer overruns when strings are read by standard C routines.
   2. Calling `strlen()` inside hot loops rather than reading `str->length`.
   3. Dereferencing NULL when passed an uninitialized string pointer.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. Add `KIND_STRING` to `object_kind_t` in `src/object.h`.
   2. Define `string_t` with `char data[]` in `src/object.h`.
   3. Declare `new_string(const char *str)` in `src/new.h`.
   4. Declare `string_cstr(object_t *obj)`, `string_len(object_t *obj)`, and `string_equal(object_t *a, object_t *b)` in `src/object.h`.
   5. Implement `new_string` in `src/new.c`.
   6. Implement accessors and equality in `src/object.c`.
   7. Integrate `KIND_STRING` into `object_len` in `src/object.c` and `object_free_payload` in `src/vm.c`.
   8. Write comprehensive unit tests in `tests/test_new.c` and `tests/test_object.c`.
2. **File Touchpoints**:
   1. `src/object.h`
   2. `src/new.h`
   3. `src/new.c`
   4. `src/object.c`
   5. `src/vm.c`
   6. `tests/test_new.c`
   7. `tests/test_object.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**:
   1. Constructor verification: Allocate empty string `""`, single-char `"a"`, and multi-word strings, asserting correct length and null-termination.
   2. Polymorphic length: Verify `object_len(new_string("hello")) == 5`.
   3. Equality testing: Confirm identical strings compare equal, differing strings compare false, and length mismatches exit early without comparing contents.
   4. C-string interoperability: Pass `string_cstr()` directly to `strcmp` and standard library formatting.
   5. Garbage collection cleanup: Allocate 1,000 strings, trigger `vm_collect_garbage()`, and assert all memory is reclaimed without leaks.
2. **Zero-Leak Guarantee**: String headers and payload buffers are confirmed 100% freed via `assert(boot_all_freed())`.
3. **Tooling Quality Gates**: `just test` (100% pass under ASan/UBSan), `just lint`, and `just check` pass cleanly.
4. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update writeup and roadmap status, and generate educational lesson in `lessons/` following the `lesson-extraction` skill.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [C99 Flexible Array Members (FAM)](https://en.cppreference.com/w/c/language/struct): Standard specification and allocation patterns for flexible array members in C.
   - [CPython Compact ASCII Strings (PEP 393)](https://peps.python.org/pep-0393/): Design of contiguous, length-prefixed ASCII string objects in CPython.
2. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython Objects/unicodeobject.c](https://github.com/python/cpython/blob/main/Objects/unicodeobject.c): Production C implementation of Python's string layout and length caching.
   - [Redis Simple Dynamic Strings (SDS)](https://github.com/redis/redis/blob/unstable/src/sds.c): Renowned systems string library combining explicit length prefixing with null-termination.
