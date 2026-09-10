# Milestone: ASCII Heap Visualizer

**ID:** `81a16cb`\
**Status:** Planned\
**Focus:** Build an ASCII pointer graph visualizer that renders live root frames, object topologies, reachable structures, and unreachable cyclic islands in the terminal.\
**Prerequisites:** [Interactive Memory REPL](0fcfad7_interactive_memory_repl.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Implement a non-intrusive heap visualizer module (`src/inspector.c`) that traverses root frames and uncollected heap objects to render hierarchical ASCII trees, box-and-pointer diagrams, and cyclic reference markers directly to terminal output or via the REPL.
1. **Scope Boundaries**: Graphical web UIs or GUI desktop windows are intentionally out of scope; terminal-based UTF-8 / ASCII formatting provides instant, zero-dependency visual insight into memory topology.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Hierarchical ASCII graph rendering showing root frames, generations, children, and cyclic back-edges:
     ```
     [Stack Root: Frame #0]
       ├── var 'root_list' ──> [List #12 (Gen 1, rc=2)]
       │                         ├── [0] ──> [Tuple #14 (Gen 0, rc=1)]
       │                         │             └── [0] ──> [Int 42]
       │                         └── [1] ──> [String "hello" (rc=1)]
       └── var 'head' ───────> [Node #20 (Gen 0, rc=2)]
                                 └── next ──> [Node #21 (Gen 0, rc=2)]
                                                └── next ──> [Node #20 (Cycle ↺)]
     [Unreachable Cyclic Island (Pending Sweep)]
       ├── [Node #30 (Gen 0, rc=1)] <───┐
       └── [Node #31 (Gen 0, rc=1)] ────┘
     ```
1. **Core Systems Invariants**:
   - Non-intrusive inspection invariant: Visualizing the heap or printing pointer graphs must never mutate refcounts, alter object payloads, or disturb GC mark bits.
   - Cycle suppression invariant: Visualizer graph traversal must track visited object pointers (using a temporary lookup set or address table) to guarantee termination on cyclic reference structures.
   - Safe root isolation invariant: Traversing live heap roots must strictly read through valid frame pointers and safely handle uninitialized or `None` slots without dereferencing NULL.
1. **Architectural Trade-offs**: Dynamic visited pointer tracking set vs static recursion depth limits; tracking visited pointers ensures complete, cycle-safe rendering of arbitrarily tangled graphs without risking stack overflows or missing cycles.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Graph traversal (Depth-First Search vs Breadth-First Search); cycle detection and suppression; mental model reinforcement for automatic memory management; structural representation of memory graphs.
1. **Socratic Inquiries**:
   - How does visualizing unreachable cyclic islands before and after a GC sweep reinforce the difference between reference counting and tracing collectors?
   - If a visualizer marks objects to prevent cycles, how does it avoid conflicting with the garbage collector's own mark phase?
   - What visual cues (e.g. `(Cycle ↺)`, indentation levels, box connectors) most clearly communicate ownership and reference loops in a text terminal?
1. **Failure Modes & Pitfalls**: Infinite loops during traversal of cyclic structures; stack overflow on deeply nested object graphs; reading uninitialized memory in partially constructed objects.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. Define visualizer configuration and rendering interfaces in `src/inspector.h`.
   1. Implement visited address set tracking and DFS tree traversal in `src/inspector.c`.
   1. Format box connectors, indentation levels, object metadata (type, ID, generation, refcount), and cyclic back-references.
   1. Expose the `dump` / `graph` inspection command in the interactive REPL (`src/repl.c`).
   1. Write unit tests in `tests/test_inspector.c` asserting correct ASCII rendering and cycle suppression on linear, tree, and cyclic graph topologies.
1. **File Touchpoints**:
   1. `src/inspector.h`, `src/inspector.c`
   1. `src/repl.c`
   1. `tests/test_inspector.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Tests in `tests/test_inspector.c` verifying ASCII output on self-referential objects, two-node cycles, complex DAGs, and multi-frame stacks.
1. **Zero-Leak Guarantee**: All temporary buffers and visited tracking sets allocated during inspection are completely freed, confirmed by `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero warnings under ASan/UBSan.
