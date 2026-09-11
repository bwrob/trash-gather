# ISO C17 Portability & Language Evolution Guide

This reference defines standards compliance, post-C99 language features, compiler compatibility, and portability invariants for C systems code.

______________________________________________________________________

## 1. The Language Target: ISO C17 (`-std=c17`)

ISO C17 (ISO/IEC 9899:2018) is a bug-fix and defect-resolution release of C11. It introduces no new syntax beyond C11, making it the most stable, mature modern baseline across all mainstream compilers (GCC, Clang, MSVC).

### Core Principle: Portability First

Systems code must compile cleanly across all major platforms and standard compilers without depending on non-standard compiler extensions (GNU extensions, MSVC-specific pragmas).

______________________________________________________________________

## 2. Conscious Post-C99 Syntax

Decisions to use features introduced in C11/C17 must be **conscious, intentional, and justified**.

### 1. Anonymous Structs and Unions (C11 §6.7.2.1)

- **When Justified**: Tagged unions and composite records where qualified naming adds unnecessary verbosity without improving safety.
  ```c
  /* With anonymous union: direct access via obj->v_int */
  typedef struct {
    object_kind_t kind;
    union {
      int64_t v_int;
      double v_float;
      void *v_ptr;
    };
  } object_t;
  ```
- **Portability Note**: Supported by GCC, Clang, and MSVC (when compiling in C11/C17 mode). Always verify that field names do not collide with outer struct members.

### 2. Compile-Time Assertions (`_Static_assert` / `static_assert`)

- **When Justified**: Validating ABI invariants, struct alignment, and size guarantees at compile time rather than runtime:
  ```c
  #include <assert.h>
  _Static_assert(sizeof(object_t) == 32, "object_t must remain exactly 32 bytes for cache line packing");
  ```

### 3. Type-Generic Expressions (`_Generic`)

- **When Justified**: Macro interfaces that dispatch based on operand type without sacrificing type safety:
  ```c
  #define print_val(x) _Generic((x), \
    int: print_int,                  \
    double: print_double,            \
    default: print_default)(x)
  ```

______________________________________________________________________

## 3. Risky & Prohibited Constructs

Even though allowed by some C standards, avoid the following constructs in portable systems runtimes:

### 1. Variable-Length Arrays (VLAs)

- **Status**: Made **optional** in C11 (conditional on `__STDC_NO_VLA__`).
- **Hazard**: Stack allocations dependent on input variables can silently overflow the thread stack without recourse.
- **Rule**: Prohibited. Use fixed-size buffers or explicit heap allocations with bounds checking.

### 2. Complex Numbers (`<complex.h>`)

- **Status**: Optional in C11 (`__STDC_NO_COMPLEX__`).
- **Rule**: Avoid unless explicitly required by domain-specific mathematical libraries.

### 3. Non-Standard Compiler Extensions

- Prohibited:
  - GCC statement expressions `({ ... })`
  - GNU nested functions
  - Non-standard attributes without feature-test macro guards (`__attribute__((...))` should be wrapped in portable macros or avoided in public headers)

______________________________________________________________________

## 4. Cross-Platform Type Safety

1. **Fixed-Width Integers**: Always use fixed-width integer types (`int8_t`, `int16_t`, `int32_t`, `int64_t`, `uint8_t`, etc.) from `<stdint.h>` for data layouts and wire formats.
1. **Buffer Indices & Sizes**: Use `size_t` for counts, byte sizes, and memory offsets. Use `ptrdiff_t` for pointer differences.
1. **Format Specifiers**: Always use `PRI*` macros from `<inttypes.h>` (e.g. `PRId64`, `PRIu64`) when printing fixed-width types with `printf`.
