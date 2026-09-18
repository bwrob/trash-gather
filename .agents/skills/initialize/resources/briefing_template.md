# Milestone Pre-Flight Briefing Template (`.agents/skills/initialize/resources/briefing_template.md`)

This template defines the mandatory output structure for the Phase 3 pre-flight presentation during milestone initialization.

______

````markdown
## 🚀 Milestone Pre-Flight: <Milestone Title> (`<hash_id>`)

### 1. Pre-Flight Status & Branch Assurance
- **Prerequisites**: `<prerequisite_milestone>` is marked `✅ Completed`.
- **Status**: Updated to `⏳ In Progress` in `roadmap/<hash_id>_<slug>.md` and `roadmap/README.md`.
- **Working Branch**: Assured on `milestone/<hash_id>-<slug>`.

______________________________________________________________________

### 2. The Point of the Goal & Learning Aims
- **The Pedagogical Point**: <Why this milestone exists in the curriculum sequence and what problem it solves.>
- **Core Learning Aims**: <Specific systems programming concepts, memory safety invariants, and VM runtime mental models being mastered.>

______________________________________________________________________

### 3. Technical Scope & Core API Contract
- **Header Declarations**:
  ```c
  // Exact function prototypes declared in src/<header>.h
````

- **Ownership & Lifetime Contract**:
  - **Parameter Ownership**: \<Borrowed vs owned pointers, reference counts upon entry.>
  - **Return Semantics**: \<Container initial refcount, singleton returns, error returns.>
  - **Cleanup & Rollback**: <Unwinding rules on NULL arguments or allocation failures.>
- **Refactoring Scope**: <Any deprecations or delegations.>

______

## 4. Explicit Byte-Offset Memory Diagram

```text
Byte Offset:
+00 [ 8 bytes: field_name ] (type)
+08 [ 8 bytes: field_name ] (type)
+16 [ 4 bytes: field_name ] (type)
+20 [ 4 bytes: PADDING    ] (Alignment hole to 8-byte boundary!)
Total: X bytes (strictly aligned for 64-bit CPU bus)
```

______

### 5. Physical Machine Mechanics (What the Silicon Does)

- **CPU Registers & Calling Conventions**: \<Register passing, spill areas, stack frame offsets.>
- **Cache Line Chunking (64 Bytes)**: \<L1/L2 data cache behavior and contiguous memory layouts.>
- **MMU & Memory Controller**: \<Address alignment, word-boundary bus fetches, virtual memory page behavior.>

______

### 6. Inductive Invariant Modeling

$$\\mathcal{I}_0 \\implies \\mathcal{I}_k \\implies \\mathcal{I}_{k+1} \\quad \\text{and} \\quad \\mathcal{I}_{\\text{rollback}}$$

- **Base Invariant $\\mathcal{I}\_0$**: \<Initial state at allocation (zeroed slots, refcount = 1, rooted in VM).>
- **Inductive Step $\\mathcal{I}_k \\implies \\mathcal{I}_{k+1}$**: <How step k transitions state while preserving pointer symmetry and reference parity.>
- **Rollback Invariant $\\mathcal{I}\_{\\text{rollback}}$**: \<How a failure at step k unwinds steps 0..k-1 without leaks or double-frees.>

______

### 7. Incremental Micro-Loop Decomposition

1. **Micro-Loop 1**: \<Implement operation A in src/ -> test A -> green.>
1. **Micro-Loop 2**: \<Implement operation B in src/ -> test B -> green.>
1. **Micro-Loop 3**: \<Adversarial testing, boundary probes, and allocation failure injection sweeps.>

______

### 8. Before-Goal Conceptual References & Targeted Readings

- **Standard & External References**:
  - [Title](URL) — Short summary.
- **Targeted Book Readings from `.sources/INDEX.md`**:
  - **\<Author\> — *\<Title\>***: Chapter X, Section Y (pp. A–B) — \<Why read this\>.

```text
```
