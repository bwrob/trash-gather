# Milestone: The `None` Immortal Singleton Object

**ID:** `fc1cc81`\
**Status:** Planned\
**Focus:** Implement the `None` singleton object, protect it against GC sweep deallocation (immortality), and use it for uninitialized slots and default returns.\
**Prerequisites:** [Comprehensive Runtime Source Documentation & Doxygen Annotations](f06ad6f_document_entire_source.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Implement a singleton object kind `NONE` accessible via `new_none()` (or `vm_get_none()`), mirroring Python's `None` (`Py_None`).
1. **Scope Boundaries**: Other singleton constants (such as `True` and `False` booleans) are optional extensions following the same pattern.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - New object kind: `NONE` added to `object_kind_t`.
   - Global singleton instance housed in `vm_t`:
     ```c
     struct VM {
       ...
       object_t *none_object;
     };
     ```
1. **Core Systems Invariants**:
   - Identity invariant: There exists exactly one `None` object in the runtime; `new_none() == new_none()` always evaluates to `true` (pointer equality).
   - Immortality invariant: The `None` singleton must never be swept or deallocated by `vm_collect_garbage()`. It is created at `vm_new()` and freed only at `vm_free()`.
   - Refcount immunity: Decrefing `None` must never trigger `object_free()`.
1. **Architectural Trade-offs**: Using a real `None` object rather than raw C `NULL` enables consistent object semantics and method dispatch across all types, but requires special-case immunity in the GC sweep loop.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: The Null Object Pattern; immortal objects in managed runtimes (CPython PEP 683); static vs heap object lifetimes.
1. **Socratic Inquiries**:
   - In Python, why does `a is None` check pointer identity rather than value equality?
   - How can the sweep phase distinguish an immortal singleton from normal heap objects? Is it better to set an `is_immortal` bit, check a dedicated pointer address (`obj == vm->none_object`), or keep it permanently marked?
   - What happens if a user stores `None` inside a list or tuple that is later freed? Should `_refcount_dec(none)` be a no-op?
1. **Failure Modes & Pitfalls**: The GC sweep loop inadvertently freeing `none_object`, causing a dangling pointer; `new_none()` returning multiple distinct allocations; memory leaks if `none_object` is tracked incorrectly in the VM object array.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Add `NONE` to `object_kind_t` in `src/object.h`.
   - Add `object_t *none_object;` to `vm_t` in `src/vm.h`.
   - Initialize `none_object` in `vm_new()` in `src/vm.c`.
   - Implement `new_none()` in `src/new.c` and `src/new.h` returning `CURRENT_VM->none_object`.
   - Update `sweep()` in `src/vm.c` to skip `CURRENT_VM->none_object`.
   - Update `_refcount_dec()` in `src/object.c` so `NONE` objects never deallocate.
   - Free `none_object` during `vm_free()` in `src/vm.c`.
   - Write unit tests in `tests/test_new.c` and `tests/test_object.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/new.h`, `src/new.c`
   - `src/vm.h`, `src/vm.c`
   - `tests/test_new.c`, `tests/test_object.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify `new_none()` returns the identical pointer on repeated calls, and setting list/tuple slots to `None` works smoothly.
1. **Zero-Leak Guarantee**: Running repeated GC collection passes with rooted and unrooted `None` references confirms `assert(boot_all_freed())` upon `vm_free()`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero errors.
