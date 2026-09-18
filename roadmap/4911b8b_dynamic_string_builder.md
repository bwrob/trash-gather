# Milestone: Dynamic String Builder & Safe Formatting

**ID:** `4911b8b`\
**Status:** Planned\
**Difficulty:** 2 / 5\
**Focus:** Build an amortized dynamic byte/string buffer (`string_builder_t`) supporting `sb_append()`, `sb_append_format()`, and `sb_build()`, mastering `snprintf` sizing semantics, geometric buffer growth, and safe null-termination guarantees.\
**Prerequisites:** [Dynamic Resizable List Mutations](d7b5feb_dynamic_resizable_list.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Implement a lightweight, resizable string builder abstraction (`string_builder_t`) tracking `char *buffer`, `size_t length`, and `size_t capacity`.
   - Implement `sb_new(size_t initial_capacity)` and `sb_free(string_builder_t *sb)`.
   - Implement `sb_append(string_builder_t *sb, const char *str)` and `sb_append_char(string_builder_t *sb, char c)`.
   - Implement `sb_append_format(string_builder_t *sb, const char *fmt, ...)` using `vsnprintf` and handling buffer growth if formatting exceeds capacity.
   - Implement `sb_build(string_builder_t *sb)` to transfer buffer ownership to a new `STRING` object or dynamically allocated C-string.
1. **Scope Boundaries**:
   - Cycle-safe recursive object representation is deferred to Milestone `bf0a981_cycle_safe_string_repr.md`.
   - Dynamic slicing and views are handled in Milestone `c44db02_dynamic_slices_and_byte_buffers.md`.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - String builder heap layout:
     ```
     +-------------------------------------------------+
     | string_builder_t                                |
     |  size_t length    (e.g., 5)                     |
     |  size_t capacity  (e.g., 16)                    |
     |  char *buffer ------------------------------\   |
     +---------------------------------------------+---+
                                                   |
         +-----------------------------------------+
         v
     +---+---+---+---+---+----+---+---+---+---+---+---+---+---+---+---+
     | H | e | l | l | o | \0 | x | x | x | x | x | x | x | x | x | x |
     +---+---+---+---+---+----+---+---+---+---+---+---+---+---+---+---+
     ```
1. **Core Systems Invariants**:
   - Null-termination invariant: `buffer[length] == '\0'` must hold at all times, and `capacity > length` (capacity must always include room for the trailing null byte).
   - `snprintf` sizing invariant: Use the return value of `vsnprintf` (which returns the number of characters that would have been written, excluding `\0`) to detect truncation and resize the buffer to exact required capacity.
   - Ownership handover: Calling `sb_build()` moves the underlying `char *buffer` out of the builder or allocates a compact copy, ensuring no double-free or dangling references when `sb` is discarded.
1. **Architectural Trade-offs**: Dynamic growth amortizes reallocation cost across many small string concatenations, eliminating the quadratic copy overhead ($O(N^2)$) of naive repeated `strcat` / `strdup`.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Buffer overrun vulnerabilities in C (CWE-120); `snprintf` vs `sprintf` vs `strcpy`; geometric buffer growth; va_list delegation via `va_copy` and `vsnprintf`.
1. **Socratic Inquiries**:
   - Why is repeatedly calling `strcat(dest, src)` in a loop an $O(N^2)$ operation, and how does tracking `length` reduce it to $O(N)$?
   - What does `snprintf(buf, size, fmt, ...)` return when the output is truncated, and why does this make two-pass formatting possible?
   - Why must you use `va_copy` if you need to run `vsnprintf` twice (once to measure length, once to format into the resized buffer)?
1. **Failure Modes & Pitfalls**: Off-by-one errors forgetting space for the terminating null character (`capacity < length + 1`); failing to call `va_end` on copies made with `va_copy`; passing an un-reallocated buffer when `vsnprintf` indicates truncation.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Create `src/string_builder.h` declaring `string_builder_t` and builder operations (`sb_new`, `sb_free`, `sb_append`, `sb_append_format`, `sb_build`).
   - Create `src/string_builder.c` implementing geometric capacity growth and string operations.
   - Use `va_copy` and `vsnprintf` inside `sb_append_format` to format strings safely without truncation.
   - Add unit tests in `tests/test_string_builder.c` verifying small appends, massive multi-kilobyte strings, and formatted numbers.
1. **File Touchpoints**:
   - `src/string_builder.h`, `src/string_builder.c`
   - `tests/test_string_builder.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify appending empty strings, single chars, large strings (> 4 KB); format combinations (`%d`, `%s`, `%f`, `%p`); simulate allocation failure during buffer expansion.
1. **Zero-Leak Guarantee**: Both `sb_free()` and `sb_build()` paths verify zero memory leaks via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______________________________________________________________________

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Safe String Formatting in C with vsnprintf](https://en.cppreference.com/w/c/io/vfprintf): Using two-pass sizing with vsnprintf and va_copy for buffer overrun prevention.
   - [CWE-120: Buffer Copy without Checking Size of Input](https://cwe.mitre.org/data/definitions/120.html): Security analysis of classic C buffer overflows and standard mitigations.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [Redis Simple Dynamic Strings (SDS) Architecture](https://github.com/redis/redis/blob/unstable/src/sds.c): Production design of high-throughput dynamic string buffers with cached length and capacity.
   - [CPython Objects/unicodeobject.c Writer API](https://github.com/python/cpython/blob/main/Objects/unicodeobject.c): How Python constructs formatted strings dynamically while guaranteeing null termination.
