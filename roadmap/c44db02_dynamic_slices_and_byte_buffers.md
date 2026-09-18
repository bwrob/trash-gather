# Milestone: Non-Owning Sequence Slices & Sub-Views (`slice_t`)

**ID:** `c44db02`\
**Status:** Planned\
**Difficulty:** 3 / 5\
**Focus:** Implement non-owning container sub-views (`slice_t`) over lists, tuples, and byte buffers, mastering zero-copy sub-windowing, offset translation, and garbage collection base reference retention.\
**Prerequisites:** [Raw Byte Buffer Object (bytes_t)](52fb556_raw_byte_buffer.md), [Python-Style Sequence Negative Indexing](b0c1d8b_python_sequence_negative_indexing.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Introduce `SLICE` as a first-class `object_kind_t` backed by an embedded `slice_t` payload in `object_data_t`.
   - Implement `new_slice(object_t *source, int64_t start, int64_t stop, int64_t step)` supporting sequence types (`LIST`, `TUPLE`, `BYTES`).
   - Implement zero-copy read indexing `slice_get(object_t *slice, int64_t index)` mapping to the corresponding element in the underlying container.
   - Enforce base container retention: `slice_t` increments `refcount_inc(source)` and marks `source` during GC cycle tracing, guaranteeing the backing buffer remains alive while the slice is reachable.
   - Hook into `object_len(slice)` returning the logical slice length calculated via standard Python slice arithmetic: $\\max(0, \\lceil(\\text{stop} - \\text{start}) / \\text{step}\\rceil)$.
1. **Scope Boundaries**:
   - In-place slice assignments (`seq[a:b] = replacement`) with buffer shifts are deferred to advanced container milestones.
   - Multidimensional tensor slicing is deferred to numerical computing milestones.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Non-owning slice structure:
     ```c
     typedef struct {
         object_t *source; // Underlying sequence container (LIST, TUPLE, BYTES)
         size_t start;     // Normalized start index
         size_t stop;      // Normalized stop index
         int64_t step;     // Stride step (typically 1)
         size_t length;    // Precomputed element count
     } slice_t;
     ```
   - Slice Reference Retention Graph:
     ```
     Root Variable: my_slice
     +-----------------------------------------+
     | object_t                                |
     | kind: SLICE                             |
     | data.v_slice:                           |
     |   source: points to my_list (incref'd)  |
     |   start: 2, stop: 5, step: 1, length: 3 |
     +-------------------|---------------------+
                         |
                         v
     +-----------------------------------------+
     | object_t (LIST: [0, 1, 2, 3, 4, 5, 6])  | (Kept alive by slice!)
     +-----------------------------------------+
     ```
1. **Core Systems Invariants**:
   - Backing container retention: While a `slice_t` is reachable, its `source` container is guaranteed to remain valid and alive in memory. Dropping the variable holding the source does not cause a use-after-free in the slice.
   - Read-only zero-copy: Creating a slice performs zero element copies, requiring only $O(1)$ time and memory.
   - Bounds mapping invariant: For any $0 \\le i < \\text{length}$, the mapped offset $\\text{start} + i \\times \\text{step}$ is guaranteed to fall strictly within the bounds of `source`.
1. **Architectural Trade-offs**: Slices provide instant sub-view operations without duplicating huge buffers, but can cause retained memory leaks if a small 1-element slice keeps a 100 MB byte buffer or list alive in the GC.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Non-owning views and borrowed references in systems programming (Rust `&[T]`, C++ `std::string_view`, Python memoryview/slice); interior offset mapping; object retention graphs in tracing collectors.
1. **Socratic Inquiries**:
   - Why does Python's `list[1:4]` copy elements into a new list, while Python's `memoryview` creates a non-owning zero-copy view? What are the safety trade-offs of both approaches?
   - How can you safely normalize negative indices and bounds when `step < 0` (reverse slicing)?
   - What happens during GC mark phase if `slice->source` is not traced?
1. **Failure Modes & Pitfalls**: Dangling pointer if `source` is freed while the slice survives; integer division by zero if `step == 0`; subtle off-by-one errors when computing slice lengths.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Define `slice_t` in `src/object.h`.
   - Add `SLICE` to `object_kind_t` and `slice_t v_slice;` to `object_data_t` in `src/object.h`.
   - Implement slice length and index normalization helpers in `src/object.c`.
   - Implement `new_slice()` in `src/new.c`.
   - Implement `slice_get()` in `src/object.c`.
   - Update `object_decref_children` and `object_free_payload` in `src/object.c` to decref `slice->source`.
   - Update `trace_blacken_object` in `src/vm.c` to trace `slice->source`.
   - Add comprehensive unit tests in `tests/test_slice.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/new.h`, `src/new.c`
   - `src/vm.c`
   - `tests/test_slice.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify slice creation over lists, tuples, and byte buffers; test forward and step > 1 slicing; test dropped source retaining memory safely under GC cycles; test out-of-bounds index rejection.
1. **Zero-Leak Guarantee**: Verify full cleanup without memory leaks via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______________________________________________________________________

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Python Slice Objects and Indices Resolution](https://docs.python.org/3/c-api/slice.html): The C-API specification for evaluating `[start:stop:step]` slice parameters.
   - [Non-Owning String Views and Buffer Windows](https://en.wikipedia.org/wiki/String_view): Borrowed references, interior offset calculations, and lifetime bounds in systems languages.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [PEP 3118 – Revising the Buffer Protocol](https://peps.python.org/pep-3118/): How Python standardizes zero-copy buffer sharing between strings, bytes, and external views.
   - [CPython Objects/sliceobject.c Implementation](https://github.com/python/cpython/blob/main/Objects/sliceobject.c): Production mechanics of slice instantiation, bounds clamping, and length evaluation.
