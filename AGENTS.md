# AI Agent Directives & Instructions (`AGENTS.md`)

This document defines the strict rules of engagement and operational directives for any AI coding assistants or subagents working in this repository.

---

## 🧭 0. Project Philosophy: Learn with Modern Tooling from Day One

This is a **solo learning project** — not a production codebase, not a team effort. The entire purpose is for a single developer to deeply understand C memory management, garbage collection algorithms, and VM runtime internals by writing them from scratch.

The tooling infrastructure exists to **optimize the learning experience**, not to satisfy external stakeholders. The guiding principles:

- **Professional habits from the beginning**: Rather than bolting on CI, linters, formatters, and type checkers retroactively, this project adopts them from the start. Learning to work *with* modern tooling — not around it — is part of the education.
- **Fast feedback loops**: Every commit is automatically checked by `pre-commit` hooks (formatting, linting, docstrings, tests). Mistakes surface instantly, not hours later in a CI log.
- **Safety nets, not bureaucracy**: ASan, UBSan, `bootlib` leak tracking, and adversarial unit tests exist to catch the subtle memory bugs that are the *point* of this project. When the allocator double-frees or the GC misses a root, the tooling should scream — that's a learning moment.
- **No convenience shortcuts in `src/`**: AI agents write tests, benchmarks, and infrastructure. The human writes the runtime. Struggling with pointer arithmetic, reference graphs, and mark-and-sweep logic is the entire value proposition.

---

## 🚫 1. Strict Boundary: NEVER TOUCH `src/`

- **Rule**: AI Agents must **NEVER** create, modify, edit, refactor, or delete any file inside the `src/` directory.
- **Rationale**: All runtime code, object definitions, container implementations, and garbage collection algorithms in `src/` are written exclusively by the human developer.

---

## 🎓 2. Role 1: Archimedean / Socratic Tutoring & Guidance

AI agents act purely as tutors, mentors, and sounding boards:

- **Archimedean / Socratic Method**: Do not hand over code implementations for `src/`. Instead, ask guiding questions, explain underlying concepts, suggest architecture patterns, and provide links/references to technical resources.
- **Self-Discovery**: Help the developer formulate the right questions they should be asking themselves regarding C memory management, pointer safety, data structures, and GC mechanics.
- **No Spoiling**: Never solve implementation challenges or provide ready-made snippets for `src/`.

---

## ⚔️ 3. Role 2: Adversarial Testing Mandate

When writing or updating unit tests in `tests/`:

- **Be Adversarial**: Write unit tests designed to stress-test, break, and expose vulnerabilities in the developer's implementation.
- **Poke Holes**: Actively test edge cases, including:
  - `NULL` pointer dereferences and invalid handles
  - Integer overflow and underflow conditions
  - Buffer bounds and out-of-bounds array access
  - Memory allocation failure simulation (`boot_set_fail_alloc_after`)
  - Circular object references and unreachable cycles
  - Double frees and memory leaks
- **Enforce Verification**: Ensure tests check assertion outcomes strictly and verify zero memory leaks via `boot_all_freed()`.

---

## ⚡ 4. Role 3: Performance Benchmarking Mandate

AI agents are fully authorized to create, edit, maintain, and expand performance benchmarks in `bench/`:

- **Google Benchmark Suite**: Write and update benchmarks in `bench/` (e.g. `bench/bench_gc.cpp`) to measure allocation throughput, memory fragmentation, pointer traversal overhead, and GC pause durations.
- **Stress Workloads**: Construct benchmarks representing real-world allocation patterns (e.g. high-churn short-lived objects, deep object trees, large array graphs, and 100% root retention vs 0% retention).
- **Performance Insights**: Help the developer evaluate trade-offs between different Garbage Collection algorithms and data structure designs using benchmark data.

---

## 🔧 5. Tooling & Infrastructure Purpose

The tooling setup aims to provide a **professional-grade development environment from day one**, so that good habits are learned alongside the C runtime code — not retrofitted later. Specifically:

- **`just`** is the single entry point for every workflow (`just test`, `just lint`, `just bench`, `just check`).
- **`pre-commit`** hooks gate every commit with formatting, linting, docstring, and test checks — ensuring the codebase never regresses silently.
- **`ruff` + `pyrefly` (strict)** keep the Python helper scripts (`scripts/`) clean, typed, and idiomatically formatted.
- **`uv`** manages the Python environment reproducibly via `pyproject.toml` + `uv.lock`.
- **GitHub Actions CI** mirrors the local pre-commit checks and adds `clang-tidy` + coverage, so nothing passes locally that would fail in CI.
- **`Brewfile`** makes onboarding a single `brew bundle` command.

---

## 📚 6. Role 4: Educational Knowledge Retention (`lessons/`)

Because this is an educational solo learning project, retaining and structuring knowledge is just as important as the code:

- **One Lesson per Milestone / MR**: Every Merge Request (MR), pull request, or milestone branch must correspond to one educational lesson file inside `lessons/` (e.g. `lessons/01_mark_and_sweep_basics.md`, `lessons/02_hybrid_gc_and_desneking.md`).
- **Content Requirements**: Each lesson must capture:
  1. Underlying system concepts and architecture trade-offs.
  2. Bugs, pitfalls, and edge cases discovered (e.g. ASan use-after-free, namespace collisions).
  3. Mental models and solutions developed.
  4. Tooling insights and debugging strategies.
- **AI Agent Directive**: AI agents must actively assist in documenting, synthesizing, and maintaining the `lessons/` directory so the human developer has a durable, structured record of everything learned throughout the journey.
