# Milestone: Fallible Stack and Frame Allocation Hardening

**ID:** `509705c`\
**Status:** Planned\
**Difficulty:** 1 / 5\
**Focus:** Harden the VM stack, object tracking, and execution frame allocators against memory exhaustion, ensuring fallible push operations and frame creations propagate errors without memory leaks or premature garbage collection sweeps.\
**Prerequisites:** [The `None` Immortal Singleton Object](fc1cc81_none_immortal_singleton.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   1. Check all fallible container operations (`stack_push`, `stack_new`, `malloc`, `realloc`) across VM runtime lifecycles.
   2. Harden `vm_track_object` so failure to append to `vm->objects` aborts object creation or triggers a safe rollback invariant ($\mathcal{I}_{\text{rollback}}$) instead of creating untracked "ghost" objects with corrupted `tracker_id`.
   3. Propagate allocation failure out of `vm_new_frame` and `frame_reference_object` so reference count increments and frame registrations stay strictly symmetrical without orphaned increments.
   4. Harden `trace_mark_object` so failure to push to `gray_objects` cannot mark an object `is_marked = true` while dropping its children from traversal (preventing reachable object reclamation).
   5. Fix `stack_push` capacity growth logic: eliminate integer truncation (`size_t` to `int`), handle initial zero-capacity allocations (`capacity == 0`), and guard against unsigned multiplication overflow.
   6. Make `vm_new()` return a status boolean (or `vm_t *`) rather than `void` so VM initialization failures are detectable, and protect `CURRENT_VM` against re-entrant overwrite leaks.
2. **Scope Boundaries**:
   1. Dynamic resizable lists (`list_t`) resizing policies are handled in `d7b5feb_dynamic_resizable_list.md`.
   2. Slab allocators and arena pools are deferred to `e10d642_object_slab_allocator.md`.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Stack dynamic array resizing:

     ```text
     Initial / Empty:
     stack->capacity = 0 | 8
     stack->data     = NULL | void*[capacity]

     Growth step (k -> k+1):
     new_capacity    = capacity == 0 ? 8 : capacity * 2
     new_data        = realloc(stack->data, new_capacity * sizeof(void *))
     ```

   - Fallible call graph:

     ```text
     object_new()       -> vm_track_object()       -> stack_push(vm->objects) [Must succeed or unwind]
     frame_reference()  -> stack_push(frame->refs) -> object_refcount_inc()    [Symmetrical on success only]
     trace_mark()       -> stack_push(gray_stack)  -> obj->is_marked = true   [Atomic with gray stack push]
     ```

2. **Core Systems Invariants**:
   - **Tracked Invariant ($\mathcal{I}_{\text{track}}$)**: Every live mortal `object_t` registered in the runtime must have a valid `tracker_id` corresponding to `vm->objects->data[obj->tracker_id] == obj`. An untracked mortal object must never be returned to caller code.
   - **Tri-Color Invariant ($\mathcal{I}_{\text{color}}$)**: An object marked `is_marked = true` must either reside on the `gray_objects` stack or have had all its referenced children traversed and marked (black). It can never be marked black if pushing to `gray_objects` fails.
   - **Refcount Symmetrical Ownership ($\mathcal{I}_{\text{ref}}$)**: If `frame_reference_object` fails to register the reference in `frame->references`, the object refcount must not remain incremented.
   - **Rollback Invariant ($\mathcal{I}_{\text{rollback}}$)**: If `object_new` fails during `vm_track_object`, the newly allocated `object_t` header must be immediately freed and `NULL` returned.
3. **Architectural Trade-offs**:
   - Propagating boolean status up from leaf functions requires return checks at every caller site, slightly increasing control flow branches in exchange for eliminating silent heap corruption.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   - Tri-color garbage collection invariants (Dijkstra et al.): No black object may reference a white object unless an intermediate gray object exists. Marking an object without queuing it on the gray stack silently breaks the invariant.
   - Multi-stage allocation rollback and failure atomicity.
   - Fallible memory contracts in C systems programming (SEI CERT MEM32-C, MEM34-C).
2. **Socratic Inquiries**:
   - If `trace_mark_object` marks `obj->is_marked = true` but fails to push `obj` onto `gray_objects` due to OOM, what color does `obj` represent? What will `sweep()` do to `obj`'s children?
   - If `vm_track_object` assigns `tracker_id = vm->objects->count - 1` after a failed `stack_push`, what object's slot does `tracker_id` now refer to? What happens when `obj` is later freed and calls `vm_untrack_object`?
   - Why is `int new_capacity = stack->capacity * 2;` risky when `capacity` is typed as `size_t`?
3. **Failure Modes & Pitfalls**:
   - Silent OOM drop: Ignoring `stack_push` return value leads to dangling memory, state desynchronization, or use-after-free during subsequent garbage collections.
   - Overflow on zero: If `capacity == 0`, multiplying by 2 yields 0, leading to zero-byte realloc and out-of-bounds writes.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. Fix `stack_push` capacity arithmetic in `src/vm/stack.c`: calculate `new_capacity = (stack->capacity == 0) ? 8 : stack->capacity * 2` with `size_t` and overflow checks.
   2. Update `vm_track_object` to return `bool` in `src/vm/vm.h` and `src/vm/vm.c`. Ensure `tracker_id` is only assigned when the push succeeds.
   3. Update `object_new` in `src/vm_objects/object.c`: if `vm_track_object(obj)` fails, immediately free `obj` and return `NULL`.
   4. Update `frame_reference_object` in `src/vm/vm.c`: check `stack_push(frame->references, obj)`; only increment `object_refcount_inc(obj)` if the push succeeds, and return `bool`.
   5. Update `trace_mark_object` in `src/vm/vm.c`: check `stack_push(gray_objects, obj)`; only set `obj->is_marked = true` if the push succeeds.
   6. Update `vm_new_frame` in `src/vm/vm.c`: check `malloc` and `stack_new` results; roll back cleanly if either allocation fails.
   7. Update `vm_new` in `src/vm/vm.h` and `src/vm/vm.c`: return `bool` (or check existing `CURRENT_VM`), cleanly handling allocation failures without leaking.
2. **File Touchpoints**:
   - `src/vm/stack.h`
   - `src/vm/stack.c`
   - `src/vm/vm.h`
   - `src/vm/vm.c`
   - `src/vm_objects/object.c`
   - `tests/test_stack.c`
   - `tests/test_vm.c`
   - `tests/test_frame.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**:
   1. Test `stack_push` starting from `capacity = 0`, verifying graceful expansion to 8 and subsequent doublings.
   2. Inject allocation failures via `boot_set_fail_alloc_after` during `object_new`, verifying clean rollback and zero leaks.
   3. Inject allocation failures during `frame_reference_object`, asserting refcounts remain unchanged on push failure.
   4. Inject allocation failures during `trace()` on `stack_new(8)` and subsequent gray stack pushes, verifying no crashes or corruptions.
   5. Inject allocation failure during `vm_new()`, verifying no orphaned frames or object stacks leak.
2. **Zero-Leak Guarantee**: Explicit assertion that all allocations are tracked and confirmed freed via `assert(boot_all_freed())`.
3. **Tooling Quality Gates**: `just test` (100% pass with ASan/UBSan), `just coverage` (100.00% line coverage), `just lint` (`clang-tidy` + docstrings), `just check` (all pre-commit hooks).
4. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson file in `lessons/` following the `lesson-extraction` skill.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Robert C. Seacord — *Effective C: An Introduction to Professional C Programming* (No Starch Press, 2020)](../resources/seacord_2020_effective_c.pdf): Chapter 6 (Dynamically Allocated Memory, §Common Dynamic Memory Errors), detailing multi-stage error rollbacks, allocation failure handling, and state restoration.
   - [Jens Gustedt — *Modern C* (Manning Publications, 3rd Ed, 2023)](../resources/gustedt_2023_modern_c.epub): Level 2, Chapter 15 (Program Failure), explaining degraded runtime invariants, defensive assertions, and structured cleanup.
   - [SEI CERT C Coding Standard - MEM32-C](https://wiki.sei.corg.cmu.edu/confluence/display/c/MEM32-C.+Detect+and+handle+memory+allocation+errors): Comprehensive rules for verifying all fallible allocation and realloc returns in C systems programming.
   - [On-the-Fly Garbage Collection: An Exercise in Cooperation (Dijkstra et al.)](https://www.cs.utexas.edu/users/EWD/ewd04xx/EWD496.PDF): Foundational paper defining the tri-color marking abstraction and the essential invariant that black nodes cannot point to white nodes without gray intermediaries.
2. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython Internal Stack Evaluation (Python/ceval.c)](https://github.com/python/cpython/blob/main/Python/ceval.c): How production runtimes size and bounds-check evaluation stacks without runaway recursion or unhandled allocation crashes.
   - [Linux Kernel Memory Allocation Error Handling Patterns](https://www.kernel.org/doc/html/latest/process/coding-style.html#centralized-exiting-of-functions): The canonical `goto cleanup` idiom for unwinding multi-stage allocations on early failure.
