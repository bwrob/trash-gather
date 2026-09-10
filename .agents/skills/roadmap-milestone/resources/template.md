# Milestone: <Milestone Title>

**ID:** `<hash_id>`\
**Status:** Planned | In Progress | Completed\
**Focus:** \<1-2 sentence core technical objective summarizing the engineering goal>\
**Prerequisites:** \[<Milestone Title>\](\<hash_id>\_<slug>.md) | None

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Clear, concrete enumerated statements of what this milestone builds or solves.
1. **Scope Boundaries**: Explicit non-goals (what is intentionally deferred to future milestones).

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**: Detailed data structures, struct definitions, ASCII diagrams, or memory models.
1. **Core Systems Invariants**: Rules that must never be violated (e.g. NULL safety, lifecycle ownership, allocation boundaries, tracking registration).
1. **Architectural Trade-offs**: Analysis of design trade-offs (e.g. CPU vs memory, cache locality vs indirection, complexity vs safety).

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Key computer science and systems programming principles (e.g. tri-color marking, weak generational hypothesis, open addressing).
1. **Socratic Inquiries**: Guiding questions to prompt self-discovery on failure modes, undefined behavior, and edge cases.
1. **Failure Modes & Pitfalls**: Common traps (e.g. use-after-free, memory fragmentation, uninitialized slots).

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**: Ordered, incremental tasks to implement the milestone.
1. **File Touchpoints**: Exact file paths affected across `src/`, `include/`, `tests/`, and `bench/`.

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Specific tests in `tests/` to write or run (boundary conditions, stress tests, failure simulations).
1. **Zero-Leak Guarantee**: Explicit assertion that all allocations are tracked and confirmed freed via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test` (100% pass with ASan/UBSan), `just lint` (`clang-tidy` + docstrings), `just check` (all pre-commit hooks).
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson file in `lessons/` following the `lesson-extraction` skill.
