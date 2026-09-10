# Milestone: Weak References & Non-Owning Pointers (`weakref_t`)

**ID:** `a7ca40b`\
**Status:** Planned\
**Focus:** Implement non-owning pointer handles (`weakref_t`) that observe target objects without preventing GC reclamation, automatically clearing to NULL when the referee is collected.\
**Prerequisites:** [Full CPython-Style Offset-0 Hierarchy](222f6ce_cpython_offset0_hierarchy.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Introduce the `weakref_t` object type, allowing users to create weak reference handles (`new_weakref(target)`); ensure weak references do not increment the referee's `refcount` or keep it alive during GC mark phase; implement an active weak reference registry in `vm_t`; during `gc_sweep()`, automatically clear (`ref = NULL`) all weak references whose targets are condemned before memory deallocation.
1. **Scope Boundaries**: Dead-notification callbacks and weak-key/weak-value dictionary collections are deferred to future extensions.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Weak reference wrapper holding a non-owning raw pointer to the target object:
     ```
     [Stack Root]
       ├── var 'wr' ──> [Object: WEAKREF #1] ──(non-owning)──┐
       │                                                     ▼
       └── var 'obj' ──> [Object: TARGET #2 (rc=1)] <────────┘

     After 'obj' is popped from stack:
       [Target #2 has rc=0] ──> Deallocated during sweep
       [WEAKREF #1] target pointer automatically nulled:
       [Object: WEAKREF #1] ──> target: NULL (returns None on deref)
     ```
1. **Core Systems Invariants**:
   - Non-owning reference invariant: Creating, holding, or copying a `weakref_t` must never increment the referee's `refcount` or mark it as reachable during the mark phase.
   - Safe clearing before deallocation invariant: The GC sweep phase must inspect all registered weak references and clear unmarked targets to `NULL` *before* the target object's memory is released, preventing use-after-free hazards.
   - Registry cleanup invariant: When a `weakref_t` instance itself is deallocated, it must unregister itself from the VM's active weak reference table.
1. **Architectural Trade-offs**: Maintaining a central registry of active weak references introduces small linear scanning overhead during GC sweep, but provides a 100% safe, dangling-pointer-free non-owning reference abstraction.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Non-owning references; observer pattern; breaking reference cycles with weak links; dangling pointer mitigation; CPython `PyWeakReference` and JVM `WeakReference`.
1. **Socratic Inquiries**:
   - Why would an object cache or a parent-pointer in a tree structure benefit from weak references instead of strong references?
   - Why must the garbage collector clear weak references in a dedicated pre-sweep pass rather than after `boot_free()` has already returned the target's memory to the OS?
   - What happens if user code calls `weakref_deref(wr)` on a weak reference whose target was collected? How does returning `None` prevent undefined behavior?
1. **Failure Modes & Pitfalls**: Dereferencing a dangling pointer if the weakref is not cleared before target deallocation; marking weakref targets as roots during cycle tracing, which would accidentally resurrect or keep them alive forever.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. Define `weakref_t` object layout and `OBJ_WEAKREF` type tag in `include/object.h`.
   1. Add the weak reference tracking list to `vm_t` in `include/vm.h`.
   1. Implement `new_weakref()` and `weakref_deref()` in `src/new.c` and `src/object.c`.
   1. Update `gc_mark()` in `src/vm.c` to deliberately avoid tracing through weak reference target pointers.
   1. Add a pre-sweep pass in `src/vm.c` that checks registered weak references and nulls targets that are unmarked.
   1. Write unit and adversarial tests in `tests/test_object.c` verifying dereferencing, automatic nulling on target deallocation, and cycle breaking.
1. **File Touchpoints**:
   1. `include/object.h`
   1. `include/vm.h`
   1. `src/object.c`
   1. `src/new.c`
   1. `src/vm.c`
   1. `tests/test_object.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify that creating a weakref does not prevent immediate refcount deallocation when the target's only strong reference is dropped; verify that `weakref_deref()` returns `None` after the target is collected; verify that weak references break cycles between two objects.
1. **Zero-Leak Guarantee**: All weak reference instances, targets, and registry entries are cleanly tracked and freed with zero memory leaks via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero warnings under ASan/UBSan.
