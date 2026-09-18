# Milestone: Single-Arena Object Slab Allocator

**ID:** `e10d642`\
**Status:** Planned\
**Difficulty:** 2 / 5\
**Focus:** Build a standalone, fixed 64 KB slab arena with intrusive free-list slot threading for `object_t` allocations, eliminating `malloc` header overhead and mastering memory slot reuse without system deallocations.\
**Prerequisites:** [Object Header Bitflags & Memory Layout](c87f151_object_header_bitflags.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Define a fixed 64 KB slab arena structure (`slab_arena_t`) and slot union (`union Slot { object_t object; union Slot *next_free; }`).
   - Implement standalone arena initialization: allocate a single 64 KB memory block from `boot_malloc()` and thread an intrusive singly-linked free list through all slots.
   - Implement `slab_alloc(slab_arena_t *arena)` yielding a recycled `object_t*` in $O(1)$ time by popping the free list head.
   - Implement `slab_free(slab_arena_t *arena, object_t *obj)` returning a slot to the free list in $O(1)$ time by pushing to the free list head.
   - Implement arena teardown (`slab_arena_free(slab_arena_t *arena)`), deallocating the entire 64 KB arena in a single `boot_free()` call.
1. **Scope Boundaries**:
   - Multi-arena dynamic expansion on exhaustion is deferred to Milestone `f682854_multi_arena_slab_chaining.md`.
   - Global VM allocator redirection (`new_object()` integration) is deferred to Milestone `f682854_multi_arena_slab_chaining.md`.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Slot union and Single Slab Arena:
     ```c
     typedef union Slot {
         object_t object;
         union Slot *next_free;
     } slot_t;

     typedef struct SlabArena {
         size_t capacity;       // Total slots in 64 KB buffer
         size_t allocated_count; // Currently leased active objects
         slot_t *free_list;     // Head of singly-linked free slots
         slot_t slots[];        // C99 flexible array member
     } slab_arena_t;
     ```
   - Intrusive free-list reuse diagram:
     ```
     Arena: [Header: free_list -> Slot 1]
     Slot 0: [ ACTIVE object_t ]
     Slot 1: [ FREE: next_free -> Slot 3 ]
     Slot 2: [ ACTIVE object_t ]
     Slot 3: [ FREE: next_free -> NULL ]
     ```
1. **Core Systems Invariants**:
   - Zero-metadata free slot invariant: When a slot is inactive, its memory stores exclusively `next_free` pointer bytes. Active objects never contain intrusive pointers.
   - Capacity bounds: Leased slots must satisfy $0 \\le \\text{allocated_count} \\le \\text{capacity}$. When $\\text{allocated_count} == \\text{capacity}$, `free_list == NULL` and `slab_alloc` returns `NULL`.
   - Single-allocation arena invariant: All slots reside contiguously within the 64 KB arena memory block, requiring zero per-slot `malloc` or `free` calls.
1. **Architectural Trade-offs**: Fixed-size slabs eliminate the 8-to-16 byte glibc malloc header overhead per object and eliminate external heap fragmentation, at the cost of supporting only uniform fixed-size allocations (`sizeof(object_t)`).

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Intrusive data structures; free-list threading through dead memory; memory fragmentation (internal vs external); fixed-size block allocation ($O(1)$ time).
1. **Socratic Inquiries**:
   - Why can an inactive `object_t` slot safely store a `next_free` pointer inside its own memory without allocating extra metadata?
   - What is the memory footprint of an intrusive free list when all objects are currently in use?
   - What happens if a caller calls `slab_free` with a pointer that does not belong to the arena's memory bounds?
1. **Failure Modes & Pitfalls**: Double-freeing a slot corrupting the intrusive free-list into a circular loop; dereferencing `free_list` when the arena is exhausted; buffer overruns if `sizeof(slot_t)` is calculated incorrectly.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Create `src/slab.h` declaring `slot_t`, `slab_arena_t`, `slab_arena_new`, `slab_arena_free`, `slab_alloc`, and `slab_free`.
   - Create `src/slab.c` implementing arena initialization, intrusive free-list linking, allocation, and deallocation.
   - Implement pointer boundary validation helper `static inline bool arena_contains(slab_arena_t *arena, void *ptr)`.
   - Write comprehensive unit tests in `tests/test_slab.c` verifying sequential allocations, free-list recycling, and exhaustion handling.
1. **File Touchpoints**:
   - `src/slab.h`, `src/slab.c`
   - `tests/test_slab.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify allocating all slots until exhaustion; verify free-list recycling by allocating, freeing, and re-allocating; verify out-of-bounds pointer rejection in debug builds.
1. **Zero-Leak Guarantee**: Full teardown via `slab_arena_free()` leaves zero memory leaks under `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______________________________________________________________________

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [The Slab Allocator: An Object-Caching Kernel Memory Allocator (Bonwick)](https://people.eecs.berkeley.edu/~kubitron/cs262/handouts/papers/bonwick.pdf): The seminal paper introducing fixed-size slab memory caching and eliminating malloc fragmentation.
   - [Intrusive Data Structures and Embedded Free Lists](https://www.data-structures-in-practice.com/intrusive-linked-lists/): Threading singly-linked free pointers through inactive memory blocks without metadata overhead.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [Linux Kernel SLAB/SLUB Memory Allocator](https://www.kernel.org/doc/gorman/html/understand/understand011.html): How the Linux kernel implements slab caches for uniform kernel objects.
   - [CPython Objects/obmalloc.c Pool Architecture](https://github.com/python/cpython/blob/main/Objects/obmalloc.c): How Python manages fixed-size allocation pools and intrusive free-list slot recycling.
