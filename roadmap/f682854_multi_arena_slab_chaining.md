# Milestone: Multi-Arena Dynamic Chaining & VM Runtime Integration

**ID:** `f682854`\
**Status:** Planned\
**Difficulty:** 3 / 5\
**Focus:** Expand the single-arena slab allocator into a dynamic multi-arena chain (`object_slab_t`), integrate it directly into `vm_new()` / `vm_free()`, and redirect runtime object allocations away from libc `malloc`.\
**Prerequisites:** [Single-Arena Object Slab Allocator](e10d642_object_slab_allocator.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Create a multi-arena coordinator struct `object_slab_t` holding a singly-linked list of `slab_arena_t` blocks.
   - Implement dynamic arena allocation: when the active arena free list is exhausted, allocate a new 64 KB arena, link it to the arena chain, and continue servicing allocations seamlessly.
   - Embed `object_slab_t` into `vm_t` in `src/vm.h`.
   - Update `new_object()` in `src/new.c` to allocate `object_t` instances from the VM slab allocator instead of `boot_malloc()`.
   - Update `object_free()` and GC `sweep()` to return dead `object_t` slots to the slab free list.
   - Update `vm_free()` to deallocate all chained 64 KB arenas in a clean loop.
1. **Scope Boundaries**:
   - Multi-size-class pools for variable-sized payloads (strings, tuples, lists) are deferred to Milestone `7ebcf1a_size_class_pool_allocator.md`.
   - Returning empty arenas to the OS during process execution is deferred to advanced allocator optimizations.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Multi-Arena Chained Layout:

     ```text
     vm_t -> object_slab_t
               |
               +---> arena_head ---> arena_2 ---> arena_1 ---> NULL
                       | (64 KB)       | (64 KB)    | (64 KB)
                       v               v            v
                     [slots]         [slots]      [slots]
     ```

1. **Core Systems Invariants**:
   - Global arena ownership invariant: All active and inactive arenas belong to `vm->slab`. No individual object slot calls `boot_free()`.
   - Fast-path allocation: If the current free list is non-empty, allocation takes $O(1)$ time without system calls. New arena allocation only occurs on exhaustion.
   - Teardown safety: Calling `vm_free()` frees all chained arenas in order without leaving dangling pointers or memory leaks.
1. **Architectural Trade-offs**: Dynamic arena chaining provides virtually unlimited object capacity without preallocating the entire heap upfront, trading a tiny metadata pointer (`next_arena`) per 64 KB block.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Amortized allocation latency; arena allocator chains; pointer ownership segregation between container payloads and header objects.
1. **Socratic Inquiries**:
   - Why is freeing an entire arena in a single pass at VM shutdown orders of magnitude faster than freeing 50,000 individual `object_t` instances with libc `free()`?
   - What happens to the variable payloads (e.g. `tuple->elements` or `string->v_string`) when `object_t` itself is recycled in a slab?
   - How can you determine which arena an object slot belongs to when freeing?
1. **Failure Modes & Pitfalls**: Forgetting to free payloads (`object_free_payload`) before returning an `object_t` to the slab free-list; memory leaks when unlinking arenas; use-after-free if an arena is destroyed while objects are still referenced.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Extend `src/slab.h` with `object_slab_t` and multi-arena functions (`slab_init`, `slab_destroy`, `slab_alloc_object`, `slab_free_object`).
   - Implement dynamic chaining in `src/slab.c`: allocate new arenas on exhaustion and prepend to the arena list.
   - Embed `object_slab_t slab;` into `vm_t` in `src/vm.h`.
   - Update `vm_new()` to call `slab_init()` and `vm_free()` to call `slab_destroy()`.
   - Replace `boot_malloc(sizeof(object_t))` in `src/new.c` with `slab_alloc_object(&CURRENT_VM->slab)`.
   - Update `sweep()` in `src/vm.c` to return swept slots via `slab_free_object()`.
   - Add unit tests in `tests/test_slab.c` testing dynamic expansion across 3+ arenas (> 4,000 objects).
1. **File Touchpoints**:
   - `src/slab.h`, `src/slab.c`
   - `src/vm.h`, `src/vm.c`
   - `src/new.c`
   - `tests/test_slab.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Allocate thousands of objects exceeding single-arena capacity; verify seamless expansion; run full GC cycle collections under the slab allocator; verify zero leaks on VM teardown.
1. **Zero-Leak Guarantee**: Full VM teardown with multiple chained arenas verifies zero memory leaks via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Region-Based Memory Management and Arenas](https://en.wikipedia.org/wiki/Region-based_memory_management): Structuring memory into fixed-size blocks chained dynamically on demand.
   - [Untangling Lifetimes: The Arena Allocator](https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator): Practical systems guide to building high-performance chained arena allocators.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython Objects/obmalloc.c Arena Management](https://github.com/python/cpython/blob/main/Objects/obmalloc.c): How Python chains 256 KB arenas and manages free-pool lists across runtime lifecycles.
   - [jemalloc Architecture and Design](http://jemalloc.net/): Production multi-arena techniques for scalable, thread-aware systems memory allocation.
