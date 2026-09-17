# Milestone: Multi-Size-Class Pool Allocator

**ID:** `7ebcf1a`\
**Status:** Planned\
**Difficulty:** 3 / 5\
**Focus:** Implement a multi-size-class pool allocator (PyMalloc Lite) that categorizes small allocations (16–256 bytes) into discrete size classes and dedicated 4 KB pools, falling back to system malloc for larger requests.\
**Prerequisites:** [Fixed-Size Object Slab Allocator](e10d642_object_slab_allocator.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   1. Expand beyond uniform objects to support arbitrary small-sized allocations (16 to 256 bytes), serving variable payloads like small tuples, string buffers, and list element arrays.
   1. Establish discrete size classes in powers of 2 or 16-byte steps: 16, 32, 48, 64, 96, 128, 192, 256 bytes.
   1. Divide 64 KB arenas into 4 KB pools, where each pool is assigned dynamically to a single size class on demand.
   1. Implement sized allocation and deallocation: `pool_alloc(size)` and `pool_free(ptr, size)`.
   1. Fallback mechanism: any allocation request $> 256$ bytes delegates directly to system `malloc()` and `free()`.
1. **Scope Boundaries**:
   1. Address-based bitmask pool header recovery without passing size to `free()` is deferred to Milestone `a929415`. Sized deallocation `pool_free(ptr, size)` is explicitly used in this step.
   1. Cross-arena pool migration and memory compaction are non-goals.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Data structures:
     ```c
     #define POOL_SIZE 4096
     #define ARENA_SIZE 65536
     #define NUM_SIZE_CLASSES 8

     typedef struct PoolHeader {
         uint8_t size_class_idx;
         uint16_t block_size;
         uint16_t capacity;
         uint16_t free_count;
         void *free_list;
         struct PoolHeader *next;
         struct PoolHeader *prev;
     } pool_header_t;

     typedef struct {
         pool_header_t *active_pools[NUM_SIZE_CLASSES];
         slab_arena_t *arenas;
     } pool_allocator_t;
     ```
   - Pool Hierarchy Diagram:
     ```
     Arena (64 KB)
     +---------------------------------------------------------------+
     | Pool 0 (4 KB)  | Dedicated to 32-byte blocks                  |
     | Pool 1 (4 KB)  | Dedicated to 64-byte blocks                  |
     | Pool 2 (4 KB)  | Dedicated to 16-byte blocks                  |
     | ...            | Unassigned / free pools                      |
     +---------------------------------------------------------------+
     ```
1. **Core Systems Invariants**:
   1. **Size-Class Rounding Invariant**: Any requested size $S \\le 256$ is deterministically mapped to the smallest size class $C \\ge S$.
   1. **Intrusive Block Invariant**: Free blocks within a pool store a pointer to the next free block in their own unallocated memory (`*(void **)block = pool->free_list`).
   1. **Fallback Invariant**: Any allocation with $S > 256$ bypasses the pool allocator and calls `malloc(S)`, and is freed via `free(ptr)`.
   1. **Doubly-Linked Active Pool Invariant**: Only pools with at least one free block remain on `active_pools[class_idx]`. When a pool becomes full (`free_count == 0`), it is unlinked from the active list until a block is freed.
1. **Architectural Trade-offs**:
   1. **Internal Fragmentation vs. Memory Efficiency**: Small objects round up to the nearest size class (e.g. 18 bytes uses a 32-byte block), but completely eliminate the 16-byte `malloc` header tax and external fragmentation.
   1. **Sized Deallocation**: Requiring `size` on `pool_free(ptr, size)` avoids complex alignment logic in this intermediate step while delivering full multi-pool performance.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   - Segregated-Storage / Size-Class Allocation: Grouping memory blocks of identical sizes into dedicated pools to eliminate external fragmentation.
   - High-water mark bump allocation within fresh pools before threading into the free list.
   - Internal vs. External Fragmentation trade-offs in runtime allocators.
1. **Socratic Inquiries**:
   - If an allocation request is for 17 bytes and the nearest size classes are 16 and 32 bytes, why is rounding up to 32 bytes still more memory efficient than calling glibc `malloc(17)`?
   - Why do we unlink a pool from `active_pools` once its `free_count` reaches 0? What would be the performance penalty if we didn't?
   - How does sized deallocation simplify the allocator compared to an allocator that takes only `void *ptr`?
1. **Failure Modes & Pitfalls**:
   - Size mismatch on free: Passing the wrong size to `pool_free(ptr, size)`, causing the block to be inserted into the free list of the wrong size class.
   - Pool exhaustion without arena expansion: Failing to allocate a new arena when all current pools are occupied.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. Create `src/pool.h` and `src/pool.c` defining size classes, `pool_header_t`, and `pool_allocator_t`.
   1. Implement size class mapping function `size_to_class(size_t size)`.
   1. Implement pool carving from 64 KB arenas and initial block bump allocation.
   1. Implement `pool_alloc(size)` and `pool_free(ptr, size)` with $> 256$ byte fallback.
   1. Wire `tuple_t`, `list_t`, and string payload allocations in `src/new.c` and `src/object.c` through the pool allocator.
1. **File Touchpoints**:
   - `src/pool.h`, `src/pool.c`: Multi-size-class pool allocator implementation.
   - `src/new.c`, `src/object.c`: Variable payload allocation routing.
   - `tests/test_pool.c`: Unit and stress tests across all size classes.

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**:
   1. Allocate and free blocks across all discrete size classes (16, 32, 64, ..., 256) verifying correct size class routing.
   1. Full-pool exhaustion and replenishment: allocate enough blocks to fill a 4 KB pool, verify unlinking, allocate one more to trigger a new pool, free a block in the first pool, verify relinking.
   1. Fallback verification: allocate 512 bytes and 4096 bytes, verifying proper system `malloc` routing and zero leaks.
1. **Zero-Leak Guarantee**:
   1. All arenas and system-allocated fallback blocks confirmed freed via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**:
   1. `just test` passes with ASan/UBSan.
   1. 100.00% coverage across `src/pool.c`.
   1. `just lint` passes.
1. **Milestone Completion & Lesson Extraction**:
   1. Update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`.
   1. Document educational takeaways on segregated size-class pool design in `lessons/` per the `lesson-extraction` skill.
