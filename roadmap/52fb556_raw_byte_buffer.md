# Milestone: Raw Byte Buffer Object (`bytes_t`)

**ID:** `52fb556`\
**Status:** Planned\
**Difficulty:** 2 / 5\
**Focus:** Implement a Python `bytes`/`bytearray` inspired flat contiguous `uint8_t` byte buffer object (`bytes_t`) supporting geometric buffer growth, byte-level mutation, and hexadecimal serialization.\
**Prerequisites:** [Dynamic Resizable List Mutations](d7b5feb_dynamic_resizable_list.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Introduce `BYTES` as a first-class `object_kind_t` backed by an embedded `bytes_t` payload in `object_data_t`.
   - Store flat, contiguous arrays of raw unsigned bytes (`size_t size`, `size_t capacity`, `uint8_t *data`).
   - Implement `new_bytes(const uint8_t *initial_data, size_t size)` and `new_bytes_empty(size_t initial_capacity)`.
   - Implement byte append `bytes_append(object_t *bytes, uint8_t byte)` with geometric reallocation.
   - Implement byte indexing `bytes_get(object_t *bytes, int64_t index)` and `bytes_set(object_t *bytes, int64_t index, uint8_t val)` supporting negative indexing.
   - Implement hexadecimal conversion `bytes_to_hex(const object_t *bytes)` producing a human-readable hex string.
1. **Scope Boundaries**:
   - Non-owning slice views over byte buffers are deferred to Milestone `c44db02_dynamic_slices_and_byte_buffers.md`.
   - File stream I/O using byte buffers is handled in Milestone `402c62c_binary_heap_serialization.md`.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Contiguous byte buffer layout:
     ```
     object_t
       [kind = BYTES]
       [data.v_bytes]
          size: 4
          capacity: 8
          data: -------------> uint8_t[8]: [0xDE, 0xAD, 0xBE, 0xEF, x, x, x, x]
     ```
1. **Core Systems Invariants**:
   - Capacity bounds: $0 \\le \\text{size} \\le \\text{capacity}$ must hold after every write or append operation.
   - Reallocation safety: Reallocation must never leak the existing buffer if memory allocation fails.
   - Exclusive payload ownership: `bytes->data` is owned exclusively by the `BYTES` object and deallocated via `boot_free()` in `object_free_payload()`.
1. **Architectural Trade-offs**: Storing raw unboxed `uint8_t` bytes avoids the 40-byte overhead of wrapping each individual byte into an `INTEGER` object, achieving optimal density for binary data.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Unsigned byte representations (`uint8_t` vs `char`); endianness and raw memory dumps; amortized array reallocation; binary data vs text strings in systems runtimes.
1. **Socratic Inquiries**:
   - Why is using `uint8_t` (from `<stdint.h>`) essential when handling raw binary data in C, rather than plain `char` which can be signed or unsigned depending on the compiler?
   - How does Python distinguish between immutable `bytes` and mutable `bytearray`?
   - Why is memory density so critical when processing binary network streams or image buffers?
1. **Failure Modes & Pitfalls**: Signedness bugs when casting between `char` and `uint8_t`; integer overflow on capacity calculation; buffer overruns on negative index write operations.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Define `bytes_t` in `src/object.h`: `typedef struct { size_t size; size_t capacity; uint8_t *data; } bytes_t;`.
   - Add `BYTES` to `object_kind_t` and `bytes_t v_bytes;` to `object_data_t` in `src/object.h`.
   - Implement constructors in `src/new.c`: `new_bytes` and `new_bytes_empty`.
   - Implement `bytes_append`, `bytes_get`, `bytes_set`, and `bytes_to_hex` in `src/object.c`.
   - Hook into `object_len` and `object_free_payload` in `src/object.c`.
   - Add comprehensive unit tests in `tests/test_bytes.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/new.h`, `src/new.c`
   - `tests/test_bytes.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify appending thousands of bytes with geometric growth; test negative indexing (`bytes[-1]`); test out-of-bounds access rejection; verify correct hex formatting (`deadbeef`).
1. **Zero-Leak Guarantee**: Verify zero memory leaks via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______________________________________________________________________

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Fixed-Width Integer Types in ISO C (\<stdint.h>)](https://en.cppreference.com/w/c/types/integer): Exact bit-width types (`uint8_t`, `size_t`) and signedness safety.
   - [Python bytes and bytearray Specification](https://docs.python.org/3/library/stdtypes.html#bytes-and-bytearray-operations): Differences between immutable byte sequences and mutable dynamic byte buffers.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython Objects/bytearrayobject.c Implementation](https://github.com/python/cpython/blob/main/Objects/bytearrayobject.c): How CPython structures resizable raw byte arrays and implements hexadecimal string conversions.
   - [SIMD Memory Vectorization Fundamentals](https://en.wikipedia.org/wiki/Single_instruction,_multiple_data): How modern compilers optimize contiguous byte operations with vector extensions.
