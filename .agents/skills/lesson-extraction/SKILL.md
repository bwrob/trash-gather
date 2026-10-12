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

______

## 🎯 Extraction Triggers

Activate this skill and create/update a lesson when:

1. **Milestone / MR Finalization**: Preparing to open, merge, or conclude a pull request or milestone branch.
1. **Subtle Bug Diagnosis**: Resolving tricky runtime failures (e.g., ASan `heap-use-after-free`, double frees, uncollected cyclic leaks).
1. **Architectural Pivots**: Transitioning runtime paradigms (e.g., pure Mark-and-Sweep $\\rightarrow$ Hybrid Reference Counting, explicit VM arguments $\\rightarrow$ global singleton).
1. **Systems & OS Discoveries**: Uncovering C compiler quirks, POSIX collisions, or tooling breakthroughs.

______

## 🧭 Guiding Philosophy: Transferable Wisdom Over Codebase Trivia

A lesson must **not** read like an ephemeral commit message or a blow-by-blow narrative of line edits. Instead:

- **Focus on Universal Principles**: Frame every problem around transferable computer science concepts, systems programming patterns, memory management trade-offs, and compiler/OS invariants.
- **Audience**: Write the lesson so that a systems engineer building an allocator, VM, or garbage collector in C, Rust, or Zig would find immediate, durable value in reading it.
- **Mental Models**: Clearly state the broken invariant and the architectural mental model that resolved it. Anchor principles with concrete examples without getting lost in trivialities.

______

## 📁 File Naming & Indexing

- **Path Pattern**: `lessons/NN_<topic_slug>.md`
  - Zero-padded two-digit sequence number: `01`, `02`, `03`, etc.
  - Descriptive lowercase slug: `01_mark_and_sweep_basics.md`, `02_hybrid_gc_and_desneking.md`.
- **Index Maintenance**:
  - Every new lesson must be added to the index table in `lessons/README.md` with:
    - Sequence number (`**03**`)
    - Markdown link to the file
    - Key concepts explored

______

## 📝 Lesson Structure & Template

Every lesson must strictly follow the schema and sections defined in the external template:

- **Template Path**: [resources/template.md](./resources/template.md)
- **Mandatory Sections**:
  1. `## 1. System Engineering & Core Concepts`
  1. `## 2. Pitfalls, Failure Modes & Diagnosis`
  1. `## 3. Architectural Solutions & Mental Models`
  1. `## 4. Hardware & Silicon Mechanics (What the Machine Did)`
  1. `## 5. Tooling Insights & Workflow Takeaways`
  1. `## 6. Recommended Reading & Literature References`

______

## 🔍 Literature RAG Workflow

When synthesizing a lesson, grounding engineering concepts in canonical systems textbooks is strictly required:

1. **Query Local Literature RAG**:
   - Use the `search_literature` MCP tool or terminal command `just rag-search "<concept>"` (optionally `--book <slug>`) to retrieve verbatim definitions, algorithms, and discussions from the local corpus (`resources/` indexed in LanceDB `.rag_db/`).
   - Use `get_book_toc("<slug>")` and `get_book_page("<slug>", <page>)` to inspect context and exact page numbers.
2. **Canonical Literature Mapping**:
   - **Garbage Collection & Memory Management**: *The Garbage Collection Handbook* (Jones, Hosking & Moss) and *Crafting Interpreters* (Nystrom).
   - **C Memory Layout & Pointers**: *Pointers on C* (Reek) and *Effective C* (Seacord).
   - **C Standards & Language Invariants**: *Modern C* (Gustedt).
   - **Hardware Systems, Caches & Virtual Memory**: *Computer Systems: A Programmer's Perspective* (Bryant & O'Hallaron).
   - **UNIX Runtime & System Programming**: *Advanced Programming in the UNIX Environment* (Stevens & Rago).
3. **Mandatory Citations in Section 6**:
   - Every lesson must include relative markdown links pointing directly into `../resources/<filename>` with exact section titles and page numbers (e.g. `[Richard Jones et al. — *The Garbage Collection Handbook* (CRC Press, 2023)](../resources/jones_2023_garbage_collection_handbook.pdf): Chapter 2, §2.2, pp. 59–60`).

______

## ✅ Quality Rubric

Before finalizing a lesson, verify that it meets these standards:

- [ ] **Transferable, Not Localized**: Focuses on durable systems engineering principles (e.g., decoupling payload release from reference clearing, POSIX Darwin namespace collisions, unsigned loop underflow) rather than superficial code changes.
- [ ] **Defines the Invariants**: Clearly identifies what invariants broke and what invariants restored correctness.
- [ ] **Diagnostic Fidelity**: Quotes real sanitizer errors or test failures that guided the investigation.
- [ ] **Explains the "Why"**: Compares architectural alternatives and explains the performance/safety trade-offs.
- [ ] **Hardware Intuition**: Documents the underlying physical machine mechanics (cache lines, CPU word alignment, branch predictor, or virtual memory paging).
- [ ] **Literature RAG Grounding**: Actively queries the local RAG store (`just rag-search` or `search_literature`) to anchor concepts to canonical literature. Section 6 contains verified links to `resources/` with exact chapters and page ranges.
- [ ] **New External References**: If introducing a new textbook or paper not currently in `resources/`, download it using `libgen-resource` (`uv run python .agents/skills/libgen-resource/scripts/libgen_fetch.py download "<Title>"`), register it in `resources/references.bib`, and index it via `just rag-index`.
- [ ] **Formatting**: Markdown is clean and formatted compliant with pre-commit checks (`just check`).
