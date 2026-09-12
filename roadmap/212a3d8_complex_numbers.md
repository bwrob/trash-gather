# Milestone: Complex Numbers & Arithmetic

**ID:** `212a3d8`\
**Status:** Planned\
**Focus:** Introduce Python-style complex numbers (`COMPLEX`) with 64-bit IEEE 754 components, integrating into polymorphic arithmetic (`add`, `multiply`), truthiness evaluation, and lifecycle tracking without compiler-specific extensions.\
**Prerequisites:** [Polymorphic Multiplication & Sequence Repetition](687b4cb_polymorphic_multiplication.md), [Boolean Immortal Singletons & Truthiness](6c3a989_bool_singletons_and_truthiness.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Introduce `COMPLEX` kind to the runtime object model in `object_kind_t`.
   - Define a pure, portable `complex_t` payload structure containing double-precision real and imaginary components (`double real; double imag;`) inside `object_data_t`.
   - Implement factory constructor `object_t *new_complex(double real, double imag);` registered with VM tracking.
   - Implement component accessors and utilities: `double complex_real(const object_t *obj);`, `double complex_imag(const object_t *obj);`, and `object_t *complex_conjugate(const object_t *obj);`.
   - Integrate `COMPLEX` into polymorphic addition ([`add()`](../src/object.c)) with numeric promotion:
     - `COMPLEX + COMPLEX` $\\to$ $(a_r + b_r) + (a_i + b_i)j$
     - `COMPLEX + FLOAT` and `FLOAT + COMPLEX` $\\to$ $(a_r + b) + a_i j$
     - `COMPLEX + INTEGER` and `INTEGER + COMPLEX` $\\to$ $(a_r + b) + a_i j$
   - Integrate `COMPLEX` into polymorphic multiplication ([`multiply()`](687b4cb_polymorphic_multiplication.md)) with numeric promotion:
     - `COMPLEX * COMPLEX` $\\to$ $(a_r b_r - a_i b_i) + (a_r b_i + a_i b_r)j$
     - `COMPLEX * scalar` and `scalar * COMPLEX` $\\to$ $(a_r s) + (a_i s)j$
   - Integrate `COMPLEX` into polymorphic truthiness ([`object_is_truthy()`](6c3a989_bool_singletons_and_truthiness.md)):
     - Falsy if and only if both `real == 0.0` and `imag == 0.0` (correctly handling IEEE 754 signed zeros `+0.0` and `-0.0`).
     - Truthy if either component is non-zero.
1. **Scope Boundaries**:
   - Complex division and advanced edge-case overflow handling (e.g. Smith's algorithm for division) are deferred to advanced numeric milestones.
   - Transcendental elementary complex functions (`cmath`: `exp`, `log`, `sin`, `sqrt`) are deferred to standard math library milestones.
   - Non-standard or optional C compiler extensions (such as `<complex.h>` or `_Complex` which are optional in ISO C11/C17 via `__STDC_NO_COMPLEX__`) are strictly avoided.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Complex number struct layout stored inline inside `object_data_t`:
     ```c
     typedef struct {
       double real; /* Real component */
       double imag; /* Imaginary component */
     } complex_t;
     ```
   - Inline memory layout of `object_t` with `COMPLEX`:
     ```
     +-------------------------------------------------------+
     |                       object_t                        |
     +-----------------+-------------------+-----------------+
     |  is_marked: 1   |   refcount: N     |  tracker_id: ID |
     +-----------------+-------------------+-----------------+
     |  kind: COMPLEX                                        |
     +-------------------------------------------------------+
     |  data.v_complex (16 bytes inline)                     |
     |    double real (8 bytes)                              |
     |    double imag (8 bytes)                              |
     +-------------------------------------------------------+
     ```
   - Numeric promotion ladder:
     ```
     INTEGER  --->  FLOAT  --->  COMPLEX
       (int)        (double)     (double real, double imag)
     ```
1. **Core Systems Invariants**:
   - **Zero Auxiliary Heap Allocations**: Since `sizeof(complex_t)` is 16 bytes (two 64-bit IEEE 754 doubles), it fits by value inside `object_data_t`, requiring zero extra heap allocations beyond the `object_t` header.
   - **Strict ISO C17 Portability**: Never include `<complex.h>` or use the `_Complex` keyword. The runtime must compile cleanly and identically across GCC, Clang, and MSVC without depending on optional C11/C17 features.
   - **IEEE 754 Zero Semantics**: Truthiness checks must evaluate `obj->data.v_complex.real == 0.0 && obj->data.v_complex.imag == 0.0`. In IEEE 754, `+0.0 == -0.0` evaluates to true, guaranteeing that `-0.0 + -0.0j` correctly tests falsy.
   - **Commutative Promotion**: For any scalar $S$ and complex number $Z$, `add(z, s)` and `add(s, z)` must produce equal complex objects, as must `multiply(z, s)` and `multiply(s, z)`.
   - **Immutability Invariant**: Complex objects are immutable scalars; arithmetic operations and conjugation must allocate new objects rather than mutating inputs.
1. **Architectural Trade-offs**:
   - Double-precision (`double`) vs single-precision (`float`): Using `double` provides 53 bits of mantissa precision matching standard Python `complex` semantics, prevents precision truncation when 32-bit integers are promoted, and fits within existing 16-byte union slots without increasing `sizeof(object_t)`.
   - Portable struct vs compiler complex extensions: Using an explicit C struct guarantees portability to platforms where `__STDC_NO_COMPLEX__` is defined, eliminating vendor compiler lock-in.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   - Numeric towers and bidirectional type coercion in dynamically typed runtime systems.
   - IEEE 754 floating-point standards: representation of signed zeros (`+0.0`, `-0.0`), infinities, and NaN propagation in complex arithmetic.
   - SEI CERT C rules and ISO C17 standards on optional language features (`__STDC_NO_COMPLEX__`).
   - Algebraic field axioms of the complex numbers ($\\mathbb{C}$).
1. **Socratic Inquiries**:
   - In Python, why does `bool(0 + 0j)` evaluate to `False`, but `(0 + 0j) == False` evaluates to `False`? What does this illustrate about the distinction between truthiness coercion and equality?
   - Why did ISO C11 make `<complex.h>` optional rather than mandatory? Why is an explicit `struct { double real, imag; }` safer for portable systems programming?
   - In floating-point arithmetic, how does `(a_r * b_i + a_i * b_r)` behave when one operand contains `NaN` or infinity?
1. **Failure Modes & Pitfalls**:
   - Assuming only `real == 0.0` determines truthiness, erroneously treating `0.0 + 2.0j` as falsy.
   - Floating-point overflow when multiplying large complex values, resulting in premature infinities.
   - Misaligning union fields or introducing platform-specific padding when adding `double` fields to `object_data_t`.
   - Memory leaks if arithmetic failure paths fail to untrack and free intermediate objects.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Add `COMPLEX` to `object_kind_t` enum in `src/object.h`.
   - Define `complex_t` struct and add `complex_t v_complex;` to `object_data_t` union in `src/object.h`.
   - Declare `object_t *new_complex(double real, double imag);` in `src/new.h`.
   - Declare `double complex_real(const object_t *obj);`, `double complex_imag(const object_t *obj);`, and `object_t *complex_conjugate(const object_t *obj);` in `src/object.h`.
   - Implement `new_complex()` in `src/new.c`.
   - Implement component accessors and `complex_conjugate()` in `src/object.c`.
   - Update `add()` in `src/object.c` with bidirectional dispatch across `INTEGER`, `FLOAT`, and `COMPLEX`.
   - Update `multiply()` in `src/object.c` to support complex arithmetic with scalars and complex operands.
   - Update `object_is_truthy()` in `src/object.c` to evaluate complex numbers.
   - Add comprehensive unit tests in `tests/test_object.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/new.h`, `src/new.c`
   - `tests/test_object.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**:
   - Constructor & Accessors: verify `new_complex(3.0, -4.5)` yields `complex_real == 3.0` and `complex_imag == -4.5`.
   - Complex Addition: verify $(1 + 2j) + (3 + 4j) = (4 + 6j)$, $(2 + 3j) + 5 = (7 + 3j)$, and $5 + (2 + 3j) = (7 + 3j)$.
   - Complex Multiplication: verify $(1 + 2j) \\times (3 + 4j) = (-5 + 10j)$, $(2 + 3j) \\times 2 = (4 + 6j)$, and $2 \\times (2 + 3j) = (4 + 6j)$.
   - Complex Conjugation: verify `complex_conjugate(3 + 4j)` yields `(3 - 4j)` and `complex_conjugate(3 - 4j)` yields `(3 + 4j)`.
   - Truthiness Evaluation: verify `0.0 + 0.0j` and `-0.0 + -0.0j` are falsy, while `0.0 + 1.0j`, `1.0 + 0.0j`, and `-2.0 + 3.0j` are truthy.
   - Non-Sequence Multiplication Rejection: verify `multiply(complex, list)` and `multiply(string, complex)` return `NULL`.
1. **Zero-Leak Guarantee**:
   - All tests must pass with `assert(boot_all_freed())`, verifying complete memory reclamation of complex objects.
1. **Tooling Quality Gates**:
   - `just test` (100% pass rate under AddressSanitizer and UndefinedBehaviorSanitizer).
   - `just lint` (`clang-tidy`, Doxygen docstrings, and `just lint-roadmap`).
   - `just check` (all pre-commit hooks clean).
1. **Milestone Completion & Lesson Extraction**:
   - Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson file in `lessons/` following the `lesson-extraction` skill.
