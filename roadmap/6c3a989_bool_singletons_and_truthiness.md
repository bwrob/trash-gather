# Milestone: Boolean Immortal Singletons & Truthiness

**ID:** `6c3a989`\
**Status:** Planned\
**Focus:** Implement immortal boolean singleton objects (`True` and `False`), protect them from GC sweep deallocation, and introduce runtime truthiness evaluation (`object_is_truthy`).\
**Prerequisites:** [The None Immortal Singleton Object](fc1cc81_none_immortal_singleton.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Introduce `OBJ_BOOLEAN` kind to the runtime object model.
   - Instantiate two immortal singleton objects (`True` and `False`) during runtime startup (`vm_new()`).
   - Provide constant-time runtime accessors `vm_get_true(vm)` and `vm_get_false(vm)` (or `new_bool(vm, bool value)`).
   - Protect boolean singletons against garbage collection sweep reclamation and reference counting deallocation.
   - Implement polymorphic truth value evaluation: `bool object_is_truthy(const object_t *obj)`.
1. **Scope Boundaries**:
   - Full boolean logical short-circuiting operators (`and`, `or`, `not`) in bytecode execution are deferred to the bytecode evaluation engine.
   - Custom user-defined class truthiness hooks (e.g. `__bool__` or `__len__`) are deferred to the object-oriented protocol milestone.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - New object kind in `object_kind_t`:
     ```c
     typedef enum {
       OBJ_INT,
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
1. **Core Systems Invariants**:
   - **Singleton Identity**: There exist exactly two boolean instances per VM. `new_bool(vm, true) == new_bool(vm, true)` and `new_bool(vm, false) == new_bool(vm, false)` must hold true via pointer equality (`==`).
   - **GC Sweep Immunity**: Boolean singletons must never be swept or freed by `vm_collect_garbage()`. They are allocated once at VM initialization and released exclusively at `vm_free()`.
   - **Refcount Immunity**: Decrementing the reference count on a boolean singleton must never trigger `object_free()`.
   - **Truthiness Specification**:
     - `NULL` evaluates to `false` (defensive rejection).
     - `None` evaluates to `false`.
     - `True` evaluates to `true`; `False` evaluates to `false`.
     - Numbers: integer `0` is `false`; any non-zero integer is `true`.
     - Sequences (Strings, Lists, Tuples): empty sequence (length 0) is `false`; non-empty sequence (length > 0) is `true`.
1. **Architectural Trade-offs**:
   - Interned singletons vs transient heap objects: Allocating dynamic boolean objects per comparison expression causes extreme heap churn and fragmentation. Immortal singletons incur an initial fixed memory footprint but provide zero-allocation boolean creation, cache permanence, and $O(1)$ pointer-identity comparisons.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   - Flyweight Pattern and Object Interning in managed language runtimes.
   - Reference count saturation vs immortal flag tagging (CPython PEP 683 immortal objects).
   - Truthiness semantics across language architectures: type-coercive truthiness vs strict structural evaluation.
1. **Socratic Inquiries**:
   - In Python, why does `x is True` perform a fast pointer comparison whereas `x == True` invokes value equality?
   - If an immortal object's reference count is decremented to zero by untrusted code, what memory bug occurs if immortality is not enforced in `object_decref()`?
   - How does separating boolean identity from integer values (unlike legacy C where any integer serves as a boolean) enhance runtime safety and type introspection?
1. **Failure Modes & Pitfalls**:
   - Allowing `object_decref()` to free boolean singletons, resulting in dangling pointers in the `vm_t` header.
   - Sweep phase of mark-and-sweep cycle collector reclaiming singletons if they are not added to the root set.
   - Misinterpreting an empty string or empty tuple as truthy because the pointer itself is non-NULL.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Extend `object_kind_t` in `src/object.h` with `OBJ_BOOLEAN`.
   - Add boolean value field to `object_t` union payload: `bool bool_value;`.
   - Add `true_object` and `false_object` pointers to `vm_t` in `src/vm.h`.
   - Allocate and initialize both singletons in `vm_new()` (`src/vm.c`).
   - Update `object_decref()` (`src/object.c`) and sweep collection (`src/gc.c`) to respect singleton immortality.
   - Implement `object_is_truthy(const object_t *obj)` in `src/object.c`.
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
   - Pointer identity validation: verify that repeated queries for `True` and `False` return identical memory addresses.
   - Decref stress test: call `object_decref()` 10,000 times on `True` and `False` and verify neither pointer is corrupted or freed.
   - GC collection immunity: trigger full garbage collection cycles and verify boolean singletons survive intact.
   - Comprehensive truthiness coverage: verify truthiness evaluation across all runtime kinds (`NULL`, `None`, `True`, `False`, integers, empty/non-empty strings, empty/non-empty lists, empty/non-empty tuples).
1. **Zero-Leak Guarantee**:
   - All tests must pass with `assert(boot_all_freed())`, verifying complete reclamation of VM singletons at runtime shutdown.
1. **Tooling Quality Gates**:
   - `just test` (100% pass rate under AddressSanitizer and UndefinedBehaviorSanitizer).
   - `just lint` (`clang-tidy`, Doxygen docstring checks, and `just lint-roadmap`).
   - `just check` (all pre-commit git hooks clean).
1. **Milestone Completion & Lesson Extraction**:
   - Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson file in `lessons/` following the `lesson-extraction` skill.
