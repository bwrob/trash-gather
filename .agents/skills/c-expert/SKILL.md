---
name: c-expert
class: language
description: >-
  Expert C systems programming, runtime architecture, memory safety, and code review:
  covers ISO C17, object models, tagged unions, flexible array members, reference counting,
  mark-and-sweep garbage collection, pointer ownership, allocation rollback, undefined behavior,
  and sanitizer verification. Use when writing, reviewing, optimizing, or debugging C systems code.
paths: "**/*.c,**/*.h"
---

# C Systems, Runtime Architecture & Code Review Expert

Covers ISO C17 systems programming, language runtimes, virtual machines, garbage collectors, memory safety, and code reviews.

______________________________________________________________________

## 1. Working Rules & Principles

- **State Ownership**: Every pointer passed to or returned by a function must have an unambiguous ownership contract (borrowed vs. owned).
- **Check All Fallible Calls**: Never assume an allocation or system call succeeded. Every `malloc`, `calloc`, or container lookup must be verified before dereferencing.
- **Roll Back on Failure**: In multi-stage allocations, a failure at step $K$ must unwind steps $1 \\dots K-1$ cleanly without leaking resources (`goto cleanup` pattern).
- **Assert Internal Invariants**: State invariants and validate boundaries at leaf functions. Check sizes before multiplication to prevent integer overflow.
- **Portability First**: Target ISO C17 (`-std=c17`). Post-C99 syntax (such as anonymous structs/unions or `_Static_assert`) must be conscious, intentional, and portable across GCC, Clang, and MSVC.
- **Verify Rebuilt Artifacts**: Use instrumented builds (`-fsanitize=address,undefined`) and leak-tracking harnesses (`bootlib`) to guarantee zero memory leaks and zero undefined behavior.

______________________________________________________________________

## 2. Repo Conventions Outrank This Skill

Always read the repository's `AGENTS.md`, its public headers, and adjacent `.c` files before proposing or reviewing changes:

- **Strict `src/` Boundary**: In educational or pair-programming repositories where the human developer writes the runtime (as defined in `AGENTS.md`), the AI agent must **never** create or edit files in `src/`.
- **Socratic Tutoring & Review**: Reviews must use Archimedean inquiries and probing questions to guide the developer to uncover pointer traps, allocation leaks, and GC invariants rather than spoiling the solution with ready-made runtime code.
- **Project Idioms**: Where repo conventions sanction specific patterns (e.g. `goto cleanup`, custom allocation interceptors, tagged unions), preserve them rather than introducing external restyling.

______________________________________________________________________

## 3. Core Systems Competencies

### 3.1 Object Models & Contiguous Memory

- **Flexible Array Members (C99 §6.7.2.1)**: Use contiguous payload structures (`items[]`) for variable-length containers (tuples, arrays) to eliminate pointer chasing.
- **Allocation Arithmetic**: Compute sizes accurately: $\\text{sizeof}(T) + N \\times \\text{sizeof}(\\text{elem})$. Check for unsigned integer overflow before multiplying.
- **Slot Safety**: Always zero-initialize (`NULL`) all pointer slots upon allocation so GC tracing cannot inspect uninitialized memory.
- **Union Constraints**: A struct with a flexible array member cannot appear by value inside a `union`; it must be held via a pointer (`tuple_t *v_tuple`).

### 3.2 Garbage Collection & Lifecycle Mechanics

- **Dual GC Balance**:
  - Reference counting handles immediate reclamation of tree-like, acyclic graphs.
  - Mark-and-sweep or cycle collectors resolve cyclic reference meshes.
- **Tracing Completeness**: `trace_blacken_object()` must visit every referenced child slot in container objects.
- **Teardown Order**: `object_free_payload()` must release nested heap allocations before the parent object container is freed.
- **Root Safety**: Objects must be rooted or tracked during construction before triggering recursive allocations that could cause a GC sweep.

### 3.3 Undefined Behavior & Safety Guards

- **Strict Aliasing & Type Punning**: Use `memcpy` instead of pointer casts (`*(float *)&i`) for bit-casting.
- **Bounds Checking**: Always validate container indices against `size` before reading or writing.
- **No VLAs**: Prohibit Variable-Length Arrays; they are optional in C11/C17 and introduce unbounded stack overflow hazards.

______________________________________________________________________

## 4. Verification Standard

Every C systems change or review must satisfy:

1. **Clean Build**: Zero warnings under `-Wall -Wextra` targeting ISO C17 (`-std=c17`).
1. **Sanitizer Clean**: Zero reports under `-fsanitize=address,undefined`.
1. **Zero-Leak Guarantee**: Memory leak tracking asserts `assert(boot_all_freed())` at the end of all test runs.
1. **Clang-Tidy & Docs**: Passes static analysis (`just lint-c`) and Doxygen docstring validation (`just lint-docs`).

______________________________________________________________________

## 5. Reference Manuals

Consult these comprehensive guides in [references/](./references/) when writing, auditing, or reviewing:

- [review-checklist.md](./references/review-checklist.md) — Socratic code review protocol, question templates, and 5-phase review checklist.
- [gc-runtime-architecture.md](./references/gc-runtime-architecture.md) — Object models, flexible array members, reference counting, mark-and-sweep, and failure rollbacks.
- [c17-portability.md](./references/c17-portability.md) — ISO C17 standards, conscious post-C99 syntax, cross-compiler rules, and prohibited constructs.
- [memory-safety.md](./references/memory-safety.md) — Lifetimes, allocation arithmetic, buffer overflow mitigation, and sanitizer configurations.
- [correctness-traps.md](./references/correctness-traps.md) — Undefined behavior catalog, integer promotion traps, pointer comparison rules, and alignment.
- [runtime-safety.md](./references/runtime-safety.md) — Assertion boundaries, fallible call checking, and state invariants.
- [implementation-structure.md](./references/implementation-structure.md) — Module decomposition, error idioms (`goto cleanup`), and type models.
- [build-and-measurement.md](./references/build-and-measurement.md) — Compiler warning bundles, ASan/UBSan setup, and benchmarking.
- [legibility-standard.md](./references/legibility-standard.md) — Systems code readability and naming clarity.
