# Milestone: Interactive Memory REPL

**ID:** `0fcfad7`\
**Status:** Planned\
**Difficulty:** 5 / 5\
**Focus:** Build an interactive command-line interface (`just run`) for allocating objects, pushing/popping frames, triggering garbage collection passes, and inspecting runtime telemetry in real time.\
**Prerequisites:** [Cycle-Safe String Representation & Object Printing](bf0a981_cycle_safe_string_repr.md), [Function Objects & Closures (closure_t)](a0c00e1_closures_and_lexical_environments.md), [Remembered Sets, Write Barriers & Minor Generational GC](034b527_generational_write_barriers_and_minor_gc.md), [Page-Aligned PyMalloc with Bitmask Pool Recovery](a929415_page_aligned_pymalloc.md), [ASCII Heap Visualizer](81a16cb_ascii_heap_visualizer.md), [Cross-VM Value Marshaling & Result Channels](dac4aea_cross_vm_marshaling_and_channels.md), [Binary Heap Graph Serialization](402c62c_binary_heap_serialization.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**: Transform the sandbox executable (`src/main.c`, `just run`) into an interactive terminal REPL capable of parsing commands to declare variables (`let x = [1, 2]`), push and pop call frames (`frame push`, `frame pop`), mutate references, invoke garbage collector passes (`gc minor`, `gc major`), display memory telemetry, and render ASCII heap graphs (`graph`, `dump`).
1. **Scope Boundaries**: Integrates the ASCII pointer graph visualizer from `81a16cb`. Complex scripting grammars, arbitrary expression evaluators, and graphical UIs are intentionally out of scope.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - The REPL maintains a session environment wrapping active stack frames, bound variable symbols, and command buffers:

     ```text
     [REPL Command Loop (stdin)]
              │
              ▼
     [Tokenizer / Command Parser]
              │
              ▼
     [VM Root Environment / Frame Stack]
       ├── Frame #0 (Global Scope)
       │     ├── var "a" ──> [List Obj #1 (rc=1)]
       │     └── var "b" ──> [Tuple Obj #2 (rc=1)]
       └── Frame #1 (Local Frame)
             └── var "c" ──> [Dict Obj #3 (rc=1)]
     ```

1. **Core Systems Invariants**:
   - Error recovery invariant: Invalid commands, malformed syntax, or unresolvable variable names must output friendly diagnostics and cleanly abort without corrupting VM state or leaking temporary allocations.
   - Clean exit invariant: Exiting the REPL session (`exit`, `quit`, or EOF) must pop all active stack frames, trigger final deallocations, and cleanly release all command buffers and token records.
   - Root anchoring invariant: Any object instantiated via a REPL command must be registered on an active root frame before subsequent allocations occur, preventing premature cycle sweeps during complex commands.
1. **Architectural Trade-offs**: Adopting a lightweight line-buffered command parser (`fgets` with deterministic token scanning) avoids heavy parser dependencies while offering immediate, intuitive interactive control over VM internals.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Read-Eval-Print Loops (REPL); command-driven state transitions; safe terminal input parsing; isolated execution contexts and interactive debugger architecture.
1. **Socratic Inquiries**:
   - If a multi-step REPL command allocates intermediate heap objects before encountering a syntax error, how does the runtime clean them up without memory leaks?
   - How does manually pushing and popping simulated call frames via CLI commands demonstrate the difference between immediate refcount reclamation and cyclic island retention?
   - Why is registering named variables within a dedicated REPL root table safer than maintaining raw C pointers across user input prompts?
1. **Failure Modes & Pitfalls**: Unbounded buffer overflows when reading terminal input; memory leaks on aborted commands; dangling object references after popping a frame if the local symbol table is not invalidated.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. Define REPL command types, token representation, and environment structures in `src/repl.h`.
   1. Implement the command loop, token scanner, and execution dispatcher (`let`, `set`, `frame push`, `frame pop`, `gc`, `stats`, `help`, `exit`) in `src/repl.c`.
   1. Wire `src/main.c` to enter the interactive REPL loop upon launching `just run`.
   1. Ensure all allocated temporary strings and environment symbols are tracked and freed upon error or exit.
   1. Write scripted stdin integration tests in `tests/test_repl.c` validating command execution and error recovery.
1. **File Touchpoints**:
   1. `src/main.c`
   1. `src/repl.h`, `src/repl.c`
   1. `src/vm.h`, `src/vm.c`
   1. `tests/test_repl.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Automated tests in `tests/test_repl.c` feeding scripted input sequences via redirected stdin, verifying correct state mutations, frame management, and error resilience.
1. **Zero-Leak Guarantee**: Exiting a REPL session after arbitrary valid and invalid commands guarantees zero memory leaks via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero warnings under ASan/UBSan.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Building Read-Eval-Print Loops in C Systems](https://en.wikipedia.org/wiki/Read%E2%80%93eval%E2%80%93print_loop): Designing clean command dispatch, input line parsing, and execution loop state.
   - [Standard I/O Streams and Line Buffering in C](https://en.cppreference.com/w/c/io): Safely reading user input, handling EOF, and emitting formatted error diagnostics.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython Parser/myreadline.c Terminal Architecture](https://github.com/python/cpython/blob/main/Parser/myreadline.c): How the Python interactive shell interfaces with terminal streams and handles user interrupts.
   - [The Lua Standalone Interpreter Source (lua.c)](https://www.lua.org/source/5.4/lua.c.html): Compact, elegant architecture of a production REPL in clean ANSI C.
