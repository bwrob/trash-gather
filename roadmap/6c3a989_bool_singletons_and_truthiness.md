# Milestone: Boolean Immortal Singletons & Truthiness

**ID:** `6c3a989`\
**Status:** Planned\
**Focus:** Implement immortal boolean singleton objects (`True` and `False`), protect them from GC deallocation, introduce polymorphic truthiness evaluation (`object_is_truthy`, `object_to_bool`), and implement short-circuiting iteration predicates (`object_all`, `object_any`).\
**Prerequisites:** [The None Immortal Singleton Object](fc1cc81_none_immortal_singleton.md), [Polymorphic Sequence Length Protocol](b81f9a7_polymorphic_sequence_length.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Introduce `OBJ_BOOLEAN` kind to the runtime object model in `object_kind_t`.
   - Instantiate two immortal singleton objects (`True` and `False`) during runtime startup (`vm_new()`).
   - Provide constant-time runtime accessors `vm_get_true(vm)` and `vm_get_false(vm)` (and `new_bool(vm, bool value)`).
   - Protect boolean singletons against garbage collection sweep reclamation and reference counting deallocation.
   - Implement polymorphic truth value evaluation: `bool object_is_truthy(const object_t *obj)`.
   - Implement polymorphic boolean constructor / converter: `object_t *object_to_bool(vm_t *vm, const object_t *obj)` (equivalent to Python `bool(x)`).
   - Implement short-circuiting collection predicates: `bool object_all(const object_t *iterable)` and `bool object_any(const object_t *iterable)` (along with VM-level wrappers `vm_all()` and `vm_any()`).
1. **Scope Boundaries**:
   - Full boolean logical short-circuiting operators (`and`, `or`, `not`) in bytecode execution are deferred to the bytecode evaluation engine.
   - Custom user-defined class truthiness hooks (e.g. `__bool__` or `__len__`) are deferred to the object-oriented protocol milestone.
   - Dynamic generator function iteration protocols are deferred to generator runtime milestones.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Object kind discriminator in `object_kind_t`:
     ```c
     typedef enum {
       OBJ_INT,
       OBJ_FLOAT,
       OBJ_STRING,
       OBJ_LIST,
       OBJ_TUPLE,
       OBJ_NONE,
       OBJ_BOOLEAN,
     } object_kind_t;
     ```
   - Singleton storage anchored directly in `vm_t`:
     ```
     +-------------------------------------------------------+
     |                         vm_t                          |
     |  +----------------+  +----------------+  +---------+  |
     |  |  none_object   |  |  true_object   |  |false_obj|  |
     +--+-------+--------+--+-------+--------+--+----+----+--+
                |                   |                |
                v                   v                v
         +--------------+    +--------------+  +--------------+
         | kind: NONE   |    | kind: BOOL   |  | kind: BOOL   |
         | immortal: 1  |    | value: true  |  | value: false |
         | refcount: SAT|    | immortal: 1  |  | immortal: 1  |
         +--------------+    +--------------+  +--------------+
     ```
   - Predicate short-circuiting control flow:
     ```
     object_all(iterable):
       For each element in iterable:
         if !object_is_truthy(elem) -> RETURN false (short-circuit immediately)
       RETURN true (vacuously true for empty collections)

     object_any(iterable):
       For each element in iterable:
         if object_is_truthy(elem)  -> RETURN true  (short-circuit immediately)
       RETURN false (empty collections return false)
     ```
1. **Core Systems Invariants**:
   - **Singleton Identity**: There exist exactly two boolean instances per VM. `new_bool(vm, true) == new_bool(vm, true)` and `new_bool(vm, false) == new_bool(vm, false)` must hold true via pointer equality (`==`).
   - **GC Sweep Immunity**: Boolean singletons must never be swept or freed by `vm_collect_garbage()`. They are allocated once at VM initialization and released exclusively at `vm_free()`.
   - **Refcount Immunity**: Decrementing the reference count on a boolean singleton must never trigger `object_free()`.
   - **Truthiness Specification**:
     - `NULL` evaluates to `false` (defensive rejection).
     - `None` evaluates to `false`.
     - `True` evaluates to `true`; `False` evaluates to `false`.
     - Numbers: integer `0` or float `0.0` is `false`; any non-zero value is `true`.
     - Sequences (Strings, Lists, Tuples): empty sequence (`object_len(seq) == 0`) is `false`; non-empty sequence (`object_len(seq) > 0`) is `true`.
   - **Short-Circuiting Guarantee**: `object_all()` must cease traversal on the very first falsy element encountered without reading subsequent elements. `object_any()` must cease traversal on the very first truthy element.
   - **Vacuous Truth**: An empty collection passed to `object_all()` must evaluate to `true` (universal quantification over an empty set: $\\forall x \\in \\emptyset: P(x)$ is vacuously true). An empty collection passed to `object_any()` must evaluate to `false`.
   - **Type Safety**: Non-iterable objects (Integers, Booleans, None, NULL) passed to `object_all()` or `object_any()` must be safely rejected without memory faults, returning `false`.
1. **Architectural Trade-offs**:
   - Interned singletons vs transient heap objects: Allocating dynamic boolean objects per comparison expression causes extreme heap churn and fragmentation. Immortal singletons incur an initial fixed memory footprint but provide zero-allocation boolean creation, cache permanence, and $O(1)$ pointer-identity comparisons.
   - Dedicated C predicates vs higher-order callback functions: Hardcoding `object_all` and `object_any` directly with polymorphic truthiness checks avoids function pointer dispatch overhead, inline loop unrolling hazards, and indirect branching latency in hot loops.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   - Flyweight Pattern and Object Interning in managed language runtimes.
   - Reference count saturation vs immortal flag tagging (CPython PEP 683 immortal objects).
   - Short-circuit evaluation semantics and mathematical predicate logic: universal quantification ($\\forall$) versus existential quantification ($\\exists$).
   - Vacuous truth in formal verification and type systems.
1. **Socratic Inquiries**:
   - In Python, why does `all([])` evaluate to `True` while `any([])` evaluates to `False`?
   - If `all()` is passed a 1,000,000-element list whose first element is `0`, how many memory dereferences must occur under strict short-circuiting? What happens if short-circuiting is omitted?
   - If an immortal object's reference count is decremented to zero by untrusted code, what memory bug occurs if immortality is not enforced in `object_decref()`?
   - Why must `object_to_bool()` return an immortal singleton rather than allocating a new boolean object on the heap?
1. **Failure Modes & Pitfalls**:
   - Allowing `object_decref()` to free boolean singletons, resulting in dangling pointers in the `vm_t` header.
   - Sweep phase of mark-and-sweep cycle collector reclaiming singletons if they are not added to the root set or marked immortal.
   - Failing to short-circuit in `object_all()` / `object_any()`, causing unnecessary CPU latency or reading through corrupted trailing elements.
   - Misclassifying an empty sequence or empty string as truthy because the pointer itself is non-NULL.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Extend `object_kind_t` in `src/object.h` with `OBJ_BOOLEAN`.
   - Add boolean value field to `object_t` union payload: `bool bool_value;`.
   - Add `true_object` and `false_object` pointers to `vm_t` in `src/vm.h`.
   - Allocate and initialize both singletons in `vm_new()` (`src/vm.c`).
   - Update `object_decref()` (`src/object.c`) and sweep collection (`src/gc.c`) to respect singleton immortality.
   - Implement `bool object_is_truthy(const object_t *obj)` in `src/object.c`.
   - Implement `object_t *object_to_bool(vm_t *vm, const object_t *obj)` in `src/object.c`.
   - Implement `bool object_all(const object_t *iterable)` and `bool object_any(const object_t *iterable)` in `src/object.c`.
   - Implement `object_t *vm_all(vm_t *vm, const object_t *iterable)` and `object_t *vm_any(vm_t *vm, const object_t *iterable)` returning boolean singletons in `src/vm.c`.
   - Free both singletons cleanly inside `vm_free()` (`src/vm.c`).
   - Add thorough unit and adversarial test suites in `tests/test_object.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/vm.h`, `src/vm.c`
   - `src/gc.c`
   - `tests/test_object.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**:
   - Pointer identity validation: verify that repeated queries for `True` and `False` return identical memory addresses (`new_bool(vm, true) == vm_get_true(vm)`).
   - Decref stress test: call `object_decref()` 10,000 times on `True` and `False` and verify neither pointer is corrupted or freed.
   - GC collection immunity: trigger full garbage collection cycles and verify boolean singletons survive intact.
   - Comprehensive truthiness coverage: verify truthiness evaluation across all runtime kinds (`NULL`, `None`, `True`, `False`, integers, empty/non-empty strings, empty/non-empty lists, empty/non-empty tuples).
   - Short-circuit verification: test `object_all()` on `[0, panic_ptr]` and verify execution stops at `0` without dereferencing `panic_ptr`.
   - Vacuous truth validation: verify `object_all([]) == true`, `object_all(()) == true`, `object_any([]) == false`, `object_any(()) == false`.
   - Heterogeneous containers: test `all` and `any` on lists containing mixed types (integers, strings, booleans, nested tuples).
1. **Zero-Leak Guarantee**:
   - All tests must pass with `assert(boot_all_freed())`, verifying complete reclamation of VM singletons at runtime shutdown.
1. **Tooling Quality Gates**:
   - `just test` (100% pass rate under AddressSanitizer and UndefinedBehaviorSanitizer).
   - `just lint` (`clang-tidy`, Doxygen docstring checks, and `just lint-roadmap`).
   - `just check` (all pre-commit git hooks clean).
1. **Milestone Completion & Lesson Extraction**:
   - Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson file in `lessons/` following the `lesson-extraction` skill.
