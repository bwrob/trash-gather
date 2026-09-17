# Milestone: Fixed-Size Object Slab Allocator

**ID:** `e10d642`\
**Status:** Planned\
**Difficulty:** 2 / 5\
**Focus:** Implement a high-throughput fixed-size slab allocator for `object_t` instances, eliminating `malloc` header overhead and heap fragmentation via contiguous 64 KB arenas and intrusive free-list threading.\
**Prerequisites:** [Comprehensive Runtime Source Documentation & Doxygen Annotations](f06ad6f_document_entire_source.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   1. Build a specialized slab allocator for uniform `object_t` instances (`sizeof(object_t)`), bypassing libc `malloc()` per object creation.
   1. Organize memory into contiguous 64 KB Arenas (`slab_arena_t`) allocated from the OS/heap via a single `malloc()` call per arena.
   1. Implement an intrusive singly-linked free list: when an `object_t` slot is freed, cast its memory to store a pointer to the next free slot (`void *next_free`), eliminating per-block metadata overhead.
   1. Support dynamic arena expansion: when an arena's capacity is exhausted, link a new 64 KB arena to the arena chain.
   1. Integrate with the VM lifecycle: `vm_new()` initializes the slab allocator, and `vm_free()` tears down all arenas in a single pass without individual block deallocation overhead.
1. **Scope Boundaries**:
   1. Variable-sized allocations (tuples, strings, list arrays, matrix buffers) continue to use standard `malloc()` in this milestone; multi-size-class pools are deferred to Milestone `7ebcf1a`.
   1. Returning empty arenas back to the OS during runtime is deferred; arenas persist for the duration of the VM process and are freed at shutdown.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - `slab_arena_t` and `object_slab_t` structures:
     ```c
     typedef union Slot {
         object_t object;
         union Slot *next_free;
     } slot_t;

     typedef struct SlabArena {
         struct SlabArena *next;
         size_t capacity;
         size_t allocated_count;
         slot_t slots[]; // C99 flexible array member
     } slab_arena_t;

     typedef struct {
         slab_arena_t *arenas;
         slot_t *free_list;
         size_t total_objects;
         size_t peak_objects;
     } object_slab_t;
     ```
   - Intrusive Free-List Diagram:
     ```
     Arena Memory (64 KB Slabs)
     +---------------------------------------------------------------+
     | Header: next arena*, capacity, allocated_count                |
     +---------------------------------------------------------------+
     | Slot 0: [ ACTIVE object_t (is_marked, refcount, kind, data) ] |
     +---------------------------------------------------------------+
     | Slot 1: [ FREE: *next_free --------------------------------+  |
     +------------------------------------------------------------|--+
     | Slot 2: [ ACTIVE object_t ]                                |  |
     +------------------------------------------------------------|--+
     | Slot 3: [ FREE: *next_free <-------------------------------+  |
     +---------------------------------------------------------------+
     ```
1. **Core Systems Invariants**:
   1. **Intrusive Pointer Alignment & Sizing Invariant**: `sizeof(slot_t) == sizeof(object_t)`, and `sizeof(object_t) >= sizeof(void *)` is verified via `_Static_assert` at compile time.
   1. **Zero Metadata Overhead Invariant**: Every allocated block has 0 bytes of header overhead; its pointer is returned directly to the runtime.
   1. **O(1) Allocation/Free Invariant**: `slab_alloc()` pops the head of `free_list` in $O(1)$; `slab_free()` pushes the returned slot onto the head of `free_list` in $O(1)$.
   1. **Total Arena Teardown Invariant**: Upon `vm_free()`, iterating through `arenas` list and freeing each arena frees all active and inactive `object_t` instances with zero memory leaks (`boot_all_freed()`).
1. **Architectural Trade-offs**:
   1. **Fast Allocation vs. Coarse Reclamation**: Individual objects are recycled immediately via the free list, but whole arena pages are not returned to the OS until the VM exits.
   1. **Single-Purpose vs. General Purpose**: Only handles `object_t`, making it exceptionally simple, fast, and cache-friendly, leaving variable payloads for subsequent tiers.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   - Slab Allocation (Bonwick slab allocator): Dedicating contiguous memory slabs to uniform object types to avoid external fragmentation and metadata tax.
   - Intrusive Data Structures: Embedding list pointers directly inside unused payload memory to achieve zero space overhead for tracking free elements.
   - Cache Locality & Spatial Prefetching: Contiguous object placement ensures sequential scans (e.g. GC sweep and mark) hit warm cache lines.
1. **Socratic Inquiries**:
   - Why is `sizeof(object_t) >= sizeof(void *)` a strict requirement for intrusive free lists? What would happen on a 64-bit architecture if `object_t` were only 4 bytes?
   - In standard `malloc`, freeing an object updates chunk boundary tags. How does our slab allocator know the size of a freed block without any headers?
   - How does contiguous arena storage affect GC mark and sweep throughput compared to pointers scattered randomly across the glibc heap?
1. **Failure Modes & Pitfalls**:
   - Double-free corruption: Freeing the same `object_t` pointer twice introduces a cycle in the `free_list`, causing subsequent allocations to hand out aliased pointers.
   - Pointer provenance violations: Passing a pointer that was not allocated from the slab into `slab_free()`.
   - Dangling pointer dereference: Accessing an object after pushing its slot onto the `free_list`.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. Create `src/slab.h` and `src/slab.c` declaring `object_slab_t` and functions `slab_init()`, `slab_alloc()`, `slab_free()`, `slab_destroy()`.
   1. Implement arena allocation (`malloc(sizeof(slab_arena_t) + capacity * sizeof(slot_t))`) and carve unallocated slots into the initial intrusive free list.
   1. Replace `malloc(sizeof(object_t))` in `src/new.c` with `slab_alloc()`.
   1. Replace `free(obj)` in `src/object.c` (`object_free`) with `slab_free(obj)`.
   1. Hook `slab_init()` into `vm_new()` and `slab_destroy()` into `vm_free()` in `src/vm.c`.
1. **File Touchpoints**:
   - `src/slab.h`: Declare slab types and API.
   - `src/slab.c`: Slab implementation.
   - `src/vm.h`, `src/vm.c`: VM integration.
   - `src/new.c`: Object allocation route.
   - `src/object.c`: Object deallocation route.
   - `tests/test_slab.c`: Adversarial tests for slab capacity, reuse, and leak tracking.

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**:
   1. Allocate $N$ objects, free all of them, verify that re-allocating $N$ objects reuses the exact same pointers in reverse or LIFO order.
   1. Allocate across multiple arena boundaries (e.g. allocating 2,000 objects when an arena holds 1,000) and verify seamless expansion.
   1. Verification under GC cycle collection: ensure `vm_collect_garbage()` recycles unreferenced objects back into the slab's `free_list`.
   1. Allocation failure injection: verify graceful cleanup when `malloc` fails during arena expansion.
1. **Zero-Leak Guarantee**:
   1. Complete teardown via `slab_destroy()` leaves zero bytes allocated, verified by `assert(boot_all_freed())`.
1. **Tooling Quality Gates**:
   1. 100% pass in `just test` with ASan/UBSan.
   1. 100.00% line coverage across `src/slab.c` in `just coverage`.
   1. `just lint` clean.
1. **Milestone Completion & Lesson Extraction**:
   1. Update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`.
   1. Document educational takeaways on slab allocation and intrusive free lists in `lessons/` per the `lesson-extraction` skill.
