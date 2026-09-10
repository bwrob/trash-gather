# Milestone: Comprehensive Runtime Source Documentation & Doxygen Annotations

**ID:** `f06ad6f`\
**Status:** Planned\
**Focus:** Add complete Doxygen docstrings, memory contracts, and architectural invariants across all source files in `src/`, expanding automated docstring linting to enforce runtime coverage.\
**Prerequisites:** [Heap-Allocated Variable-Length Tuple](f9c475f_heap_allocated_variable_length_tuple.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Add comprehensive Doxygen docstring headers and function annotations across all C files in `src/` (`src/main.c`, `src/new.c`, `src/object.c`, `src/stack.c`, `src/vm.c`); explicitly document pointer ownership rules (borrowed vs owned references), lifecycle preconditions, and GC invariant assumptions; configure `scripts/lint_docstrings.py` to enforce docstring coverage for `src/`.
1. **Scope Boundaries**: Modifying algorithmic behavior or refactoring runtime logic is strictly out of scope; this milestone focuses purely on documentation, invariant specification, and docstring linting enforcement.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Architectural component overview documented directly in source headers:
     ```
     [vm_t Runtime Instance]
       ├── frames (stack_t*) ───────> [frame_t] ──> slots [object_t* roots]
       ├── objects (tracker_t*) ────> [object_t* array (all live heap allocs)]
       ├── small_ints cache ────────> [-128..127 immortal flyweights]
       └── gc_stats_t ──────────────> [allocation & sweep telemetry]
     ```
1. **Core Systems Invariants**:
   - Doxygen completeness invariant: Every function declaration and definition in `src/` must specify `@brief`, `@param[in/out]`, and `@return` tags conforming to `scripts/lint_docstrings.py`.
   - Memory ownership contract invariant: Every allocator and deallocator must document pointer ownership semantics (whether callers inherit ownership, whether references are borrowed, and who is responsible for decref).
   - CI gating invariant: Once documented, `just lint-docs` and `just check` must enforce 100% docstring compliance across `src/` on every commit and PR.
1. **Architectural Trade-offs**: Enforcing strict docstrings on `src/` introduces minor authoring overhead when adding new runtime functions, but establishes durable architectural clarity and prevents knowledge decay in a solo educational project.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Self-documenting systems code; API contracts; defensive programming invariants; Doxygen tag syntax and static documentation verification.
1. **Socratic Inquiries**:
   - Why is documenting whether a function returns a borrowed pointer vs an owned reference essential for preventing memory leaks in C?
   - What happens when a function's documented invariant (e.g. "pointer must not be NULL") is violated, and how should defensive assertions reflect that?
   - How do architectural diagrams inside module headers help a developer navigate complex pointer interactions before writing new code?
1. **Failure Modes & Pitfalls**: Outdated docstrings that drift from the implementation; documenting line-by-line mechanics rather than explaining design invariants and memory ownership contracts.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. Document all object creation helpers, type tags, and union field ownership in `src/object.c` and `src/new.c`.
   1. Document stack resizing, frame boundary rules, and error conditions in `src/stack.c`.
   1. Document GC phase state machines (`gc_mark()`, `gc_sweep()`, `vm_collect()`) and tracker arrays in `src/vm.c`.
   1. Document CLI startup, argument handling, and execution flow in `src/main.c`.
   1. Update `scripts/lint_docstrings.py` to include `src` in its scanned directory list.
   1. Run `just lint-docs` and `just check` to verify full repository compliance.
1. **File Touchpoints**:
   1. `src/main.c`
   1. `src/new.c`
   1. `src/object.c`
   1. `src/stack.c`
   1. `src/vm.c`
   1. `scripts/lint_docstrings.py`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: `just lint-docs` passes cleanly with `src/` included in the verification list, reporting zero missing docstrings.
1. **Zero-Leak Guarantee**: No runtime logic changes; existing test suite executes with zero memory leaks verified via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero warnings under ASan/UBSan.
