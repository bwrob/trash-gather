# Milestone: Contiguous 2D Float Matrix & Elementwise Arithmetic

**ID:** `e67df2f`\
**Status:** Planned\
**Difficulty:** 2 / 5\
**Focus:** Implement a flat row-major 2D float matrix object (`matrix_t`), 2D elementwise get/set accessors with dimension validation, and polymorphic elementwise arithmetic operations.\
**Prerequisites:** [Complex Numbers & Arithmetic](212a3d8_complex_numbers.md), [Polymorphic Multiplication & Sequence Repetition](687b4cb_polymorphic_multiplication.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Introduce `MATRIX` as an `object_kind_t` backed by an embedded `matrix_t` in `object_data_t`.
   - Store flat, contiguous row-major `float` buffers (`rows`, `cols`, `float *data`).
   - Implement `new_matrix(size_t rows, size_t cols)` allocating a single contiguous `float[rows * cols]` array.
   - Implement 2D accessors: `matrix_get(object_t *mat, size_t r, size_t c)` and `matrix_set(object_t *mat, size_t r, size_t c, float val)` with bounds checking.
   - Implement elementwise operations: `matrix_add(a, b)` requiring identical dimensions, and scalar multiplication `matrix_scale(mat, float factor)`.
   - Hook into `object_len(mat)` to return row count `rows`.
1. **Scope Boundaries**:
   - Strided access, zero-copy transpose (`A.T`), and base buffer retention are deferred to Milestone `eb930e7_strided_matrix_views.md`.
   - Full matrix multiplication ($O(N^3)$ `matmul`) is deferred to Milestone `eb930e7_strided_matrix_views.md`.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Contiguous 2D row-major matrix layout:

     ```text
     object_t
       [kind = MATRIX]
       [data.v_matrix]
          rows: 2
          cols: 3
          data: -------------> float[6]: [1.0, 2.0, 3.0, 4.0, 5.0, 6.0]
     ```

   - Row-major linear index translation:
     $$\\text{index}(r, c) = r \\times \\text{cols} + c$$
1. **Core Systems Invariants**:
   - Bounds invariant: Accessing $(r, c)$ is valid if and only if $0 \\le r < \\text{rows}$ and $0 \\le c < \\text{cols}$.
   - Memory management: `matrix->data` is owned exclusively by the matrix object. When the matrix is collected, `object_free_payload()` frees `matrix->data`.
   - Dimension matching: Binary elementwise addition requires `a->rows == b->rows` and `a->cols == b->cols`. Mismatched dimensions fail safely without memory corruption.
1. **Architectural Trade-offs**: Flat 1D arrays are cache-friendly and contiguous compared to arrays of pointers (`float**`), eliminating pointer chasing and extra allocations.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Row-major vs column-major array layouts in memory; cache spatial locality; multi-dimensional index mapping in flat memory buffers.
1. **Socratic Inquiries**:
   - Why is allocating a single flat `float *data` buffer of size `rows * cols` significantly faster and more cache-efficient than allocating `rows` separate `float*` row buffers?
   - How does row-major order align with C's 2D array memory layout (`float arr[R][C]`)?
   - What happens if `rows * cols` overflows `size_t` during allocation?
1. **Failure Modes & Pitfalls**: Integer multiplication overflow when computing `rows * cols * sizeof(float)`; 2D indexing off-by-one errors; forgetting to free `data` in `object_free_payload`.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Define `matrix_t` in `src/object.h`: `typedef struct { size_t rows; size_t cols; float *data; } matrix_t;`.
   - Add `MATRIX` to `object_kind_t` and `matrix_t v_matrix;` to `object_data_t` in `src/object.h`.
   - Implement `new_matrix(size_t rows, size_t cols)` in `src/new.c`.
   - Implement accessors `matrix_get` and `matrix_set` in `src/object.c`.
   - Implement `matrix_add` and integrate into `object_add` in `src/object.c`.
   - Add payload cleanup in `object_free_payload` in `src/object.c`.
   - Add unit tests in `tests/test_matrix.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/new.h`, `src/new.c`
   - `tests/test_matrix.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify 2D indexing on various dimensions ($1 \\times 1$, $10 \\times 10$, $100 \\times 5$); verify bounds check rejection on row/col out of bounds; verify elementwise addition and dimension mismatch rejection.
1. **Zero-Leak Guarantee**: Verify matrix buffers cleanly freed via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Row-Major vs Column-Major Memory Layouts](https://en.wikipedia.org/wiki/Row-_and_column-major_order): Memory address mapping for 2D matrices and cache-friendly linear traversal.
   - [Locality of Reference and CPU Cache Lines](https://en.wikipedia.org/wiki/Locality_of_reference): Understanding spatial locality and stride access performance impacts on modern hardware.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [Basic Linear Algebra Subprograms (BLAS) Interface](https://www.netlib.org/blas/): Standard C memory conventions for high-throughput dense matrix operations.
   - [NumPy Flatiter and Contiguous Memory Layouts](https://numpy.org/doc/stable/reference/c-api/types-and-structures.html): How NumPy handles 2D flat buffers and dimension-validated elementwise arithmetic.
