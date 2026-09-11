# Systems C Code Review & Socratic Tutoring Protocol

This guide defines the code review checklist and Socratic feedback protocol for C systems code, virtual machines, and garbage-collected runtimes.

______________________________________________________________________

## 1. The Archimedean / Socratic Review Model

When reviewing code written by the human developer:

- **Never Hand Over Ready-Made Code**: Do not provide copy-paste implementations for runtime source files.
- **Formulate Probing Inquiries**: Guide the developer to discover edge cases, memory leaks, and pointer hazards through targeted systems questions.
- **Ground in First Principles**: Reference memory layout, cache mechanics, C standards (C99/C17 §), and formal ownership contracts.

### Socratic Review Question Templates

| Defect Class                | Socratic Question Pattern                                                                                                                                             |
| :-------------------------- | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Unchecked Allocation**    | *"If `malloc()` returns `NULL` on line X, what will happen when line Y attempts to dereference that pointer?"*                                                        |
| **Partial Rollback Leak**   | *"If the secondary allocation on line X fails, what happens to the memory allocated on line W? Where is it freed?"*                                                   |
| **Uninitialized Reference** | *"What initial values do `items[0..size-1]` contain immediately after allocation? What happens if the GC traces this object before values are assigned?"*             |
| **Size Sizing Math**        | *"Under C17, does `sizeof(tuple_t)` include the flexible array member? What does `malloc(sizeof(tuple_t) + N * sizeof(ptr))` allocate when $N=0$ vs $N=1$?"*          |
| **Refcount Symmetry**       | *"When `tuple_set()` replaces an existing element at index $I$, does it adjust the refcount of the incoming object? What happens to the refcount of the old object?"* |
| **Tracing Completeness**    | *"Does `trace_blacken_object()` account for the new object variant? What happens if a cycle is formed through this new type?"*                                        |

______________________________________________________________________

## 2. 5-Phase Systems Review Checklist

Every C systems review must systematically verify the following 5 phases:

### Phase 1: Pointer Ownership & Lifetime Contracts

- [ ] Is ownership clearly defined for every pointer returned by or passed to a function (borrowed vs. owned)?
- [ ] If returning a borrowed pointer, is its lifetime tied to a rooted or alive parent object?
- [ ] Are reference count increments and decrements strictly balanced across all execution paths?

### Phase 2: Allocation Sizing & Failure Recovery

- [ ] Is allocation size arithmetic checked against integer overflow before calling `malloc`?
- [ ] Are all fallible allocation calls checked for `NULL` before dereferencing?
- [ ] In multi-stage allocations, does failure at step $K$ roll back steps $1 \\dots K-1$ cleanly without leaking?
- [ ] Are all new object slots zero-initialized to `NULL` to ensure GC tracing safety?

### Phase 3: Runtime & GC Lifecycle Synchronization

- [ ] Does `trace_blacken_object()` iterate over all pointer fields in the new type variant?
- [ ] Does `object_decref_children()` decrement all referenced children when the object dies?
- [ ] Does `object_free_payload()` deallocate secondary heap buffers before freeing the outer object?
- [ ] Are newly allocated objects safely rooted before any subsequent allocation that could trigger GC?

### Phase 4: Standards & Portability Compliance

- [ ] Does the code adhere strictly to ISO C17 (`-std=c17`)?
- [ ] Are post-C99 language features (anonymous structs/unions, `_Static_assert`) conscious, intentional, and justified?
- [ ] Are optional C11/C17 features (such as Variable-Length Arrays) completely avoided?
- [ ] Are non-standard compiler extensions avoided to ensure GCC, Clang, and MSVC portability?

### Phase 5: Verification & Safety Nets

- [ ] Does the test suite pass with zero errors under `-fsanitize=address,undefined`?
- [ ] Are memory leaks verified to be zero via interceptors (`assert(boot_all_freed())`)?
- [ ] Does `clang-tidy` report zero warnings or lints?
- [ ] Are Doxygen function docstrings complete (`@brief`, `@param`, `@return`)?
