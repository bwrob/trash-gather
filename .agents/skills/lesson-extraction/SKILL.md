---
name: lesson-extraction
description: >-
  Provides a structured rubric and workflow for documenting and synthesizing educational
  engineering lessons in lessons/ (e.g. lessons/01_*.md) upon milestone completion, major
  refactors, or subtle bug resolutions. Use whenever creating, updating, or reviewing lessons.
---

# Educational Lesson Extraction Skill (`lesson-extraction`)

This skill defines the process, structure, and quality standards for extracting and maintaining **educational engineering lessons** in the `lessons/` directory.

In this project, capturing deep systems understanding is just as important as writing the code. Every milestone branch, Merge Request (MR), or significant debugging breakthrough must leave behind a durable, structured record of **transferable engineering principles**.

---

## 🎯 Extraction Triggers

Activate this skill and create/update a lesson when:
1. **Milestone / MR Finalization**: Preparing to open, merge, or conclude a pull request or milestone branch.
2. **Subtle Bug Diagnosis**: Resolving tricky runtime failures (e.g., ASan `heap-use-after-free`, double frees, uncollected cyclic leaks).
3. **Architectural Pivots**: Transitioning runtime paradigms (e.g., pure Mark-and-Sweep $\rightarrow$ Hybrid Reference Counting, explicit VM arguments $\rightarrow$ global singleton).
4. **Systems & OS Discoveries**: Uncovering C compiler quirks, POSIX collisions, or tooling breakthroughs.

---

## 🧭 Guiding Philosophy: Transferable Wisdom Over Codebase Trivia

A lesson must **not** read like an ephemeral commit message or a blow-by-blow narrative of line edits. Instead:
- **Focus on Universal Principles**: Frame every problem around transferable computer science concepts, systems programming patterns, memory management trade-offs, and compiler/OS invariants.
- **Audience**: Write the lesson so that a systems engineer building an allocator, VM, or garbage collector in C, Rust, or Zig would find immediate, durable value in reading it.
- **Mental Models**: Clearly state the broken invariant and the architectural mental model that resolved it. Anchor principles with concrete examples without getting lost in trivialities.

---

## 📁 File Naming & Indexing

- **Path Pattern**: `lessons/NN_<topic_slug>.md`
  - Zero-padded two-digit sequence number: `01`, `02`, `03`, etc.
  - Descriptive lowercase slug: `01_mark_and_sweep_basics.md`, `02_hybrid_gc_and_desneking.md`.
- **Index Maintenance**:
  - Every new lesson must be added to the index table in `lessons/README.md` with:
    - Sequence number (`**03**`)
    - Markdown link to the file
    - Key concepts explored

---

## 📝 Lesson Structure & Template

Every lesson must follow this standard structure:

```markdown
# Lesson NN: <Topic Title>

**Branch:** `<branch-name>` (Merged in PR #X)
**Focus:** <1-2 sentence core technical objective>

---

## 1. System Engineering & Core Concepts
- Explain the underlying computer science or systems programming concept.
- Highlight pointer mechanics, memory layouts, or graph reachability rules.
- Note platform or C-specific quirks (e.g., namespace limitations, POSIX header collisions, alignment).

---

## 2. Pitfalls, Failure Modes & Diagnosis
- Detail the exact failure mode or theoretical dilemma.
- Include concrete AddressSanitizer, UBSan, or compiler error diagnostics.
- Explain *why* naive or intuitive approaches inevitably fail (e.g., why single-pass iteration cannot deallocate arbitrary parent/child graphs).

---

## 3. Architectural Solutions & Mental Models
- Provide the mental model or invariant that solved the problem.
- Explain trade-offs between alternative designs (e.g., throughput vs. pause times, memory footprint vs. code complexity).
- Diagram state transitions, phase separations, or multi-pass workflows.

---

## 4. Performance & Systems Optimization (Optional/When Applicable)
- Hot path vs. cold path trade-offs (e.g., where inlining belongs in refcounted systems).
- Cache locality, instruction cache footprint, and allocator overhead.

---

## 5. Tooling Insights & Workflow Takeaways
- What role did the tooling play in accelerating discovery? (e.g., ASan shadow bytes, `bootlib` leak tracking, `just test-filter`).
- What testing heuristics or assertions caught the issue?
```

---

## ✅ Quality Rubric

Before finalizing a lesson, verify that it meets these standards:
- [ ] **Transferable, Not Localized**: Focuses on durable systems engineering principles (e.g., decoupling payload release from reference clearing, POSIX Darwin namespace collisions, unsigned loop underflow) rather than superficial code changes.
- [ ] **Defines the Invariants**: Clearly identifies what invariants broke and what invariants restored correctness.
- [ ] **Diagnostic Fidelity**: Quotes real sanitizer errors or test failures that guided the investigation.
- [ ] **Explains the "Why"**: Compares architectural alternatives and explains the performance/safety trade-offs.
- [ ] **Formatting**: Markdown is clean and formatted compliant with pre-commit checks (`just check`).
