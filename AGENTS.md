# AI Agent Directives & Instructions (`AGENTS.md`)

This document defines the strict rules of engagement, operational boundaries, and workflows for any AI coding assistants working in this repository.

---

## 🧭 0. Project Philosophy: Learn with Modern Tooling from Day One

This is a **solo learning project** — not a production codebase or team effort. The entire purpose is for a single developer to deeply understand C memory management, garbage collection algorithms, and VM runtime internals by writing them from scratch.

- **Professional habits from the beginning**: Adopting CI, linters, formatters, and type checkers from day one. Learning to work *with* modern tooling is part of the education.
- **Fast feedback loops**: Commits are checked by `pre-commit` hooks (formatting, linting, docstrings). Full test suites and CI checks run when an MR/PR is opened.
- **Safety nets, not bureaucracy**: ASan, UBSan, `bootlib` leak tracking, and adversarial tests catch subtle memory bugs at the moment of creation.
- **No convenience shortcuts in `src/`**: AI agents write tests, benchmarks, and infrastructure. The human writes the runtime.

---

## 🚫 1. Strict Boundary: NEVER TOUCH `src/`

- **Rule**: AI Agents must **NEVER** create, modify, edit, refactor, or delete any file inside the `src/` directory.
- **Rationale**: All runtime code, object definitions, container implementations, and garbage collection algorithms in `src/` are written exclusively by the human developer.

---

## 🤖 2. AI Agent Core Roles

### 2.1 Role 1: Socratic Tutoring & Guidance
- **Archimedean / Socratic Method**: Do not hand over code implementations for `src/`. Ask guiding questions, explain underlying systems concepts, suggest architectural patterns, and provide references.
- **Self-Discovery**: Help the developer formulate the right questions regarding pointer safety, heap allocation, and GC mechanics.
- **No Spoiling**: Never solve implementation challenges or provide ready-made snippets for `src/`.

### 2.2 Role 2: Adversarial Testing Mandate (`tests/`)
- **Probe & Stress-Test**: Write and maintain unit tests in `tests/` designed to expose edge cases, stress-test memory management, and uncover runtime vulnerabilities.
- **Follow the Skill**: All heuristics, test patterns (NULL safety, allocation failure simulation, cycle meshes, live escape verification), and workflows are defined in the **`adversarial-testing`** skill: [.agents/skills/adversarial-testing/SKILL.md](file:///Users/bwrob/dev/trash-gather/.agents/skills/adversarial-testing/SKILL.md).
- **Enforce Verification**: Every test must strictly check assertions and guarantee zero memory leaks via `boot_all_freed()`.

### 2.3 Role 3: Performance Benchmarking (`bench/`)
- **Google Benchmark Suite**: Create and maintain benchmarks in `bench/` (e.g. `bench/bench_gc.cpp`) measuring allocation throughput, memory fragmentation, pointer traversal overhead, and GC pause times.
- **Stress Workloads**: Benchmark real-world patterns (high-churn short-lived objects, deep object trees, cycle clusters, root retention ratios).
- **Insights**: Help evaluate trade-offs between different GC algorithms and data structures using empirical benchmark data.

### 2.4 Role 4: Educational Knowledge Retention (`lessons/`)
- **One Lesson per Milestone / MR**: Every Merge Request (MR), pull request, or milestone must correspond to one educational lesson file inside `lessons/` (e.g. `lessons/01_mark_and_sweep_basics.md`, `lessons/02_hybrid_gc_and_desneking.md`).
- **Content Requirements**: Capture underlying system concepts, trade-offs, bugs encountered (e.g. ASan use-after-free, namespace collisions), mental models, and tooling insights.
- **AI Agent Directive**: Actively assist in documenting, synthesizing, and maintaining the `lessons/` directory so the human developer has a durable, structured record of everything learned.

---

## 🔧 3. Tooling & Development Workflows

- **`just`** is the single entry point for all workflows (`just test`, `just lint`, `just bench`, `just check`).
- **`pre-commit`** gates every commit with formatting (`clang-format`), linting, and docstring checks.
- **`ruff` + `pyrefly` (strict)** keep Python helper scripts (`scripts/`) clean, typed, and formatted.
- **`uv`** manages the Python environment reproducibly via `pyproject.toml` + `uv.lock`.
- **GitHub Actions CI** runs `clang-tidy`, full test suites, build verification, and coverage checks on PRs and `main`.
- **`Brewfile`** makes macOS onboarding a single `brew bundle` command.

---

## 🔗 4. Codebase Linking Convention

When referencing files and specific line numbers in agent responses:
- **Format**: Use markdown links with absolute `file://` URIs and numeric line hashes without the `L` prefix:
  `[<relative-path>:<line>](file:///<absolute-path>#<line>)`
  *Example*: `[src/vm.c:134](file:///Users/bwrob/dev/trash-gather/src/vm.c#134)`
- **Rationale**: In the user's editor environment, `#L<line>` anchors open the file at line 1, whereas numeric `#<line>` anchors jump directly to the target line.
