---
name: roadmap-milestone
description: >-
  Provides a standardized workflow and uniform schema for defining, structuring, and maintaining
  project roadmap milestones in roadmap/ (e.g. roadmap/NN_*.md). Use whenever planning new architectural
  milestones, decomposing complex features into sequential steps, or updating roadmap status.
---

# Roadmap Milestone Planning Skill (`roadmap-milestone`)

This skill defines the process, structure, and quality standards for creating, updating, and maintaining **project roadmap milestones** in the `roadmap/` directory.

In this learning project, complex systems engineering challenges (like moving to variable-sized objects, generational collection, or Python-style tuples) must be decomposed into small, incremental, verifiable milestones. Every milestone must be self-contained, testable, and preserve a passing test suite at every step.

______________________________________________________________________

## 🎯 Milestone Creation Triggers

Activate this skill and create or update roadmap milestones when:

1. **Decomposing Major Features**: Breaking down high-level architectural goals into incremental stepping stones.
1. **Planning Architectural Refactors**: Structuring breaking changes into non-breaking, intermediate phases.
1. **Milestone Progression**: Marking completed milestones, refining upcoming steps based on lessons learned, or adjusting future milestones.

______________________________________________________________________

## 🧭 Guiding Philosophy: Incrementalism & Verifiability

1. **Pedagogical Ordering & DAG Placement**: New milestones must never be arbitrarily appended to the end of the roadmap. Every milestone must be intentionally situated within the curriculum DAG:
   - Identify its educational track (e.g. Object Model, CPython Memory Hierarchy, Collector Evolution, Developer Tools).
   - Determine its exact prerequisite foundations (what concepts and runtime invariants must exist before this goal is approachable for a beginner?).
   - Position the node in the Mermaid DAG in `roadmap/README.md` and connect both incoming and outgoing dependency edges.
   - Maintain a gentle learning slope, ensuring complexity advances incrementally without overwhelming jumps in abstraction.
1. **Enumerated Progression**: Roadmaps must always use enumerated lists (`1.`, `2.`, `3.`), never unstructured bullet points. Learning systems programming requires a clear, ordered sequence of operations.
1. **Small, Verifiable Steps**: Each milestone must be small enough to implement and verify within a single focused session.
1. **Zero-Regression Invariant**: Every milestone must keep existing tests passing (`just test`, `boot_all_freed()`, `just lint`, `just check`).
1. **Socratic Integration**: Milestones provide guiding questions, invariant checklists, and architectural boundaries rather than handing over ready-made snippets for `src/`.

______________________________________________________________________

## 📁 File Naming & Indexing

1. **Path Pattern**: `roadmap/<hash>_<milestone_slug>.md`
   - Stable 7-character hexadecimal hash: `hashlib.sha256(slug.encode()).hexdigest()[:7]`.
   - Descriptive lowercase slug: `d9c6780_learning_friendly_setup.md`, `f9c475f_heap_allocated_variable_length_tuple.md`.
   - **Order Decoupling**: Sequence order lives exclusively in `roadmap/README.md` and the root `README.md`. Writeup filenames and titles never hardcode milestone sequence numbers, making reordering and inserting friction-free.
1. **Single Source of Truth (`roadmap/README.md`)**:
   - `roadmap/README.md` is the authoritative single source of truth for the project roadmap.
   - It defines the **pedagogical Directed Acyclic Graph (DAG)** showing tracks and milestone prerequisite dependencies via a Mermaid diagram (`flowchart TD`).
   - Every milestone is cataloged with its title, persistent hash ID, status (`✅ Completed`, `🚧 In Progress`, `📋 Planned`), and technical focus.
1. **Top-Level README Linkage**:
   - The top-level `README.md` does not duplicate the milestone list; it maintains a concise roadmap summary that links directly to `roadmap/README.md`.
1. **Automation & Quality Tooling**:
   - **Scaffolding**: Run `just new-milestone <slug> "[Title]"` to automatically compute the deterministic 7-character sha256 hash ID and instantiate the template in `roadmap/<hash>_<slug>.md`.
   - **Linting**: Run `just lint-roadmap` to validate hash integrity, metadata presence, 5-section schema compliance, link validity, and Mermaid DAG representation. This check runs automatically in `just lint` and `just check`.

______________________________________________________________________

## 📄 Standalone Template Reference

The reference template file is stored at:

- `resources/template.md`

All milestone writeups must be strictly aligned with this template schema.

______________________________________________________________________

## 📝 Detailed Goal Writeup Format Specification

Every milestone file in `roadmap/` must follow this exact 5-section schema:

### Header & Metadata Block

```markdown
# Milestone: <Milestone Title>

**ID:** `<hash_id>`
**Status:** Planned | In Progress | Completed
**Focus:** <1-2 sentence core technical objective summarizing the engineering goal>
**Prerequisites:** [<Milestone Title>](<hash_id>_<slug>.md) | None

______________________________________________________________________
```

### Section 1: Objective & Technical Scope

Defines the technical boundaries of the milestone:

1. **Primary Goals**: Concrete enumerated deliverables (what is built, modified, or solved).
1. **Scope Boundaries**: Explicit non-goals (what is intentionally deferred to future milestones).

### Section 2: Architectural Design & Invariants

Documents the memory structures, graphs, and invariants:

1. **Memory Layout & Pointer Graph**: Detailed data structures, struct definitions, ASCII diagrams, or memory models.
1. **Core Systems Invariants**: Rules that must never be violated (e.g. NULL safety, lifecycle ownership, allocation boundaries, tracking registration).
1. **Architectural Trade-offs**: Analysis of design trade-offs (e.g. CPU vs memory, cache locality vs indirection, complexity vs safety).

### Section 3: Systems Concepts & Guiding Questions

Frames the educational and theoretical foundation:

1. **Underlying Theory**: Key computer science and systems programming principles (e.g. tri-color marking, weak generational hypothesis, open addressing).
1. **Socratic Inquiries**: Guiding questions to prompt self-discovery on failure modes, undefined behavior, and edge cases.
1. **Failure Modes & Pitfalls**: Common traps (e.g. use-after-free, memory fragmentation, uninitialized slots).

### Section 4: Implementation Steps & Touchpoints

Enumerates the concrete development path:

1. **Step-by-Step Execution Sequence**: Ordered, incremental tasks to implement the milestone.
1. **File Touchpoints**: Exact file paths affected across `src/`, `include/`, `tests/`, and `bench/`.

### Section 5: Verification & Acceptance Criteria

Defines empirical validation gates:

1. **Unit & Adversarial Tests**: Specific tests in `tests/` to write or run (boundary conditions, stress tests, failure simulations).
1. **Zero-Leak Guarantee**: Explicit assertion that all allocations are tracked and confirmed freed via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test` (100% pass with ASan/UBSan), `just lint` (`clang-tidy` + docstrings + roadmap linter), `just check` (all pre-commit hooks).
1. **Milestone Completion & Lesson Extraction**: Explicit protocol triggered upon completion: update status in writeup and `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate a durable lesson in `lessons/` via `lesson-extraction`.

______________________________________________________________________

## ✅ Quality Rubric

Before finalizing a roadmap milestone, verify:

1. **Pedagogical Placement**: Is the milestone placed thoughtfully into the DAG with its conceptual prerequisites satisfied, maintaining an approachable learning curve?
1. **Enumeration**: Is every list in the roadmap file enumerated (`1.`, `2.`, etc.) rather than bulleted?
1. **Self-Containment**: Can this milestone be completed and verified on its own without breaking the master build?
1. **Uniform Format**: Does the milestone match the exact template above, with all 5 numbered sections present?
1. **Index & DAG Updated**: Is the milestone registered in `roadmap/README.md` and connected with incoming/outgoing edges in the Mermaid DAG?
1. **Roadmap Linter Passing**: Does `just lint-roadmap` pass with zero errors?
1. **Single Source of Truth**: Does the root `README.md` link directly to `roadmap/README.md` without duplicating the milestone list?
1. **Status Emojis**: Are completed milestones marked with `✅ Completed` in `roadmap/README.md`?
