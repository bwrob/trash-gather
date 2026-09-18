---
name: initialize
description: >-
  Orchestrates the pre-flight readiness pipeline when starting a roadmap milestone:
  assures the feature branch, sets status to in progress, presents the point of the goal
  and learning aims first with memory diagrams and physical invariants, unconditionally stops,
  and only then conducts the interactive systems pre-check upon user response.
---

# Milestone Pre-Flight Readiness & Initialization (`initialize`)

This skill defines the structured pre-flight protocol executed whenever the developer selects or begins a new roadmap milestone. It ensures that the branch is ready, status is recorded, prerequisites are satisfied, provides a crisp physical/mathematical mental model with the point of the goal, learning aims, and before-goal references, **unconditionally stops** to let the developer absorb the foundation, and only then conducts an open-ended pre-check in the subsequent turn before any code is written in `src/`.

______

## 🧭 Pre-Flight Protocol Overview

The `initialize` workflow consists of 4 sequential phases:

```text
[Phase 1: DAG Audit & Branch Assurance]
  └──> [Phase 2: Milestone Status Update]
        └──> [Phase 3: Goal Point, Learning Aims, Memory Layout & Invariants]
              └──> [MANDATORY UNCONDITIONAL STOP: Developer Reviews Foundation]
                    └──> [Phase 4 (Subsequent Turn): Pre-Check Opt-In & Inquiry]
```

______

## 1. Phase 1: DAG Fit & Branch Assurance

Before starting work on any milestone:

1. **Prerequisite Verification**:
   - Inspect the target milestone file in `roadmap/<hash>_<slug>.md`.
   - Read the `**Prerequisites:**` line.
   - Verify in `roadmap/README.md` that all listed prerequisite milestones are marked with `✅ Completed` (or `[x]`).
   - **Halt on Missing Prerequisite**: If any prerequisite is incomplete, explicitly warn the developer and propose completing the missing prerequisite first.
1. **Topological Fit Evaluation**:
   - Check the milestone's Difficulty Level (1 to 5) and Tier.
   - Confirm that this milestone is the most appropriate next step in the pedagogical sequence (easiest unblocked milestones first).
1. **Branch Assurance**:
   - Check current git branch with `git branch --show-current`.
   - Assure the correct feature branch `milestone/<hash>-<slug>` is checked out:
     - If the branch already exists, switch to it:

       ```bash
       git switch milestone/<hash>-<slug>
       ```

     - If it does not exist, create and switch to it:

       ```bash
       git switch -c milestone/<hash>-<slug>
       ```

______

## 2. Phase 2: Milestone Status Update

Record the active milestone in the roadmap tracking files:

1. **Milestone File**:
   - Update `roadmap/<hash>_<slug>.md`: Set `**Status:** In Progress`.
1. **Roadmap Index & DAG**:
   - In `roadmap/README.md`, update the milestone entry to `⏳ In Progress`.
   - Update the Mermaid diagram node class to `:::inProgress`.
   - Run `just lint-roadmap` to verify that all links resolve and the Mermaid DAG remains 100% synchronized.

______

## 3. Phase 3: Goal Point, Learning Aims, Memory Layout & Invariants

Provide a high-density, comprehensive structural breakdown of what is being built, strictly following the external template in [resources/briefing_template.md](./resources/briefing_template.md) framed around **The Pedagogical Point**, **Physical Machine Mechanics**, **Mathematical Induction**, and **Foundation Reading**:

1. **The Point of the Goal & Learning Aims (Pedagogical Purpose)**:
   - State clearly and concisely *why* this milestone exists in the pedagogical sequence.
   - Articulate the specific systems programming, memory safety, and VM runtime mental models the developer is set to master.
1. **Concise Technical Scope & Core API Contract**:
   - High-level objective and explicit scope boundaries.
   - List the exact function prototypes to be declared in header files, documenting parameter ownership contracts (borrowed vs. owned) and return values.
1. **Explicit Byte-Offset Memory Diagram**:
   - Provide an ASCII memory layout diagram illustrating exact byte offsets, field sizes, alignment holes (padding), and pointer directions (following the schema in [resources/briefing_template.md](./resources/briefing_template.md)).
1. **Physical Machine Mechanics (What the Silicon Does)**:
   - Detail what the physical hardware and OS do under the hood:
     - **CPU Word Alignment & ABI Conventions**: Register passing, stack frame spilling, and alignment boundaries.
     - **Cache Line Chunking (64 Bytes)**: Contiguous struct/array memory layout vs cache misses.
     - **MMU & Virtual Memory**: Allocation behavior and page boundaries.
1. **Inductive Invariant Modeling**:
   - **Base Invariant $\\mathcal{I}\_0$**: The system state at allocation / initialization (e.g. slots zero-initialized to `NULL`, refcount initialized to 1, object safely tracked in VM).
   - **Inductive Step $\\mathcal{I}_k \\implies \\mathcal{I}_{k+1}$**: How state mutations strictly preserve reference parity, pointer validity, and reachability.
   - **Rollback Invariant $\\mathcal{I}\_{\\text{rollback}}$**: Proving that if step $k \\in {0, \\dots, N-1}$ fails, steps ${0, \\dots, k-1}$ are cleanly unwound with zero leaks and no double-free.
1. **Incremental Micro-Loop Decomposition**:
   - For multi-operation milestones, decompose into sequential micro-loops: implement operation $A$ in `src/` $\\to$ test $A$ $\\to$ achieve green $\\to$ proceed to operation $B$.
1. **Before-Goal References & Targeted Book Readings (`.sources/`)**:
   - **External & Standard Specifications**: Extract and prominently present the **"Before Implementation (Conceptual Foundations)"** references from Section 6 of the milestone file (ISO C standard rules, SEI CERT C rules, CPython references, and ABI specifications).
   - **Source Book Indexing (`.sources/INDEX.md`)**: Check if `.sources/` exists. If `.sources/INDEX.md` does not exist, create it following \[`.agents/skills/initialize/resources/index_template.md`\](file:///Users/bwrob/dev/trash-gather/.agents/skills/initialize/resources/index_template.md) to index all available volumes with chapter outlines and descriptions.
   - **Targeted Reading Recommendations**: Cross-reference the active milestone's technical concepts against `.sources/INDEX.md`. Present specific book titles, chapter numbers, section titles, and exact page ranges (or Level/Chapter identifiers) that the developer should read to acquire the requisite mental capital before writing C code.
1. **MANDATORY: Unconditional Stop**:
   - The AI agent **MUST UNCONDITIONALLY STOP** at the end of this message.
   - **NEVER** call `ask_question` in this turn.
   - **NEVER** pose pre-check questions or trivia in this initial turn.
   - Conclude the message by inviting the developer to review the mental model and let the agent know when they are ready to proceed with the interactive systems pre-check (or skip straight to implementation).

______

## 4. Phase 4: Pre-Check Opt-In & Open-Ended Inquiry (Subsequent Turn ONLY)

This phase executes **only after the developer responds** to the Phase 3 presentation:

### 4.1 Opt-In Check

If the developer has not explicitly stated their preference in their response, ask whether to run the pre-check:

- "Would you like to proceed with an interactive systems pre-check, or skip straight to implementation?"
- If the developer chooses to skip, immediately hand off execution to the human developer for **Step 1: Code Must Compile** (`just build`).

### 4.2 Open-Ended Pre-Check (NOT Multiple Choice)

If the developer opts in to the pre-check:

- **Strict Rule: ONE Question at a Time**:
  - The AI agent **MUST NEVER** bundle multiple questions together into a single message.
  - Limit the pre-check to at most 2–3 questions total.
- **Format: Open-Ended in Regular Text (No Multiple Choice)**:
  - Do NOT provide multiple-choice options or lettered choices.
  - Present the question directly in message text, ending the turn so the user can type their reasoning in their own words.
- **Physical Consequence Focus (No Rote Trivia)**:
  - Questions must test understanding of concrete machine consequences:
    - *Example (Calling Convention)*: "If a caller invokes a variadic function passing fewer arguments than the count parameter specifies, what physically happens inside the CPU registers and stack frame when `va_arg` executes?"
    - *Example (Pointer Invalidation)*: "If `realloc` is forced to allocate a new memory page elsewhere, what physically happens to external pointers pointing to items within the old buffer?"
    - *Example (Memory Alignment)*: "What does the CPU memory controller do when an 8-byte integer read is executed at address `0x1003`?"
- **Adaptive Feedback**:
  - **If the developer demonstrates clear mastery**: Acknowledge their explanation and ask the next question or greenlight implementation: *"You have full mental capital for this goal. Jump straight in! Once compiling cleanly, I'll generate the adversarial test suite."*
  - **If the developer indicates uncertainty or gaps**: Provide a **2-Minute Physical Mental Model** explaining what the hardware does with an ASCII diagram, then ask the next question or conclude.
- **Handoff**:
  - Conclude the pre-check and hand off execution to the human developer for **Step 1: Code Must Compile** (`just build`).
