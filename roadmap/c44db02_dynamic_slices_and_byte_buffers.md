# Milestone: Dynamic Slices & Byte Buffers (`slice_t`)

**ID:** `c44db02`\
**Status:** Planned\
**Focus:** Implement non-owning container sub-views, resizable raw byte buffers, and investigate interior pointer reference tracking in the garbage collector.\
**Prerequisites:** [Full CPython-Style Offset-0 Hierarchy](222f6ce_cpython_offset0_hierarchy.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Implement a resizable raw byte buffer `byte_buffer_t` and non-owning slices `slice_t` representing a sub-window `[offset, offset + length)` over an underlying sequence object (`list_t`, `tuple_t`, `byte_buffer_t`).
1. **Scope Boundaries**: Strided slices and multi-dimensional tensor sub-views are deferred to numerical runtime milestones.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Slice and byte buffer structures:
     ```c
     typedef struct {
       object_t *source;
       size_t offset;
       size_t length;
     } slice_t;

     typedef struct {
       size_t size;
       size_t capacity;
       uint8_t *data;
     } byte_buffer_t;
     ```
1. **Core Systems Invariants**:
   - Backing container retention invariant: As long as a `slice_t` object is reachable from a root, `slice->source` is reachable and marked.
   - Bounds invariant: Accessing elements through the slice strictly enforces `index < length`, mapping to `offset + index` on the underlying source.
   - Reallocation resilience: Storing an `offset` instead of a raw interior pointer prevents pointer invalidation if `byte_buffer_t` resizes via `realloc()`.
1. **Architectural Trade-offs**: Slices enable zero-copy reads without duplicating buffer memory, but hold the entire backing buffer alive, potentially causing large memory retention for small sub-views.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Non-owning borrowed references; interior pointer safety; buffer over-allocation and geometric growth amortized complexity.
1. **Socratic Inquiries**:
   - What is the difference between an offset-based slice and a raw interior pointer in C? Why are true interior pointers much harder for GCs?
   - What happens to slices if the underlying `byte_buffer_t` reallocates?
   - What memory retention hazard exists when a small 10-byte slice references a 100MB buffer?
1. **Failure Modes & Pitfalls**: Out-of-bounds reads; dangling buffer pointers after reallocations; silent retention of massive backing buffers.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Define `byte_buffer_t` and `slice_t` in `src/object.h`.
   - Implement `new_byte_buffer()` and `new_slice()` in `src/new.c` and `src/new.h`.
   - Implement `slice_get()` and `byte_buffer_append()` in `src/object.c`.
   - Integrate `SLICE` and `BYTE_BUFFER` into `trace_blacken_object()` in `src/vm.c`.
   - Integrate decref and payload reclamation in `src/object.c`.
   - Write unit tests in `tests/test_slice.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/new.h`, `src/new.c`
   - `src/vm.c`
   - `tests/test_slice.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify zero-copy slicing over lists, tuples, and byte buffers with boundary edge tests.
1. **Zero-Leak Guarantee**: Retention test confirms backing buffer survives while slice is rooted, and is fully reclaimed when slice is unrooted with `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly.
