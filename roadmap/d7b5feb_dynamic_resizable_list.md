# Milestone: Dynamic Resizable List Mutations

**ID:** `d7b5feb`\
**Status:** Planned\
**Difficulty:** 1 / 5\
**Focus:** Transform `list_t` from a fixed-size buffer into a dynamically resizable sequence supporting `list_append()`, `list_insert()`, and `list_pop()` with amortized $O(1)$ geometric growth and overlapping memory shifts via `memmove`.\
**Prerequisites:** [Polymorphic Sequence Length Protocol](b81f9a7_polymorphic_sequence_length.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Upgrade `list_t` to track both `size` (logical element count) and `capacity` (allocated slot count).
   - Implement `list_append(object_t *list, object_t *item)` with geometric reallocation ($1.5\\times$ or $2\\times$).
   - Implement `list_insert(object_t *list, int64_t index, object_t *item)` using `memmove` to safely shift trailing pointer elements rightward.
   - Implement `list_pop(object_t *list, int64_t index)` using `memmove` to shift trailing pointer elements leftward and return the removed element.
1. **Scope Boundaries**:
   - Dynamic slicing and sub-views are deferred to `c44db02_dynamic_slices_and_byte_buffers.md`.
   - In-place sorting is deferred to `3f1132d_rich_comparisons_and_sorting.md`.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Resizable `list_t` memory structure:

     ```text
     +-----------------------------------------------+
     | list_t                                        |
     |  size_t size        (e.g., 3)                 |
     |  size_t capacity    (e.g., 8)                 |
     |  object_t **elements ---------------------\   |
     +-------------------------------------------+---+
                                                 |
         +---------------------------------------+
         v
     +---+---+---+---+---+---+---+---+
     | 0 | 1 | 2 | x | x | x | x | x |  (capacity = 8, 5 spare slots)
     +---+---+---+---+---+---+---+---+
       |   |   |
       v   v   v
      [objects]
     ```

1. **Core Systems Invariants**:
   - Capacity invariant: $0 \\le \\text{size} \\le \\text{capacity}$ must hold after every mutation.
   - Pointer safety: Reallocation must use `boot_realloc` (or `boot_malloc` + `memcpy` + `boot_free`). If allocation fails, the original elements buffer must remain untouched and un-leaked.
   - Ownership transfer: Appending or inserting an object into a list transfers ownership by incrementing its reference count (`refcount_inc`). Popping an object retains its ownership for the caller without decrementing until the caller drops it.
   - Overlap safety: Element shifting on insertion and deletion must strictly use `memmove()`, never `memcpy()`, because source and destination ranges overlap.
1. **Architectural Trade-offs**: Pre-allocating capacity trades a small amount of memory overhead for $O(1)$ amortized append performance, avoiding costly heap reallocation on every single insertion.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Amortized complexity analysis; dynamic array geometric growth factors; memory overlapping and the formal distinction between `memcpy` and `memmove` in ISO C17.
1. **Socratic Inquiries**:
   - Why does ISO C state that calling `memcpy(dest, src, n)` produces Undefined Behavior when the regions `[dest, dest+n)` and `[src, src+n)` overlap, whereas `memmove` is guaranteed to be safe?
   - When expanding a dynamic array, why is geometric growth (e.g. $C\_{\\text{new}} = C\_{\\text{old}} \\times 2$) amortized $O(1)$, whereas linear growth ($C\_{\\text{new}} = C\_{\\text{old}} + 1$) is $O(N^2)$ for $N$ appends?
   - If `realloc()` fails and returns `NULL`, what happens to the existing pointer if you write `list->elements = realloc(list->elements, new_cap)`?
1. **Failure Modes & Pitfalls**: Storing `realloc` result directly into `list->elements` causing instant memory leaks on allocation failure; off-by-one errors in `memmove` byte count calculations (`count * sizeof(object_t*)`); forgetting `refcount_inc` on appended items leading to premature reclamation.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Update `list_t` in `src/object.h` to add `size_t capacity;`.
   - Update `new_list(size_t size)` in `src/new.c` to initialize `capacity >= size`.
   - Implement geometric growth helper `static bool list_ensure_capacity(list_t *list, size_t min_capacity)` in `src/object.c`.
   - Implement `bool list_append(object_t *list, object_t *item);` in `src/object.h` and `src/object.c`.
   - Implement `bool list_insert(object_t *list, int64_t index, object_t *item);` supporting negative offsets and `memmove` shifts.
   - Implement `object_t *list_pop(object_t *list, int64_t index);` removing an element, shifting subsequent elements, and returning the item.
   - Write unit and stress tests in `tests/test_object.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/new.c`
   - `tests/test_object.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify appends from empty list up to 1,000 elements; test mid-list insertion and deletion with exact order checks; simulate allocation failure during capacity growth; test invalid index popping.
1. **Zero-Leak Guarantee**: Verify full deallocation of grown dynamic lists via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Dynamic Array Amortized Complexity Analysis](https://en.wikipedia.org/wiki/Dynamic_array): Mathematical proof of amortized O(1) appends under geometric capacity scaling.
   - [ISO C realloc and memmove Semantics](https://en.cppreference.com/w/c/memory/realloc): Safe memory reallocation patterns and using memmove for overlapping buffer shifts.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython listobject.c Growth Factor Formula](https://github.com/python/cpython/blob/main/Objects/listobject.c): Analysis of Python's over-allocation formula `new_allocated = (size_t)newsize + (newsize >> 3) + (newsize < 9 ? 3 : 6)`.
   - [SEI CERT C MEM30-C: Avoid Using Stale Pointers](https://wiki.sei.cmu.edu/confluence/display/c/MEM30-C.+Do+not+access+freed+memory): Handling pointer invalidation hazards when realloc moves heap memory buffers.
