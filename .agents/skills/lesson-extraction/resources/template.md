# Educational Lesson Template (`lessons/NN_<topic_slug>.md`)

This template defines the mandatory structure and quality rubric for educational lessons authored in `lessons/`.

______

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

## 4. Hardware & Silicon Mechanics (What the Machine Did)
- Physical memory layout: struct padding holes, CPU word alignment (8-byte boundaries), and bus transaction efficiency.
- CPU Cache Locality: Cache line chunking (64-byte chunks), spatial/temporal locality, and L1/L2 hits vs cache miss stalls.
- Branch Prediction & CPU Pipeline: Branch predictor behavior (e.g. monomorphic vs polymorphic dispatch tables, predicted branches in error guards).
- Virtual Memory & MMU: Page boundaries (4 KB), bitmasks, address translation, and TLB efficiency.

---

## 5. Tooling Insights & Workflow Takeaways
- What role did the tooling play in accelerating discovery? (e.g., ASan shadow bytes, `bootlib` leak tracking, `just test-filter`, `lldb` watchpoints).
- What testing heuristics or assertions caught the issue?
```
