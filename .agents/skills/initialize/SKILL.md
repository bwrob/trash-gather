---
name: initialize
description: >-
  Orchestrates the pre-flight readiness pipeline when starting a roadmap milestone:
  verifies DAG prerequisites, formats concise logic with mathematical inductive invariants,
  and conducts an interactive systems knowledge pre-check one question at a time using ask_question.
---

# Milestone Pre-Flight Readiness & Initialization (`initialize`)

This skill defines the structured pre-flight protocol executed whenever the developer selects or begins a new roadmap milestone. It ensures that prerequisites are satisfied, provides a crisp mathematical mental model, and assesses the developer's "mental capital" before any code is written in `src/`.

______________________________________________________________________

## 🧭 Pre-Flight Protocol Overview

The `initialize` workflow consists of 4 sequential phases:

```
[Phase 1: DAG Fit Audit] ──> [Phase 2: Mathematical Invariants] ──> [Phase 3: Interactive Pre-Check (1-at-a-time)] ──> [Phase 4: Branch Setup]
```

______________________________________________________________________

## 1. Phase 1: DAG Fit & Prerequisite Audit

Before starting work on any milestone:

1. **Prerequisite Verification**:
   - Inspect the target milestone file in `roadmap/<hash>_<slug>.md`.
   - Read the `**Prerequisites:**` line.
   - Verify in `roadmap/README.md` that all listed prerequisite milestones are marked with `✅ Completed` (or `[x]`).
   - **Halt on Missing Prerequisite**: If any prerequisite is incomplete, explicitly warn the developer and propose completing the missing prerequisite first.
1. **Topological Fit Evaluation**:
   - Check the milestone's Difficulty Level (1 to 5) and Tier.
   - Confirm that this milestone is the most appropriate next step in the pedagogical sequence (easiest unblocked milestones first).

______________________________________________________________________

## 2. Phase 2: Concise Logic, Invariants & Hardware Mechanics

Provide a high-density, concise structural breakdown of what is being built, framed around **Mathematical Induction** and **Physical Machine Mechanics**:

1. **Explicit Byte-Offset Memory Diagram**:
   - Provide an ASCII memory layout diagram illustrating exact byte offsets, field sizes, alignment holes (padding), and pointer directions:
     ```
     Byte Offset:
     +00 [ 8 bytes: refcount   ] (uint64_t)
     +08 [ 2 bytes: type       ] (uint16_t)
     +10 [ 2 bytes: flags      ] (uint16_t bitflags)
     +12 [ 4 bytes: CPU PADDING] (Hardware alignment hole to 8-byte boundary!)
     +16 [ 8 bytes: payload ptr] (pointer to heap buffer)
     Total: 24 bytes (strictly 8-byte aligned for 64-bit CPU bus)
     ```
1. **Physical Machine Mechanics (What the Hardware Does)**:
   - Detail what the physical silicon and operating system are doing under the hood:
     - **CPU Word Alignment**: Why unaligned reads trigger multi-cycle penalties or hardware bus traps.
     - **Cache Line Chunking (64 Bytes)**: How contiguous struct fields and flat array payloads minimize cache line misses.
     - **MMU & Virtual Memory**: How 4 KB page boundaries, bitmasks, and address translation work physically in the memory controller.
1. **Inductive Invariant Modeling**:
   - **Base Invariant $\\mathcal{I}\_0$**: The system state at allocation / initialization (e.g. all slots zero-initialized to `NULL`, refcount initialized to 1, object safely tracked in VM).
   - **Inductive Step $\\mathcal{I}_k \\implies \\mathcal{I}_{k+1}$**: How every state mutation (appending an element, slicing, acquiring a reference, resizing a buffer) strictly preserves reference parity, pointer validity, and reachability.
   - **Rollback Invariant $\\mathcal{I}\_{\\text{rollback}}$**: Proving that if step $k \\in {0, \\dots, N-1}$ in a multi-stage allocation fails, steps ${0, \\dots, k-1}$ are cleanly and completely unwound with zero memory leaks and no double-free.
1. **Incremental Micro-Loop Decomposition**:
   - For multi-operation milestones (e.g. `append` $\\to$ `insert` $\\to$ `pop`), decompose the technical scope into sequential micro-loops. Each micro-loop is self-contained: write $A$, generate adversarial test for $A$, achieve green, then move to $B$.
1. **Core API Contract**:
   - List the exact signatures to be declared in the header file with their parameter ownership contracts (borrowed vs. owned).

______________________________________________________________________

## 3. Phase 3: Interactive Knowledge & "Mental Capital" Pre-Check

Assess whether the developer has sufficient mental capital to jump straight in, or would benefit from a 2-minute systems primer.

### ⚠️ Strict Rule: ONE Question at a Time & Physical Consequence Focus

- The AI agent **MUST NEVER** bundle multiple questions together into a single tool call or output a list of questions.
- Every diagnostic inquiry must use the `ask_question` tool with **exactly ONE question** (`questions` array of length 1).
- Wait for the user's response to the first question before formulating or asking the next question (if needed).
- Limit the pre-check to at most 2–3 questions total.
- **Physical Consequence Scenarios (No Rote Trivia)**: Questions must pose practical machine consequences rather than dry C syntax rules:
  - *Example (Pointer Invalidation)*: "If `realloc` is forced to move a memory block to a new address, what physically happens to other pointers that were previously pointing to items inside the old buffer?"
  - *Example (Alignment Trap)*: "On ARM64 or x86_64, what does the memory controller do if an 8-byte pointer is dereferenced at address `0x1003` instead of an 8-byte boundary?"
  - *Example (Bitwise Arithmetic)*: "What physical value remains in a `uint8_t` register when shifting `0xFF` left by 8 bits?"

### Pre-Check Focus Areas:

- C pointer mechanics and arithmetic (e.g. `realloc` pointer invalidation, `sizeof` on flexible array members).
- Memory alignment and hardware contracts (e.g. 4 KB page boundaries, bitmasks, struct padding).
- Translation unit linkage and storage durations (e.g. `extern` vs. `static const`).
- Failure recovery idioms (`goto cleanup`).

### Adaptive Response:

- **If the developer demonstrates clear mastery**: Greenlight immediate implementation: *"You have full mental capital for this goal. Jump straight in! Once compiling cleanly, I'll generate the adversarial test suite."*
- **If the developer indicates uncertainty or gaps**: Provide a **2-Minute Physical Mental Model**:
  - Explain the physical machine contract (CPU word alignment, RAM page offset arithmetic, or OS memory guarantees).
  - Use an ASCII memory map to remove any guesswork before they touch `src/`.

______________________________________________________________________

## 4. Phase 4: Milestone Setup & Branch Preparation

Once the pre-check is complete:

1. Update the milestone file: Set `**Status:** In Progress`.
1. Update `roadmap/README.md`: Mark milestone as `⏳ In Progress` (class `:::inProgress` in Mermaid DAG).
1. Offer to create or switch to the feature branch:
   ```bash
   git switch -c milestone/<hash>-<slug>
   ```
1. Hand off execution to the human developer for **Step 1: Code Must Compile** (`just build`).
