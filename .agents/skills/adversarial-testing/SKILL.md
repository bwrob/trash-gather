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

---

## 🎯 Core Principles

1. **Be Adversarial**: Assume every pointer can be `NULL`, every allocation can fail, and every container can form an arbitrary reference cycle.
2. **Never Touch `src/`**: As an AI agent, you write and maintain tests in `tests/` and benchmarks in `bench/`, but never edit runtime code in `src/`.
3. **Enforce Zero Leaks**: Every test must conclude with `assert_true(boot_all_freed())` or `assert(boot_all_freed())` to ensure no heap blocks remain orphaned.

---

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
- **Mutual Cycles ($A \leftrightarrow B$)**: Two containers pointing to each other. Verify that pure reference counting traps them, but `vm_collect_garbage()` cleanly reclaims them.
- **Self-Referencing Cycles ($A \rightarrow A$)**: An array pointing to itself. Tests cycle self-loop detection during gray tracing and sweep deallocation.
- **Dead Cycles with Live Escapes**: A dead cycle ($A \leftrightarrow B$) holds an outgoing reference to a live, rooted object (`live_obj`).
  - Verify that the cycle is collected.
  - Verify that `live_obj` is **not** collected.
  - Verify that `live_obj->refcount` is accurately decremented when the dead parent container is swept.
- **Multi-Node Cycle Meshes**: Complex topologies (e.g. triangles $A \rightarrow B \rightarrow C \rightarrow A$ with external tails $C \rightarrow \text{tail}$) to ensure graph traversal does not suffer from stack overflow or order-of-destruction dependencies.

### 4. Teardown & Compaction Verification
- **Arbitrary Creation Order**: Allocate children before containers and containers before children. Ensure `vm_free()` and `sweep()` do not suffer from order-of-destruction `heap-use-after-free` (the Single-Pass Deallocation Trap).
- **Tracker ID Invariant**: After `vm_collect_garbage()`, verify that surviving objects in `CURRENT_VM->objects` have their `obj->tracker_id` re-synchronized to their new array index following compaction.

---

## 🛠️ Test Execution & Tooling Workflow

Use `just` commands to build, run, and filter tests:

| Command | Purpose |
| :--- | :--- |
| `just test` | Run the full test suite with ASan and UBSan |
| `just test-filter <pattern>` | Run only tests matching a name pattern (e.g. `just test-filter gc_`) |
| `just format` | Automatically format test files according to `.clang-format` |
| `just check` | Verify all pre-commit hooks pass across the repository |
