# Milestone: Strided Matrix Views, Zero-Copy Transpose & Base Retention

**ID:** `eb930e7`\
**Status:** Planned\
**Difficulty:** 3 / 5\
**Focus:** Generalize 2D matrix indexing to row and column strides, implement zero-copy transpose views (`matrix_transpose`) borrowing underlying storage, enforce GC base object retention, and implement matrix multiplication (`matrix_matmul`).\
**Prerequisites:** [Contiguous 2D Float Matrix & Elementwise Arithmetic](e67df2f_raw_float_matrix.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Upgrade `matrix_t` to store explicit strides (`size_t stride_row`, `size_t stride_col`) and a root owner reference (`object_t *base`).
   - Update 2D access formula to strided coordinates: $\\text{offset}(r, c) = r \\times \\text{stride_row} + c \\times \\text{stride_col}$.
   - Implement `matrix_transpose(object_t *mat)` producing a zero-copy view by swapping `rows` $\\leftrightarrow$ `cols` and `stride_row` $\\leftrightarrow$ `stride_col` without duplicating the float buffer.
   - Enforce lifecycle retention: view matrices increment `refcount_inc(base)` and trace `mark_object(base)` during GC marking to prevent premature reclamation of the shared underlying buffer.
   - Implement standard $O(N^3)$ matrix multiplication `matrix_matmul(a, b)` supporting both contiguous matrices and strided views.
1. **Scope Boundaries**:
   - Arbitrary $N$-dimensional tensors ($N > 2$) are deferred to future numeric milestones.
   - NumPy-style broadcasting is an explicit non-goal.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Owning Matrix vs. Strided Transpose View:
     ```
     Owning Matrix (A: 2x3, base=NULL)
     +-----------------------------------------+
     | rows: 2, cols: 3                        |
     | stride_row: 3, stride_col: 1            |
     | data: ------------+                     |
     +-------------------|---------------------+
                         |
                         v
            +-------------------------------+
            | float[6]: [1, 2, 3, 4, 5, 6]  | (heap allocated)
            +-------------------------------+
                         ^
     Transposed View (B = A.T: 3x2, base=A)    |
     +-----------------------------------------+
     | rows: 3, cols: 2                        |
     | stride_row: 1, stride_col: 3            |
     | base: points to A (refcount incremented)|
     | data: ------------/ (shares same data!) |
     +-----------------------------------------+
     ```
1. **Core Systems Invariants**:
   - Base buffer retention invariant: If `mat->base != NULL`, `mat->data` is not freed when `mat` is collected; instead, `refcount_dec(mat->base)` is called. Only owning matrices (`base == NULL`) free their `data` buffer.
   - GC reachability: During garbage collector mark phases, any reached view must immediately mark its `base` object, keeping the physical buffer alive even if user code dropped all direct variables referencing the original owner.
   - Zero-copy guarantee: Transpose creation is an $O(1)$ memory operation that allocates only a new `object_t` header, never copying float elements.
1. **Architectural Trade-offs**: Strided views achieve instant zero-copy transposition, but hold the entire parent buffer alive in memory until all derived views are collected.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Strided array access in scientific computing (BLAS/LAPACK/NumPy); non-owning views and borrowed pointer semantics; memory leaks via unintentional parent buffer retention.
1. **Socratic Inquiries**:
   - How can swapping `stride_row` and `stride_col` produce an exact transposition of a matrix without moving a single byte of element data?
   - What would happen in the garbage collector if a user creates `B = A.T`, lets variable `A` go out of scope, and runs GC sweep without base pointer tracing?
   - Why must `matrix_matmul` use stride-based lookups rather than assuming contiguous linear memory layout?
1. **Failure Modes & Pitfalls**: Double-freeing `data` when both owner and view are collected; use-after-free if the view fails to increment the base object's reference count; cache line thrashing when iterating across large non-unit strides.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Update `matrix_t` in `src/object.h` to include `size_t stride_row; size_t stride_col; object_t *base;`.
   - Update `matrix_get` and `matrix_set` in `src/object.c` to use strided indexing.
   - Implement `object_t *matrix_transpose(object_t *mat)` in `src/object.c`.
   - Update `object_decref_children` and `object_free_payload` in `src/object.c` to handle `base` ownership.
   - Update `trace_blacken_object` in `src/vm.c` to trace `mat->data.v_matrix.base`.
   - Implement `object_t *matrix_matmul(object_t *a, object_t *b)` in `src/object.c`.
   - Add unit tests in `tests/test_matrix.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/vm.c`
   - `tests/test_matrix.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify transpose elements match $M^T(i, j) == M(j, i)$; verify mutating through view mutates underlying owner buffer; verify GC survival of buffer when owner variable is dropped while view survives; test `matrix_matmul` on rectangular and square matrices.
1. **Zero-Leak Guarantee**: Verify zero memory leaks across owners and views via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______________________________________________________________________

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [NumPy Internal Memory Layout and Strides](https://numpy.org/doc/stable/reference/arrays.ndarray.html#internal-memory-layout-of-an-ndarray): Understanding how row and column strides represent transposed matrices without copying data.
   - [Zero-Copy Networking and Data Processing](https://en.wikipedia.org/wiki/Zero-copy): Principles of sharing underlying storage buffers across multiple non-owning handles.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [The NumPy Array: A Structure for Efficient Numerical Computation](https://ieeexplore.ieee.org/document/5725236): The landmark paper on ndarray architecture, strided indexing, and view ownership.
   - [NumPy ndarray C-API Internal Implementation](https://github.com/numpy/numpy/blob/main/numpy/_core/src/multiarray/arraytypes.c): Inspection of base buffer reference counting and memory sharing across views.
