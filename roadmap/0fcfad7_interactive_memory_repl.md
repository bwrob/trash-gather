# Milestone: Interactive Memory REPL

**ID:** `0fcfad7`\
**Status:** Planned\
**Focus:** Build an interactive command-line interface (`just run`) for allocating objects, pushing/popping frames, triggering garbage collection passes, and inspecting runtime telemetry in real time.\
**Prerequisites:** [Cycle-Safe String Representation & Object Printing](bf0a981_cycle_safe_string_repr.md), [Function Objects & Closures (closure_t)](a0c00e1_closures_and_lexical_environments.md), [Generational Garbage Collection](01be152_generational_garbage_collection.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Transform the sandbox executable (`src/main.c`, `just run`) into an interactive terminal REPL capable of parsing commands to declare variables (`let x = [1, 2]`), push and pop call frames (`frame push`, `frame pop`), mutate references, invoke garbage collector passes (`gc minor`, `gc major`), and display memory telemetry.
1. **Scope Boundaries**: Live ASCII pointer graph rendering and tree visualization are deferred to Milestone 18. Complex scripting grammars, arbitrary expression evaluators, and graphical UIs are intentionally out of scope.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - The REPL maintains a session environment wrapping active stack frames, bound variable symbols, and command buffers:
     ```
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

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Read-Eval-Print Loops (REPL); command-driven state transitions; safe terminal input parsing; isolated execution contexts and interactive debugger architecture.
1. **Socratic Inquiries**:
   - If a multi-step REPL command allocates intermediate heap objects before encountering a syntax error, how does the runtime clean them up without memory leaks?
   - How does manually pushing and popping simulated call frames via CLI commands demonstrate the difference between immediate refcount reclamation and cyclic island retention?
   - Why is registering named variables within a dedicated REPL root table safer than maintaining raw C pointers across user input prompts?
1. **Failure Modes & Pitfalls**: Unbounded buffer overflows when reading terminal input; memory leaks on aborted commands; dangling object references after popping a frame if the local symbol table is not invalidated.

______________________________________________________________________

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

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Automated tests in `tests/test_repl.c` feeding scripted input sequences via redirected stdin, verifying correct state mutations, frame management, and error resilience.
1. **Zero-Leak Guarantee**: Exiting a REPL session after arbitrary valid and invalid commands guarantees zero memory leaks via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero warnings under ASan/UBSan.
