---
name: adversarial-testing
description: >-
  Provides comprehensive guidelines, heuristics, and patterns for creating adversarial unit tests
  in C using µnit and bootlib. Use whenever writing, reviewing, or expanding tests in tests/
  to stress-test memory management, simulate allocation failures, and probe reference cycles.
---

# Adversarial Testing Skill (`adversarial-testing`)

This skill defines the methodology, patterns, and verification requirements for writing **adversarial unit tests** in `trash-gather`.

The goal of adversarial testing is not merely to confirm happy paths, but to actively probe edge cases, break assumptions, expose use-after-free bugs, and guarantee 100% leak-free execution.

______________________________________________________________________

## 🤝 Symbiosis with the `c-expert` Skill

Adversarial testing does not operate in a vacuum. Every test pattern in this skill is derived directly from the systems principles, memory invariants, and language standard rules defined in the **`c-expert`** skill (\[.agents/skills/c-expert/SKILL.md\](file:///Users/bwrob/dev/trash-gather/.agents/skills/c-expert/SKILL.md)):

1. **State Ownership & Contracts (`c-expert` §1)**: Tests must explicitly probe caller vs. container ownership (borrowed vs. owned, transferred vs. retained references).
1. **Multi-Stage Allocation Rollback (`c-expert` §1 & §3)**: The unwinding logic required by `c-expert` on multi-step allocations must be systematically stressed via `boot_set_fail_alloc_after()` and component mismatch probes.
1. **Flexible Array Member Layouts (`c-expert` §3.1)**: Verify exact allocation arithmetic ($\\text{sizeof}(T) + N \\times \\text{sizeof}(\\text{elem})$), boundary accesses, and contiguous memory safety.
1. **Dual GC Lifecycle Balance (`c-expert` §3.2)**: Validate the division of labor between immediate reference counting and cyclic mark-and-sweep sweeps.
1. **Undefined Behavior Defenses (`c-expert` §3.3)**: Actively stimulate guards against integer overflow, dangling pointers, double frees, and NULL dereferences.

Whenever designing tests, review the corresponding domain section in `c-expert` to identify every memory invariant and formulate tests that attempt to violate it.

______________________________________________________________________

## 🎯 Core Principles

1. **Be Adversarial**: Assume every pointer can be `NULL`, every allocation can fail, and every container can form an arbitrary reference cycle.
1. **Never Touch `src/`**: As an AI agent, you write and maintain tests in `tests/` and benchmarks in `bench/`, but never edit runtime code in `src/`.
1. **Enforce Zero Leaks**: Every test must conclude with `assert_true(boot_all_freed())` or `assert(boot_all_freed())` to ensure no heap blocks remain orphaned.
1. **Mandate 100.00% Line Coverage**: Adversarial testing is not complete until every file in `src/` achieves **100.00% line coverage** (`just coverage`). Every defensive guard, NULL check, allocation rollback, and error path must be actively stimulated and verified by a test in `tests/`. No uncovered lines are permitted.

______________________________________________________________________

## 🔬 Adversarial Test Categories & Patterns

### 1. Pointer & Boundary Safety

- **NULL Handles**: Pass `NULL` to API functions (`refcount_inc(NULL)`, `refcount_dec(NULL)`, `array_set(NULL, 0, val)`, `array_get(NULL, 0)`, `add(NULL, NULL)`). Functions must safely return default/null values or error codes without segmentation faults.
- **Sparse Containers (NULL Slots)**: Construct arrays where only some slots are populated and others remain `NULL`. Verify that iteration, tracing, decref, and sweeping handle `NULL` without dereferencing invalid memory.
- **Out-of-Bounds Access**: Attempt reading and writing past array bounds (`array_set(arr, arr->size + 5, val)`, `array_get(arr, 999)`).

### 2. Allocation Failure Simulation (`bootlib`)

`bootlib` provides an allocation failure injector: `boot_set_fail_alloc_after(N)`.

- Use this to simulate heap exhaustion during VM initialization, stack frame expansion, or object constructors.
- **Pattern**:
  ```c
  for (int i = 0; i <= 4; i++) {
    boot_set_fail_alloc_after(i);
    vm_new();
    assert_null(vm_get_current());
  }
  // Always verify that aborted allocations do not leak memory:
  assert(boot_all_freed());
  ```

### 3. Reference Counting & Cycles

- **Container Ownership & Reference Parity**: Whenever a function returns a container holding newly created elements (e.g. `add(tupleA, tupleB)`):
  - **The `vm_free()` Masking Trap**: Teardown via `vm_free()` unconditionally frees all objects tracked in `vm.objects`. Relying solely on `vm_free()` **masks reference count leaks**!
  - **Mandatory Lifecycle Test**: Adversarial tests MUST assert that each newly created child element has `refcount == 1`. Tests MUST release the container via pure reference counting (`refcount_dec(container)`), call `vm_cleanup_after_refcount()`, and assert `boot_all_freed()`. If child elements were left with an extra reference, this test will fail immediately.
- **Mutual Cycles ($A \\leftrightarrow B$)**: Two containers pointing to each other. Verify that pure reference counting traps them, but `vm_collect_garbage()` cleanly reclaims them.
- **Self-Referencing Cycles ($A \\rightarrow A$)**: An array pointing to itself. Tests cycle self-loop detection during gray tracing and sweep deallocation.
- **Dead Cycles with Live Escapes**: A dead cycle ($A \\leftrightarrow B$) holds an outgoing reference to a live, rooted object (`live_obj`).
  - Verify that the cycle is collected.
  - Verify that `live_obj` is **not** collected.
  - Verify that `live_obj->refcount` is accurately decremented when the dead parent container is swept.
- **Multi-Node Cycle Meshes**: Complex topologies (e.g. triangles $A \\rightarrow B \\rightarrow C \\rightarrow A$ with external tails $C \\rightarrow \\text{tail}$) to ensure graph traversal does not suffer from stack overflow or order-of-destruction dependencies.

### 4. Partial Loop Failure & Rollback ($K$-of-$N$ Failure Probe)

Whenever code allocates or transforms a sequence of $N$ objects in a loop:

- **The Entry-Guard Illusion**: Testing whole-operation rejection at entry (e.g. length mismatch $a_len \\neq b_len$) only tests the initial guard. It completely fails to exercise the loop's rollback mechanism.
- **Mandatory Mid-Loop Failure Probe**: Adversarial tests MUST probe failure midway ($0 < k < N$). For example, construct an input where element $0$ succeeds, but element $1$ fails (incompatible type or NULL). Verify that the function returns `NULL` AND that element $0$ is not leaked on the heap under `vm_cleanup_after_refcount(); assert(boot_all_freed())`.
- **Heap Failure Sweep**: Run `boot_set_fail_alloc_after(i)` across all steps of the composite operation to verify that heap exhaustion midway through creating child objects unwinds cleanly.

### 5. Teardown & Compaction Verification

- **Arbitrary Creation Order**: Allocate children before containers and containers before children. Ensure `vm_free()` and `sweep()` do not suffer from order-of-destruction `heap-use-after-free` (the Single-Pass Deallocation Trap).
- **Tracker ID Invariant**: After `vm_collect_garbage()`, verify that surviving objects in `CURRENT_VM->objects` have their `obj->tracker_id` re-synchronized to their new array index following compaction.

______________________________________________________________________

## 🛠️ Test Execution & Tooling Workflow

Use `just` commands to build, run, and filter tests:

| Command                      | Purpose                                                              |
| :--------------------------- | :------------------------------------------------------------------- |
| `just test`                  | Run the full test suite with ASan and UBSan                          |
| `just test-filter <pattern>` | Run only tests matching a name pattern (e.g. `just test-filter gc_`) |
| `just format`                | Automatically format test files according to `.clang-format`         |
| `just check`                 | Verify all pre-commit hooks pass across the repository               |
