# Milestone: Learning-Friendly Modern Tooling & Safety Infrastructure

**ID:** `d9c6780`\
**Status:** Completed\
**Focus:** Establish professional C development, sanitizers, leak tracking, formatting, and pre-commit tooling from day one.\
**Prerequisites:** None

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Establish a modern C systems programming environment with `just`, AddressSanitizer (ASan), UndefinedBehaviorSanitizer (UBSan), `bootlib` leak tracking, `clang-format`, `clang-tidy`, `uv`, `ruff`, `pyrefly`, and pre-commit hooks.
1. **Scope Boundaries**: Runtime feature implementation (garbage collection algorithms and object types) is deferred to subsequent milestones.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**: Intercept system allocators (`malloc`, `calloc`, `realloc`, `free`) with `bootlib` to record pointer origins, allocation sizes, and caller callstacks.
1. **Core Systems Invariants**:
   - Zero-leak invariant: Every test must verify zero outstanding allocations via `assert(boot_all_freed())`.
   - Strict agent boundary invariant: AI assistants must never create, modify, or delete files within `src/`.
1. **Architectural Trade-offs**: Adopting rigorous CI and static analysis from day one introduces initial configuration overhead, but eliminates entire classes of subtle memory corruption bugs before they compound.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Compiler instrumentation (ASan shadow memory bytes) and runtime allocator interposition for deterministic memory tracking.
1. **Socratic Inquiries**:
   - Why do memory management bugs stay silently dormant in C until complex pointer graphs are introduced?
   - How does `bootlib` programmatic verification (`assert(boot_all_freed())`) differ from post-process leak checking?
1. **Failure Modes & Pitfalls**: Uncaught use-after-free, buffer overflows, memory leaks, and POSIX symbol collisions.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Configure `Justfile` recipes for build, test, lint, format, check, and bench workflows.
   - Configure `.pre-commit-config.yaml` to enforce formatting, linting, and docstrings.
   - Integrate µnit test runner and `bootlib` memory tracker into the build pipeline.
   - Set up GitHub Actions CI in `.github/workflows/ci.yml`.
1. **File Touchpoints**:
   - `Justfile`
   - `.pre-commit-config.yaml`
   - `vendor/bootlib/bootlib.c`, `vendor/bootlib/bootlib.h`
   - `vendor/munit/munit.c`, `vendor/munit/munit.h`
   - `scripts/lint_docstrings.py`
   - `.github/workflows/ci.yml`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: µnit test suite runs with ASan/UBSan enabled.
1. **Zero-Leak Guarantee**: Test cases conclude with `assert(boot_all_freed())` confirming 100% reclamation.
1. **Tooling Quality Gates**: `just check`, `just lint`, and `just test` pass cleanly on macOS and CI.
