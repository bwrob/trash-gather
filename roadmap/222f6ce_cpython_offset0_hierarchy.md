# Milestone: Full CPython-Style Offset-0 Hierarchy

**ID:** `222f6ce`\
**Status:** Planned\
**Focus:** Eliminate the tagged union by adopting offset-0 base header embedding (`PyObject` style), achieving single-allocation objects and eliminating union memory bloat.\
**Prerequisites:** [Comprehensive Runtime Source Documentation & Doxygen Annotations](f06ad6f_document_entire_source.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Replace the tagged union architecture with CPython-style offset-0 struct inheritance, unifying headers and flexible item payloads into a single contiguous allocation per object.
1. **Scope Boundaries**: Associative containers and hash tables are deferred to Milestone 09.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Base header struct:
     ```c
     typedef struct Object {
       object_kind_t kind;
       bool is_marked;
       size_t tracker_id;
       size_t refcount;
     } object_t;
     ```
   - Concrete variable-sized tuple struct:
     ```c
     typedef struct {
       object_t base;
       size_t size;
       object_t *items[];
     } tuple_object_t;
     ```
1. **Core Systems Invariants**:
   - Offset-0 guarantee (C99 §6.7.2.1): A pointer to any concrete object (`tuple_object_t *`, `int_object_t *`) can be safely cast to `object_t *` without pointer arithmetic.
   - Internal bloat elimination: Primitive objects allocate only their required fields (~32 bytes for integers vs ~96 bytes in tagged unions).
1. **Architectural Trade-offs**: Variable allocation sizes complicate memory management and can induce heap fragmentation, but reduce total heap memory consumption by 60–70% for primitive-dense workloads.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: C99 struct pointer casting; union bloat vs external fragmentation; single vs multiple heap allocations.
1. **Socratic Inquiries**:
   - How does dynamic polymorphism work without a tagged union when `kind` remains at offset `0`?
   - What allocator strategy (e.g. CPython's `obmalloc` size-class pools) is needed when objects vary in size?
1. **Failure Modes & Pitfalls**: Unaligned struct accesses; incorrect pointer casts; allocator external fragmentation.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Redefine `object_t` as a base metadata header in `src/object.h`.
   - Define concrete per-type structs (`int_object_t`, `tuple_object_t`, etc.) in `src/object.h`.
   - Update `new_object()` in `src/new.c` to accept `size_t total_bytes`.
   - Update constructors to allocate unified memory blocks.
   - Update GC traversal and deallocation in `src/vm.c` and `src/object.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/new.h`, `src/new.c`
   - `src/vm.h`, `src/vm.c`
   - `tests/`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Run the full suite with polymorphic casts across all object types.
1. **Zero-Leak Guarantee**: `assert(boot_all_freed())` verifies zero leaks with single-allocation objects.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly.
