# Milestone: Type Descriptor Tables & Function Pointer Dispatch

**ID:** `54d0d12`\
**Status:** Planned\
**Difficulty:** 2 / 5\
**Focus:** Replace monolithic `switch (obj->kind)` control flow with static type descriptor vtables (`type_spec_t`) holding function pointers for operations (`tp_add`, `tp_len`, `tp_dealloc`), mastering function pointer syntax, callback signatures, and open/closed dispatch tables in C.\
**Prerequisites:** [Polymorphic Multiplication & Sequence Repetition](687b4cb_polymorphic_multiplication.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Define function pointer typedefs for core polymorphic operations:
     - `typedef object_t *(*binary_func_t)(object_t *a, object_t *b);`
     - `typedef int64_t (*len_func_t)(const object_t *obj);`
     - `typedef void (*destructor_func_t)(object_t *obj);`
   - Define `type_spec_t` containing type metadata (`const char *name`, method slots `tp_add`, `tp_mul`, `tp_len`, `tp_dealloc`).
   - Create a static array of type descriptors indexed by `object_kind_t`.
   - Refactor `object_add`, `object_len`, and `object_free_payload` to dispatch directly through the type descriptor table, eliminating repetitive `switch` statements.
1. **Scope Boundaries**:
   - Dynamic user-defined classes are deferred to Tier 5.
   - Offset-0 base struct embedding is handled in Milestone `222f6ce_cpython_offset0_hierarchy.md`.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Type descriptor dispatch table:

     ```text
     object_t
       [kind = TUPLE] ------------\
                                  v
                        type_specs[TUPLE]
                        +---------------------------------------+
                        | const char *tp_name = "tuple"         |
                        | binary_func_t tp_add = tuple_concat   |
                        | binary_func_t tp_mul = tuple_repeat   |
                        | len_func_t    tp_len = tuple_length   |
                        | destructor_t  tp_dealloc = tuple_free |
                        +---------------------------------------+
     ```

1. **Core Systems Invariants**:
   - NULL-slot safety: If a type does not implement an operation (e.g. `FLOAT` has no `tp_len`), the slot is `NULL`. Callers must assert or return a type error before invoking a NULL function pointer.
   - Signature consistency: All implementation functions must strictly match the declared function pointer signatures without requiring unsafe casts.
   - Const-correctness: Type descriptors are static, read-only structures stored in the program text/data segment (`const type_spec_t type_specs[]`).
1. **Architectural Trade-offs**: Vtable dispatch adds an extra pointer dereference compared to a direct function call, but decouples type implementations into modular, single-responsibility files and eliminates $O(N)$ code sprawl in central switch blocks.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Function pointers and indirect branch execution; function pointer syntax and `typedef` declarations in C; CPython's `PyTypeObject` table architecture; the Open/Closed Principle in procedural languages.
1. **Socratic Inquiries**:
   - How does a function pointer actually work at the machine code level? Where in memory does the pointer point?
   - Why is `typedef object_t *(*binary_func)(object_t *, object_t *);` so much easier to read and maintain than writing the raw function pointer type in struct fields?
   - What happens if code attempts to jump to a `NULL` function pointer (e.g. `type->tp_len(obj)` when `tp_len == NULL`)?
1. **Failure Modes & Pitfalls**: Calling a NULL function pointer slot leading to immediate segmentation faults; signature mismatches causing subtle stack corruption on return; compiler warnings from missing `const` qualifiers.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Define callback function pointer typedefs in `src/type.h`.
   - Define `struct TypeSpec` (`type_spec_t`) in `src/type.h`.
   - Create `src/type.c` with the static table of type descriptors for all existing kinds (`INTEGER`, `FLOAT`, `STRING`, `TUPLE`, `LIST`, `NONE`, `BOOL`).
   - Implement `const type_spec_t *type_spec_get(object_kind_t kind);` in `src/type.c`.
   - Refactor `object_add`, `object_mul`, and `object_len` in `src/object.c` to look up the type spec and invoke the slot function pointer.
   - Add unit tests in `tests/test_type.c` testing vtable dispatch and unsupported operation error returns.
1. **File Touchpoints**:
   - `src/type.h`, `src/type.c`
   - `src/object.h`, `src/object.c`
   - `tests/test_type.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify valid polymorphic dispatch across all types; verify graceful failure when calling unsupported operations (NULL slots); ensure type names match expected strings.
1. **Zero-Leak Guarantee**: Verify zero memory leaks via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Function Pointers and Callback Signatures in C](https://en.cppreference.com/w/c/language/pointer#Pointers_to_functions): Syntax, type declarations, calling conventions, and typedef idioms for function pointers.
   - [Virtual Method Tables and Dynamic Dispatch](https://en.wikipedia.org/wiki/Virtual_method_table): Architectural separation of object data from type behavior tables.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython PyTypeObject Structure and Slot Defs](https://docs.python.org/3/c-api/typeobj.html): Deep dive into CPython's type method tables (`tp_as_number`, `tp_as_sequence`, `tp_dealloc`).
   - [The Open/Closed Principle in Procedural Systems Code](https://en.wikipedia.org/wiki/Open%E2%80%93closed_principle): Extending runtime behavior without modifying core dispatch switch blocks.
