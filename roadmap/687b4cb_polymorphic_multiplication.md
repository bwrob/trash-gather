# Milestone: Polymorphic Multiplication & Sequence Repetition

**ID:** `687b4cb`\
**Status:** Planned\
**Focus:** Implement polymorphic binary multiplication (`multiply`) supporting numeric arithmetic (integer and float) and Python-style sequence repetition (string, list, tuple) with commutative operand ordering.\
**Prerequisites:** [Polymorphic Sequence Length Protocol](b81f9a7_polymorphic_sequence_length.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Implement polymorphic multiplication entry point: `object_t *multiply(object_t *a, object_t *b);`.
   - Support numeric multiplication across scalar types:
     - `INTEGER * INTEGER` producing a new `INTEGER`.
     - `INTEGER * FLOAT` and `FLOAT * INTEGER` producing a new `FLOAT`.
     - `FLOAT * FLOAT` producing a new `FLOAT`.
   - Support sequence repetition with integer multipliers:
     - `STRING * INTEGER` and `INTEGER * STRING` repeating characters (e.g. `"abc" * 3 -> "abcabcabc"`).
     - `LIST * INTEGER` and `INTEGER * LIST` creating a new list with repeated element references.
     - `TUPLE * INTEGER` and `INTEGER * TUPLE` creating a new contiguous tuple with repeated element references.
   - Handle boundary and non-positive multipliers:
     - Multiplier $N \\le 0$ on any sequence returns a newly allocated, valid empty container of that type (empty string `""`, empty list `[]`, empty tuple `()`).
   - Guarantee commutative operand dispatch: `seq * n` and `n * seq` must execute identical repetition logic.
   - Defensively reject invalid operand combinations (e.g. `STRING * STRING`, `LIST * FLOAT`, `TUPLE * LIST`, or passing `NULL`), returning `NULL` safely without memory faults.
1. **Scope Boundaries**:
   - In-place augmented multiplication (`*=` / `__imul__`) is deferred to bytecode evaluation engine milestones.
   - Arbitrary-precision bignum arithmetic is deferred to advanced numeric runtime milestones.
   - User-defined class operator overloading (`__mul__` / `__rmul__`) is deferred to object-oriented method lookup milestones.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Polymorphic dispatch branching:
     ```
     multiply(a, b)
       |
       +--> (INTEGER, INTEGER)  -> new_integer(a * b)
       +--> (INTEGER, FLOAT)    -> new_float(a * b)
       +--> (FLOAT, INTEGER)    -> new_float(a * b)
       +--> (FLOAT, FLOAT)      -> new_float(a * b)
       +--> (STRING, INTEGER)   -> _repeat_string(a, b->data.v_int)
       +--> (INTEGER, STRING)   -> _repeat_string(b, a->data.v_int)
       +--> (LIST, INTEGER)     -> _repeat_list(a, b->data.v_int)
       +--> (INTEGER, LIST)     -> _repeat_list(b, a->data.v_int)
       +--> (TUPLE, INTEGER)    -> _repeat_tuple(a, b->data.v_int)
       +--> (INTEGER, TUPLE)    -> _repeat_tuple(b, a->data.v_int)
       +--> Otherwise           -> NULL (unsupported operand types)
     ```
   - Shallow copy reference sharing in repeated lists (`L = [A, B]`, `L * 2 -> [A, B, A, B]`):
     ```
     Original List:
       elements[0] ----> [ Object A ] (refcount: 1)
       elements[1] ----> [ Object B ] (refcount: 1)

     Repeated List (new object):
       elements[0] ----> [ Object A ] (refcount: 2)
       elements[1] ----> [ Object B ] (refcount: 2)
       elements[2] ----> [ Object A ] (refcount: 3)
       elements[3] ----> [ Object B ] (refcount: 3)
     ```
1. **Core Systems Invariants**:
   - **Integer Overflow Protection**: Before computing buffer sizes or allocating payloads (`size_t total = len * count`), callers must verify against multiplication overflow:
     $$\\text{count} > 0 \\land \\text{len} > \\frac{\\text{SIZE_MAX}}{\\text{count}} \\implies \\text{abort / return NULL}$$
   - **Shallow Reference Ownership**: Every repeated element inserted into a new list or tuple must have its reference count incremented via `refcount_inc(elem)`.
   - **Multi-Stage Allocation Rollback**: If memory allocation fails midway through repeating container elements at step $K$, all previously incremented elements ($0 \\le i < K$) must be decremented via `refcount_dec()` and the allocated memory freed cleanly before returning `NULL`.
   - **Boundary Multiplier Invariant**: Multiplying by zero or any negative integer ($N \\le 0$) must return an empty container of the appropriate kind, never `NULL`.
   - **Commutativity Invariant**: For all supported types $A$ and $B$, `multiply(a, b)` and `multiply(b, a)` must evaluate to semantically equivalent objects.
   - **Input Constness**: Multiplying sequences must never mutate the input sequences, their element arrays, or their existing sizes.
1. **Architectural Trade-offs**:
   - Shallow copying vs deep copying: Shallow copying matches Python's language semantics and eliminates recursive allocation overhead, but creates shared reference aliasing where mutating an element inside one position is visible at repeated positions.
   - Contiguous tuple sizing: Repeating a variable-length tuple requires computing the exact total size upfront ($\\text{sizeof}(\\text{tuple_t}) + N \\times \\text{count} \\times \\text{sizeof}(\\text{object_t}\*)$) for a single contiguous allocation.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   - Algebraic properties in runtime operators: Commutative ring properties for scalars versus non-commutative monoid action for sequence scaling ($S \\times \\mathbb{Z} \\to S$).
   - Unsigned integer overflow vulnerabilities (SEI CERT C rule INT30-C): why `len * count` wraps around silently in C without explicit boundary checks.
   - Shallow reference aliasing and shared memory models.
   - Transactional memory rollbacks: ensuring zero memory leaks and refcount balance under partial allocation failure.
1. **Socratic Inquiries**:
   - In Python, if you evaluate `x = [[0]] * 3; x[0][0] = 1; print(x)`, why does it output `[[1], [1], [1]]`? What does this demonstrate about shallow sequence repetition?
   - In C, if `size_t len = 0x40000000` and `int count = 4`, why does `len * count` equal `0` on 32-bit systems? How does checking `len > SIZE_MAX / count` prevent this vulnerability?
   - If allocating a repeated tuple runs out of memory after copying 500 element references, why is calling `refcount_dec()` on those 500 items strictly necessary before freeing the tuple container?
1. **Failure Modes & Pitfalls**:
   - Arithmetic overflow leading to undersized buffer allocation and subsequent out-of-bounds heap writes.
   - Negative multiplier sign confusion: casting a negative signed integer to `size_t` without checking if $N < 0$, resulting in an enormous positive number (`SIZE_MAX - |N| + 1`).
   - Memory leaks on partial allocation failure due to forgotten rollback loops.
   - Missing `refcount_inc()` on repeated items causing double-free and use-after-free bugs when either container is collected.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Declare `object_t *multiply(object_t *a, object_t *b);` in `src/object.h`.
   - Implement `static object_t *_repeat_string(object_t *str, int count);` in `src/object.c`:
     - Check for integer overflow and allocate `(len * count) + 1` bytes.
     - Copy string payload repeatedly using `memcpy`.
   - Implement `static object_t *_repeat_list(object_t *list, int count);` in `src/object.c`:
     - Allocate new `list_t` with capacity `len * count`.
     - Populate elements with `refcount_inc(elem)`.
     - Implement clean rollback loop on allocation failure.
   - Implement `static object_t *_repeat_tuple(object_t *tuple, int count);` in `src/object.c`:
     - Calculate contiguous size with overflow protection.
     - Allocate and populate with `refcount_inc(elem)`.
     - Implement rollback loop on failure.
   - Implement `multiply()` in `src/object.c` with bidirectional type dispatch across scalars and sequences.
   - Add unit and adversarial tests in `tests/test_object.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `tests/test_object.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**:
   - Scalar numeric multiplication: verify `INTEGER * INTEGER`, `INTEGER * FLOAT`, `FLOAT * INTEGER`, and `FLOAT * FLOAT`.
   - String repetition: verify `"abc" * 3 == "abcabcabc"`, `3 * "abc" == "abcabcabc"`, `"abc" * 0 == ""`, and `"abc" * -5 == ""`.
   - List repetition: verify `[10, 20] * 3 == [10, 20, 10, 20, 10, 20]`, `3 * [10, 20] == [10, 20, 10, 20, 10, 20]`, `[10] * 0 == []`, and `[] * 5 == []`.
   - Tuple repetition: verify `(1, 2) * 3 == (1, 2, 1, 2, 1, 2)` and non-positive multipliers return empty tuples.
   - Reference count validation: assert that elements repeated $K$ times have their reference count increased by exactly $K$.
   - Failure injection test: use `boot_set_fail_alloc_after` during list/tuple repetition to verify clean rollback with zero leaks.
   - Unsupported types: verify `multiply("a", "b")`, `multiply(list, 2.5f)`, and `multiply(NULL, x)` safely return `NULL`.
1. **Zero-Leak Guarantee**:
   - All tests must pass with `assert(boot_all_freed())`, verifying complete reclamation of repeated sequences and their child elements.
1. **Tooling Quality Gates**:
   - `just test` (100% pass rate under AddressSanitizer and UndefinedBehaviorSanitizer).
   - `just lint` (`clang-tidy`, Doxygen docstrings, and `just lint-roadmap`).
   - `just check` (all pre-commit hooks clean).
1. **Milestone Completion & Lesson Extraction**:
   - Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson file in `lessons/` following the `lesson-extraction` skill.
