# Milestone: Page-Aligned PyMalloc with Bitmask Pool Recovery

**ID:** `a929415`\
**Status:** Planned\
**Difficulty:** 4 / 5\
**Focus:** Implement a full CPython-style PyMalloc allocator featuring 4 KB page alignment, $O(1)$ pool header recovery via address bitmasking (`ptr & ~0xFFF`), and arena address boundary verification.\
**Prerequisites:** [Multi-Size-Class Pool Allocator](7ebcf1a_size_class_pool_allocator.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   1. Eliminate the need for sized deallocation (`free(ptr, size)`), making the custom allocator completely transparent to callers: `pymalloc(size)` and `pyfree(ptr)`.
   1. Align pools to 4 KB boundaries using portable ISO C17 allocation (e.g. over-allocating arena buffers to guarantee 4 KB alignment of internal pool arrays).
   1. Implement the famous CPython address-masking trick:
      $$\\text{pool} = (\\text{pool_header_t} \*)((\\text{uintptr_t})\\text{ptr} \\ & \\ \\sim(4096 - 1))$$
   1. Implement Arena Address Range Filtering: maintain an arena registry (or min/max address bounds) to determine in $O(1)$ whether an arbitrary pointer was allocated by PyMalloc or came from system `malloc()`.
   1. Drop-in replacement: wire `pymalloc` and `pyfree` as the primary runtime allocation engine backing `vm_alloc` and `vm_free`.
1. **Scope Boundaries**:
   1. OS-level virtual memory mapping via POSIX `mmap()` or Windows `VirtualAlloc` is deferred to advanced platform modules; portable `malloc()` with alignment offsets is used.
   1. Thread safety and multi-threading locks are non-goals (the VM runtime is single-threaded).

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - 4 KB Aligned Pool Layout:
     ```
     4 KB Boundary (Address: 0x...000)
     +---------------------------------------------------------------+
     | pool_header_t (at exact page base address)                    |
     | - size_class_idx (1 byte)                                     |
     | - block_size (2 bytes)                                        |
     | - free_count, capacity                                        |
     | - freeblock* (intrusive list head)                            |
     | - nextpool*, prevpool*                                        |
     +---------------------------------------------------------------+
     | Block 0 | Block 1 | Block 2 | Block 3 | ...                   |
     +---------------------------------------------------------------+
     4 KB Boundary (Address: 0x...000 + 4096)
     ```
   - Bitmask Address Lookup:
     Given any pointer `ptr = 0x104000840`:
     $$\\text{pool} = \\text{ptr} \\ & \\ \\sim\\text{0xFFF} = \\text{0x104000000}$$
     Directly points to `pool_header_t` in a single CPU cycle with zero header bytes per block!
1. **Core Systems Invariants**:
   1. **Page Alignment Invariant**: Every pool's base address is strictly a multiple of `4096` (`(uintptr_t)pool % 4096 == 0`).
   1. **Arena Provenance Invariant**: Before applying the bitmask, `pyfree(ptr)` verifies that `ptr` falls within an arena's address range `[arena_base, arena_base + ARENA_SIZE)`. If outside, `free(ptr)` is called directly.
   1. **Transparent Free Invariant**: Callers never pass size or pool pointers to `pyfree(ptr)`.
   1. **Header Protection Invariant**: Blocks within a pool start after `sizeof(pool_header_t)`, properly aligned to the size class.
1. **Architectural Trade-offs**:
   1. **Alignment Padding vs. Fast Lookup**: Over-allocating arenas by 4 KB wastes a small amount of memory per arena (at most 4095 bytes), but unlocks instantaneous $O(1)$ pool header recovery without any hash table or tree lookups.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   - Power-of-Two Bitmask Arithmetic: Exploiting binary page boundaries to convert an interior pointer to its container header in a single CPU cycle.
   - Address Space Layout & Memory Alignment: Aligning data structures to hardware page boundaries.
   - Provenance & Bounded Interval Checking: Fast address range checking to distinguish managed arena memory from external system allocations.
1. **Socratic Inquiries**:
   - Why does `(uintptr_t)ptr & ~0xFFF` always point to the start of the 4KB page regardless of which block `ptr` points to?
   - What happens if someone passes an unmanaged stack pointer or a glibc-allocated pointer to `pyfree(ptr)` without the arena boundary check?
   - In CPython, what happens when a pool becomes completely empty (`free_count == capacity`)? Should it be kept in the size class, returned to the arena pool, or given back to the OS?
1. **Failure Modes & Pitfalls**:
   - Misaligned arena base: If an arena is not aligned properly, `ptr & ~0xFFF` will point to the wrong memory, causing catastrophic header corruption.
   - Header clobbering: Carving blocks too close to `pool_header_t` and overwriting header fields when the first block is allocated.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. Create `src/pymalloc.h` and `src/pymalloc.c` declaring `pymalloc()`, `pyfree()`, `pymalloc_init()`, and `pymalloc_destroy()`.
   1. Implement 4 KB-aligned arena allocation with offset tracking.
   1. Implement `address_in_arena(ptr)` range checker.
   1. Implement the address-bitmask pool recovery macro: `#define POOL_OF(ptr) ((pool_header_t *)((uintptr_t)(ptr) & ~(POOL_SIZE - 1)))`.
   1. Implement transparent `pyfree(ptr)`: check arena range, recover pool via `POOL_OF(ptr)`, and push block onto `pool->freeblock`.
   1. Route all VM allocations (`vm_alloc`, `vm_free`) through `pymalloc` / `pyfree`.
1. **File Touchpoints**:
   - `src/pymalloc.h`, `src/pymalloc.c`: Transparent aligned allocator.
   - `src/vm.h`, `src/vm.c`: Integration with virtual machine.
   - `tests/test_pymalloc.c`: Exhaustive alignment, bitmask recovery, and address boundary tests.

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**:
   1. Test bitmask correctness: for 100 random blocks in a pool, verify `POOL_OF(ptr)` resolves to the exact same pool header.
   1. Transparent free verification: allocate 1,000 blocks of mixed sizes (16B, 64B, 256B, 1024B), free all of them via `pyfree(ptr)` without passing sizes, verify no corruption.
   1. External pointer routing: pass a `malloc()`-allocated pointer to `pyfree()` and verify it is recognized as external and freed via system `free()`.
   1. Benchmark comparison: measure allocation throughput of `pymalloc` vs glibc `malloc` under rapid object churn in `bench/bench_gc.cpp`.
1. **Zero-Leak Guarantee**:
   1. Teardown via `pymalloc_destroy()` frees all arenas and unmanaged blocks, confirmed by `assert(boot_all_freed())`.
1. **Tooling Quality Gates**:
   1. `just test` passes with ASan/UBSan.
   1. 100.00% coverage across `src/pymalloc.c`.
   1. `just lint` clean.
1. **Milestone Completion & Lesson Extraction**:
   1. Update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`.
   1. Document the 4 KB bitmask trick and CPython obmalloc architecture in `lessons/` per the `lesson-extraction` skill.
