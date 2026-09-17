# Milestone: NumPy-Style Raw Float Matrix & Strided Views

**ID:** `e67df2f`\
**Status:** Planned\
**Difficulty:** 3 / 5\
**Focus:** Implement a NumPy-inspired 2D matrix object storing raw float arrays with strided access, zero-copy transpose views, reference-counted base buffer sharing, and polymorphic linear algebra operations.\
**Prerequisites:** [Complex Numbers & Arithmetic](212a3d8_complex_numbers.md), [Polymorphic Multiplication & Sequence Repetition](687b4cb_polymorphic_multiplication.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   1. Introduce `MATRIX` as a first-class `object_kind_t` in `src/object.h` backed by an embedded `matrix_t` payload in `object_data_t`.
   1. Support owning matrices with contiguous row-major flat `float` buffers (`stride_row = cols`, `stride_col = 1`).
   1. Support non-owning strided matrix views (e.g. `matrix_transpose()`) that borrow the underlying `data` buffer by swapping strides (`stride_row` $\\leftrightarrow$ `stride_col`, `rows` $\\leftrightarrow$ `cols`) without duplicating element memory.
   1. Enforce base buffer lifecycle retention: views retain a non-null pointer to their root owner object (`base`), keeping it alive via reference counting (`refcount_inc(base)` / `refcount_dec(base)`) and garbage collection mark traversal (`mark_object(vm, mat->base)`).
   1. Implement elementwise 2D accessors: `matrix_get()` and `matrix_set()` with bounds checking and strided offset indexing ($r \\times \\text{stride_row} + c \\times \\text{stride_col}$).
   1. Integrate matrix operations into the polymorphic runtime: `object_len()` returns row count (`rows`), `object_add()` dispatches to `matrix_add()` for dimension-checked elementwise addition, and introduce `matrix_matmul()` for standard $O(N^3)$ matrix multiplication.
1. **Scope Boundaries**:
   1. Multi-dimensional tensors ($N > 2$ dimensions) and arbitrary negative or non-contiguous slicing windows are deferred to future array programming milestones.
   1. Broadcasting (e.g., auto-expanding 1D row vectors to match 2D matrices) is deferred; operations in v1 require strict dimension matching.
   1. Mutating a view through `matrix_set()` mutates the shared underlying storage (standard NumPy behavior), but matrix resizing or dynamic buffer reallocation is out of scope.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - `matrix_t` structure definition in `src/object.h`:
     ```c
     typedef struct {
         size_t rows;
         size_t cols;
         size_t stride_row;
         size_t stride_col;
         float *data;
         object_t *base; // NULL for owning buffers; points to root owner for views
     } matrix_t;
     ```
   - Owning Matrix vs. Strided View Memory Graph:
     ```
     Owner Matrix (A: 2x3)
     +-----------------------------------------+
     | object_t                                |
     | kind: MATRIX                            |
     | data.v_matrix:                          |
     |   rows: 2, cols: 3                      |
     |   stride_row: 3, stride_col: 1          |
     |   base: NULL                            |
     |   data: ------------+                   |
     +---------------------|-------------------+
                           |
                           v
              +-------------------------------+
              | float[6]: [0, 1, 2, 3, 4, 5]  | (heap allocated)
              +-------------------------------+
                           ^
                           | (shares same buffer!)
     Transposed View (B = A.T: 3x2)            |
     +-----------------------------------------+
     | object_t                                |
     | kind: MATRIX                            |
     | data.v_matrix:                          |
     |   rows: 3, cols: 2                      |
     |   stride_row: 1, stride_col: 3          |
     |   base: ------------> [Owner Matrix A]  | (refcount of A incremented!)
     |   data: ------------+                   |
     +-----------------------------------------+
     ```
   - Element addressing formula:
     $$\\text{address}(r, c) = \\text{data} + (r \\times \\text{stride_row} + c \\times \\text{stride_col})$$
1. **Core Systems Invariants**:
   1. **Base Root Invariant**: When creating a view from an existing view, `new_view->base` points directly to the ultimate root owner (`mat->base ? mat->base : mat`), avoiding cascading view-of-view reference chains.
   1. **Lifetime Extension Invariant**: If a view exists, its `base` owner's `refcount` is incremented upon view construction and decremented upon view destruction. An owner's `data` buffer is never freed while any view is alive.
   1. **Payload Deallocation Invariant**: In `object_free_payload()`, if `mat.base == NULL`, `mat.data` is freed via `vm_free()`. If `mat.base != NULL`, `mat.data` is NOT freed (ownership belongs to `base`), but `refcount_dec(mat.base)` is called.
   1. **GC Tracing Invariant**: In `mark_object()`, if `mat.base != NULL`, `mark_object(vm, mat.base)` is called to ensure reachability propagates from views to owners during cycle collection. If `mat.base == NULL`, the matrix is a leaf node in the object graph (no sub-objects to trace).
   1. **Dimension Multiplication Overflow Invariant**: Every allocation calculation `rows * cols * sizeof(float)` validates that `rows > 0`, `cols > 0`, and `rows <= SIZE_MAX / cols` to prevent integer overflow vulnerabilities (SEI CERT C rule MEM35-C).
1. **Architectural Trade-offs**:
   1. **Zero-Copy Views vs. Memory Retention**: Strided views allow $O(1)$ transposition without duplicating floating-point data, but keeping a small view alive retains the entire underlying allocation.
   1. **Internal `matrix_t` Payload vs. Heap Allocation**: Embedding `matrix_t` directly into `object_data_t` (matching `list_t`) saves a heap allocation per matrix and improves cache locality, at the cost of expanding `object_data_t` size by a few words.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   - NumPy Strided Array Internals: Representing N-dimensional shapes and permutations purely through offset strides over flat 1D memory.
   - Non-Owning Handles & Resource Ownership: Differentiating between an object that owns memory and an object that borrows memory.
   - Reference-Counted Buffer Anchoring: Using an explicit owner pointer (`base`) to extend the lifetime of shared underlying memory.
1. **Socratic Inquiries**:
   - What happens if we transpose a matrix twice ($A.T.T$)? Does the second transpose point to the first view, or directly to the owner $A$? What are the lifetime and cycle collection implications?
   - Why doesn't an owner matrix need its child references traced in `mark_object()`, whereas a view matrix does?
   - How does strided access affect CPU cache line spatial locality during row-wise vs. column-wise iteration?
   - When matrix addition $C = A + B$ is computed, should $C$ inherit the strides of $A$, or should $C$ always be a freshly allocated contiguous owner matrix?
1. **Failure Modes & Pitfalls**:
   - Double-free vulnerability: Attempting to `vm_free(mat.data)` on a view matrix whose buffer is owned by `base`.
   - Dangling buffer pointer: Creating a view without incrementing `base->refcount`, leading to use-after-free when the owner is collected.
   - Integer overflow during buffer allocation: Failing to check `rows * cols` for `size_t` wraparound before multiplying by `sizeof(float)`.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. Add `MATRIX` to `object_kind_t` and define `matrix_t` in `src/object.h`. Add `matrix_t v_matrix` to `object_data_t`.
   1. Implement `new_matrix(size_t rows, size_t cols, const float *init_data)` in `src/new.c` and declare it in `src/new.h`.
   1. Implement `matrix_transpose(object_t *mat)` in `src/object.c` and `src/object.h` to construct zero-copy views.
   1. Implement accessors `matrix_get()` and `matrix_set()` with bounds and stride calculations in `src/object.c`.
   1. Implement `matrix_add(const object_t *a, const object_t *b)` and `matrix_matmul(const object_t *a, const object_t *b)` in `src/object.c`.
   1. Wire `MATRIX` into `object_add()` (for elementwise addition) and `object_len()` (returning `rows`) in `src/object.c`.
   1. Update memory lifecycle functions in `src/object.c`: `object_free_payload()` (free data only if `base == NULL`, decref `base` if view) and `object_decref_children()`.
   1. Update cycle collector traversal in `src/vm.c`: `mark_object()` marks `base` if non-null.
1. **File Touchpoints**:
   - `src/object.h`: Declare `matrix_t`, enum `MATRIX`, and matrix API prototypes.
   - `src/object.c`: Implement matrix accessors, linear algebra, lifecycle deallocation, and polymorphism.
   - `src/new.h`: Declare `new_matrix()` constructor.
   - `src/new.c`: Implement `new_matrix()` with overflow checking.
   - `src/vm.c`: Update `mark_object()` for view base retention.
   - `tests/test_matrix.c`: New adversarial test suite probing bounds, views, matmul, and leak tracking.
   - `tests/test_runner.c`: Register matrix test suite with µnit.
   - `justfile`: Add `test_matrix.o` to build and coverage targets.

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**:
   1. Owning matrix creation with zeros and initial float data arrays.
   1. Out-of-bounds access rejection in `matrix_get()` and `matrix_set()`.
   1. Zero-copy transpose view verification: mutating element `(r, c)` in transpose view immediately mutates element `(c, r)` in the parent matrix.
   1. Lifetime extension: owner matrix reclaimed by stack/frame, but underlying buffer remains valid while transpose view is kept alive.
   1. Double transpose ($A.T.T$) produces original indexing behavior with direct reference to the root owner.
   1. Matrix addition ($A + B$) dimension mismatch validation (returns `NULL` safely without leaks).
   1. Matrix multiplication ($A \\times B$) inner dimension validation ($A\_{\\text{cols}} == B\_{\\text{rows}}$) and numerical correctness against known products.
   1. Integer overflow fuzzing: calling `new_matrix(SIZE_MAX, 2, NULL)` fails gracefully returning `NULL`.
   1. Allocation failure simulation via `boot_set_fail_alloc_after()` during matrix creation and operations.
1. **Zero-Leak Guarantee**:
   1. All allocated matrices, views, and data buffers freed cleanly with `assert(boot_all_freed())`.
1. **Tooling Quality Gates**:
   1. 100% pass rate in `just test` under AddressSanitizer and UndefinedBehaviorSanitizer.
   1. 100.00% line coverage in `just coverage` across all touched files.
   1. `just lint` (`clang-tidy`, docstring lint, and roadmap lint) passes cleanly.
   1. `just check` passes with all pre-commit hooks clean.
1. **Milestone Completion & Lesson Extraction**:
   1. Upon achieving green tests, zero leaks, and 100% coverage, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`.
   1. Update Mermaid node styling to `:::completed`.
   1. Document educational takeaways in `lessons/` (e.g. `lessons/05_strided_arrays_and_view_semantics.md`) per the `lesson-extraction` skill.
