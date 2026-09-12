# Milestone: Cycle-Safe String Representation & Object Printing

**ID:** `bf0a981`\
**Status:** Planned\
**Focus:** Serialize arbitrary objects to human-readable strings (`object_to_string()`), detecting and suppressing recursive loops for cyclic structures (`[...]`), paving the way for the interactive REPL.\
**Prerequisites:** [The None Immortal Singleton Object](fc1cc81_none_immortal_singleton.md), [Boolean Immortal Singletons & Truthiness](6c3a989_bool_singletons_and_truthiness.md), [Python-Style Sequence Negative Indexing](b0c1d8b_python_sequence_negative_indexing.md), [Cycle Iterator Object](586680e_cycle_iterator_object.md), [Polymorphic Multiplication & Sequence Repetition](687b4cb_polymorphic_multiplication.md), [Complex Numbers & Arithmetic](212a3d8_complex_numbers.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Implement `object_to_string(object_t *obj)` returning a dynamically allocated `char *` containing human-readable representations for all object types (`123`, `3.14`, `(1+2j)`, `"hello"`, `None`, `True`, `False`, `[1, 2, 3]`, `(1, "a")`, `<cycle_iterator at 0x...>`).
1. **Scope Boundaries**: Terminal colorization and ANSI syntax highlighting are deferred to Milestone 14 (Interactive REPL).

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Recursive formatter using a dynamic string builder buffer:
     ```
     Integer:  "42"
     Float:    "3.14"
     Complex:  "(1+2j)"
     String:   "\"hello\""
     Tuple:    "(1, 2, 3)"
     List:     "[1, [2, 3], None]"
     Cycle:    "<cycle_iterator at 0x...>"
     Cyclic:   "[1, 2, [...]]"
     ```
1. **Core Systems Invariants**:
   - Cycle suppression invariant: If a container object is already being visited along the current print callstack, recursion must terminate immediately and emit a cycle indicator (`[...]` for lists, `(...)` for tuples) instead of recursing infinitely.
   - Memory management invariant: The returned string is a standard heap buffer (`malloc`) owned by the caller, who is responsible for calling `free()`.
   - Const-safety invariant: Serializing an object to a string must not mutate its internal data or alter its reference counts.
1. **Architectural Trade-offs**: Detecting cycles during printing requires tracking an active visitation stack (e.g. a small dynamic pointer array), adding minor tracking overhead during printing while guaranteeing immunity against stack overflow crashes.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Graph cycle detection during depth-first traversal; dynamic string concatenation buffer strategies; Python's `repr()` implementation mechanics.
1. **Socratic Inquiries**:
   - What happens if a list contains itself (`list[0] = list`) and you attempt a naive recursive `printf`? Why does the C runtime crash with `SIGSEGV` (stack overflow)?
   - How does Python's `repr()` detect that a list or dictionary is currently being printed?
   - How can you implement a dynamic string buffer in C using `snprintf` and geometric doubling without buffer overflows?
1. **Failure Modes & Pitfalls**: Infinite recursion on cyclic structures; buffer overflow on formatting large strings or deep sequences; memory leaks from untracked intermediate string fragments.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Implement a simple string builder helper `str_builder_t` in `src/object.c` (or a dedicated helper module).
   - Implement cycle-tracking stack `vm_stack_t *visiting` to record objects currently in the active print recursion chain.
   - Implement recursive formatter `object_format_recursive(object_t *obj, str_builder_t *sb, vm_stack_t *visiting)` in `src/object.c`.
   - Implement public API `char *object_to_string(object_t *obj)` and `void object_print(object_t *obj)` in `src/object.h` and `src/object.c`.
   - Write comprehensive unit tests in `tests/test_object.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `tests/test_object.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify formatting across all types: ints, floats, strings, tuples, empty containers, nested lists, and deliberate self-referencing cycles (`list -> list`).
1. **Zero-Leak Guarantee**: Formatted strings freed by tests leave zero leaked bytes confirmed via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero warnings.
