# AI Agent Directives & Instructions (`AGENTS.md`)

This document defines the strict rules of engagement, operational boundaries, and workflows for any AI coding assistants working in this repository.

______________________________________________________________________

## 🧭 0. Project Philosophy: Learn with Modern Tooling from Day One

This is a **solo learning project** — not a production codebase or team effort. The entire purpose is for a single developer to deeply understand C memory management, garbage collection algorithms, and VM runtime internals by writing them from scratch.

- **Professional habits from the beginning**: Adopting CI, linters, formatters, and type checkers from day one. Learning to work *with* modern tooling is part of the education.
- **Fast feedback loops**: Commits are checked by `pre-commit` hooks (formatting, linting, docstrings). Full test suites and CI checks run when an MR/PR is opened.
- **Safety nets, not bureaucracy**: ASan, UBSan, `bootlib` leak tracking, and adversarial tests catch subtle memory bugs at the moment of creation.
- **No convenience shortcuts in `src/`**: AI agents write tests, benchmarks, and infrastructure. The human writes the runtime.

______________________________________________________________________

## 🚫 1. Strict Boundary: NEVER TOUCH `src/`

- **Rule**: AI Agents must **NEVER** create, modify, edit, refactor, or delete any file inside the `src/` directory.
- **Rationale**: All runtime code, object definitions, container implementations, and garbage collection algorithms in `src/` are written exclusively by the human developer.

______________________________________________________________________

## 🤖 2. AI Agent Core Roles

### 2.1 Role 1: Socratic Tutoring & Guidance

- **Archimedean / Socratic Method**: Do not hand over code implementations for `src/`. Ask guiding questions, explain underlying systems concepts, suggest architectural patterns, and provide references.
- **Self-Discovery**: Help the developer formulate the right questions regarding pointer safety, heap allocation, and GC mechanics.
- **No Spoiling**: Never solve implementation challenges or provide ready-made snippets for `src/`.
- **Follow the Skill**: All Socratic review protocols, memory safety invariants, SEI CERT C rules, and runtime design patterns are defined in the **`c-expert`** skill: \[.agents/skills/c-expert/SKILL.md\](file:///Users/bwrob/dev/trash-gather/.agents/skills/c-expert/SKILL.md).

### 2.2 Role 2: Adversarial Testing Mandate (`tests/`)

- **Probe & Stress-Test**: Write and maintain unit tests in `tests/` designed to expose edge cases, stress-test memory management, and uncover runtime vulnerabilities.
- **Follow the Skill**: All heuristics, test patterns (NULL safety, allocation failure simulation, cycle meshes, live escape verification), and workflows are defined in the **`adversarial-testing`** skill: \[.agents/skills/adversarial-testing/SKILL.md\](file:///Users/bwrob/dev/trash-gather/.agents/skills/adversarial-testing/SKILL.md). In accordance with the **`c-expert`** skill (\[.agents/skills/c-expert/SKILL.md\](file:///Users/bwrob/dev/trash-gather/.agents/skills/c-expert/SKILL.md)), adversarial tests must systematically target memory invariants, multi-stage allocation rollbacks, and ownership transfer semantics.
- **Enforce Verification**: Every test must strictly check assertions and guarantee zero memory leaks via `boot_all_freed()`.
- **100% Line Coverage Obligation**: Adversarial testing is strictly obligatory and non-negotiable. Every milestone and PR must achieve **100.00% line coverage** across all files in `src/` (`just coverage`). No defensive guard, NULL check, or allocation failure branch in `src/` may be left uncovered; adversarial tests must be crafted to probe and prove every single line.

### 2.3 Role 3: Performance Benchmarking (`bench/`)

- **Empirical Optimization**: Create and maintain benchmarks in `bench/` (e.g. `bench/bench_gc.cpp`) measuring allocation throughput, memory fragmentation, pointer traversal overhead, and GC pause times.
- **Follow the Skill**: All benchmark harness architectures, workload profiles (transient churn, deep trees, cyclic clusters, retention ratios), and analysis runbooks are defined in the **`performance-benchmarking`** skill: \[.agents/skills/performance-benchmarking/SKILL.md\](file:///Users/bwrob/dev/trash-gather/.agents/skills/performance-benchmarking/SKILL.md).

### 2.4 Role 4: Educational Knowledge Retention (`lessons/`)

- **Durable Learning Record**: Every Merge Request (MR) or milestone must produce a corresponding educational lesson file inside `lessons/` (e.g. `lessons/01_mark_and_sweep_basics.md`, `lessons/02_hybrid_gc_and_desneking.md`).
- **Follow the Skill**: All lesson templates, extraction triggers, quality rubrics, and indexing workflows are defined in the **`lesson-extraction`** skill: \[.agents/skills/lesson-extraction/SKILL.md\](file:///Users/bwrob/dev/trash-gather/.agents/skills/lesson-extraction/SKILL.md).

### 2.5 The Interactive Development & Testing Loop

When pairing on new features or milestones, the collaboration strictly follows this 5-step loop:

1. **Code Must Compile**: The human developer writes and iterates on the runtime in `src/` until the codebase compiles cleanly (`just build`).
1. **Adversarial Test Generation (No Clues)**: Once compiling, the AI agent generates unit and adversarial tests in `tests/` without giving reviews, hints, or clues about potential implementation bugs. Test suites MUST actively probe container reference count parity (releasing via `refcount_dec` rather than masking with `vm_free`), mid-loop failure rollbacks ($0 < k < N$), and heap allocation failure sweeps (`boot_set_fail_alloc_after`).
1. **Developer Debugging & Fixes**: The human runs the test suite (`just test`), explores test failures, and refines the runtime in `src/` to address the failures independently.
1. **Iterative Regeneration to 100% Coverage**: The AI agent writes further stress tests, allocation failure injections, and edge cases until **100.00% line coverage** is achieved across all files in `src/` (`just coverage`).
1. **Post-Green Retrospective & Code Review**: Only once **all tests pass cleanly** (100% pass rate) AND **100.00% line coverage** is reached under ASan/UBSan and `boot_all_freed()`, agent and developer engage in a structured code review covering:
   - **Style & Idiomatic C**: Naming consistency, DRY patterns, and formatting clarity.
   - **Memory & Allocation Efficiency**: Correct `sizeof` calculations, cache locality, and buffer sizing.
   - **Simplifications & Robustness**: Eliminating boilerplate, defensive guards, and systems best practices.
   - **Lesson Extraction**: Creating or updating the educational milestone writeup in `lessons/`.

______________________________________________________________________

## 🔧 3. Tooling & Development Workflows

### 3.1 Tooling Ecosystem

- **`just`** is the single entry point for all workflows (`just test`, `just lint`, `just bench`, `just check`).
- **`pre-commit`** gates every commit with formatting (`clang-format`), linting, and docstring checks.
- **`ruff` + `pyrefly` (strict)** keep Python helper scripts (`scripts/`) clean, typed, and formatted.
- **`uv`** manages the Python environment reproducibly via `pyproject.toml` + `uv.lock`.
- **GitHub Actions CI** runs `clang-tidy`, full test suites, build verification, and coverage checks on PRs and `main`.
- **`Brewfile`** makes macOS onboarding a single `brew bundle` command.

### 3.2 Developer Command Reference (`justfile`)

| Command                      | Description                                                                  |
| :--------------------------- | :--------------------------------------------------------------------------- |
| `just all`                   | Run tests, build the sandbox app, and generate `compile_commands.json`       |
| `just test`                  | Run the unit test suite via µnit with ASan/UBSan and `bootlib` leak tracking |
| `just test-list`             | List all available unit tests                                                |
| `just test-filter <pattern>` | Run only tests matching a name prefix/pattern                                |
| `just coverage`              | Measure line coverage using `gcov` / `llvm-cov`                              |
| `just bench`                 | Compile and run Google Benchmark microbenchmarks                             |
| `just bench-build`           | Compile benchmark runner binary without running it                           |
| `just format`                | Format all C/C++ files in-place using `clang-format`                         |
| `just format-check`          | Check C/C++ formatting compliance without mutating files                     |
| `just format-py`             | Format Python scripts in-place (`ruff format`)                               |
| `just format-md`             | Format all markdown files in-place (`mdformat`)                              |
| `just lint`                  | Run `clang-tidy` static analysis + docstring lint + Python checks            |
| `just lint-c`                | Run `clang-tidy` static analysis on C source files                           |
| `just lint-py`               | Check Python scripts (`ruff` + `pyrefly`)                                    |
| `just lint-docs`             | Check Doxygen docstrings across configured dirs                              |
| `just lint-roadmap`          | Validate roadmap milestone hash IDs, DAG consistency, and markdown links     |
| `just new-milestone <slug>`  | Scaffold a new roadmap milestone writeup from template                       |
| `just check`                 | Run all pre-commit hooks across the entire repo                              |
| `just build`                 | Compile the main sandbox application binary                                  |
| `just run`                   | Build and execute the sandbox app                                            |
| `just watch`                 | Watch `.c/.h/.cpp/.py` files and auto-rerun tests                            |
| `just debug [filter]`        | Launch lldb on the test suite (optionally filtered)                          |
| `just leaks`                 | Inspect OS-level memory leaks on macOS                                       |
| `just clean`                 | Remove build binaries and gcov artifacts                                     |
| `just compiledb`             | Regenerate `compile_commands.json` for clangd                                |
| `just install-deps`          | Install all macOS dev dependencies via Homebrew                              |
| `just setup-hooks`           | Install pre-commit git hooks                                                 |

> [!NOTE]
> On macOS, `just install-deps` installs `llvm` via Homebrew which provides `clang-tidy`, but it is keg-only. You must add it to your PATH: `export PATH="/opt/homebrew/opt/llvm/bin:$PATH"`

### 3.3 Continuous Integration Pipeline

All commits and pull requests automatically trigger GitHub Actions (`.github/workflows/ci.yml`):

1. **pre-commit** — `ruff`, `pyrefly`, `clang-format`, `mdformat`, Doxygen docstring lint, and unit tests
1. **clang-tidy** — deep static analysis on `src/*.c` (`just lint-c`)
1. **build** — compile the main sandbox binary (`just build`)
1. **benchmark build** — verify benchmark compilation (`just bench-build`)
1. **coverage** — line coverage via `gcov` (`just coverage`)

______________________________________________________________________

## 🔗 4. Codebase Linking Convention

When referencing files and specific line numbers in agent responses:

- **Format**: Use markdown links with absolute `file://` URIs and numeric line hashes without the `L` prefix:
  `[<relative-path>:<line>](file:///<absolute-path>#<line>)`
  *Example*: `[src/vm.c:134](file:///Users/bwrob/dev/trash-gather/src/vm.c#134)`
- **Rationale**: In the user's editor environment, `#L<line>` anchors open the file at line 1, whereas numeric `#<line>` anchors jump directly to the target line.

______________________________________________________________________

## 📜 5. C Language Standard & Code Review Guidelines

The codebase targets **ISO C17** (`-std=c17`).

- **Portability First**: The runtime must remain strictly portable across standard C17 compilers (GCC, Clang, MSVC) without relying on compiler-specific non-standard extensions or GNU dialects.
- **Conscious Post-C99 Syntax**: Decisions to use post-C99 syntax (such as C11/C17 anonymous structs/unions, `_Static_assert`, or `_Generic`) must be **conscious, intentional, and justified** (e.g. simplifying tagged union access without compromising portability).
- **Avoid Optional / Risky Features**: Do not introduce optional or conditionally supported C11/C17 constructs (such as Variable-Length Arrays which became optional in C11, complex types, or non-portable platform assumptions).
- **Reviewer Mandate**: When reviewing code, architectural designs, or tutoring the developer, AI agents must verify that any post-C99 language features introduced are portable, intentional, and documented.
- **Follow the Skill**: All ISO C17 portability rules, memory safety invariants, SEI CERT C rules, and the 5-phase review checklist are defined in the **`c-expert`** skill: \[.agents/skills/c-expert/SKILL.md\](file:///Users/bwrob/dev/trash-gather/.agents/skills/c-expert/SKILL.md).
