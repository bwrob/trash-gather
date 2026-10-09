# Lesson 06: Translation Unit Decomposition & Linker Graph Mechanics

**Branch:** `chore/reorganization-and-cleanup` (Merged in PR [#9](https://github.com/bwrob/trash-gather/pull/9))
**Focus:** Architectural decomposition of monolithic modules into domain directories, header DAG invariants vs. implementation graph topology, and compiler translation unit isolation.

______

## 1. System Engineering & Core Concepts

### 1.1 The Header Invariant: Strict Compile-Time Directed Acyclic Graph (DAG)

In ISO C17, `#include` directives perform literal preprocessor textual substitution. Because the compiler compiles each translation unit from top to bottom, header dependencies must form a strict **Directed Acyclic Graph (DAG)**:

- If header `A.h` requires the concrete size and byte offsets of a struct defined in `B.h`, and `B.h` includes `A.h`, circular inclusion guards prevent infinite preprocessor recursion, but leave one header evaluated before the other's types exist, triggering `error: unknown type name`.
- **Breaking Header Cycles via Incomplete Types**: Pointers do not require complete type information. By forward-declaring types (`typedef struct Object object_t;`), headers only need to know that `object_t` is a pointer-sized handle (8 bytes on 64-bit architectures), deferring layout dependencies to implementation files.

### 1.2 The Implementation Topology: Cyclic Graphs Resolved at Link Time

Unlike headers, implementation files (`.c` translation units) do **not** need to form a DAG:

- Each `.c` file is parsed and compiled into an independent object file (`.o`) containing code sections (`.text`), data sections (`.data`, `.rodata`), and a symbol table (`.symtab`).
- Functions in `vm.c` can freely call container operations like `tuple_decref_elements()`, while container routines in `tuple.c` and `object.c` can call `vm_get_none()` or `vm_track_object()`.
- The **linker** performs symbol resolution across all translation units simultaneously. As long as every unresolved symbol (`UND` in `readelf`/`nm`) matches exactly one global definition, cross-module cycles at the implementation level are valid, idiomatic systems architecture.

```text
Compilation Phase (Strict DAG):
  tuple.h ----> object.h <----+
     |                        |
     v                        | (forward declarations only)
  vm.h -----------------------+

Link-Time Phase (Arbitrary Graph Topology):
  vm.o <=====================> object.o
   ^                             ^
   |                             |
   +======> tuple.o <============+
```

______

## 2. Pitfalls, Failure Modes & Diagnosis

### 2.1 The Implicit Declaration Trap under Modular Inclusions

In monolithic files, earlier header inclusions often mask missing `#include <stdlib.h>` directives in lower-level routines. When separating container routines into dedicated files (e.g. moving list logic into `src/vm_objects/list.c`), calls to `calloc()` or `free()` failed in benchmark builds with:

```text
src/vm_objects/list.c:11:27: error: call to undeclared library function 'calloc'
      with type 'void *(unsigned long, unsigned long)'; ISO C99 and later do not
      support implicit function declarations [-Wimplicit-function-declaration]
src/vm_objects/list.c:20:9: error: call to undeclared library function 'free'
      with type 'void (void *)'; ISO C99 and later do not support implicit
      function declarations [-Wimplicit-function-declaration]
```

**Diagnostic Insight**: The unit test build was masking the missing include because `-include bootlib.h` injected wrapper definitions into every compilation unit. The benchmark build compiled with `-DBOOTLIB_NO_OVERRIDE` (using standard libc headers directly), immediately exposing that `list.c` did not explicitly import its own dependencies.

### 2.2 Exhaustive Switch Return Trap on Tagged Unions

When introducing an explicit `INVALID` variant (value 0) to `enum ObjectKind` to guard uninitialized memory, `object_len()` contained a `switch (obj->kind)` matching all defined enum variants.
However, because GCC was invoked with `-Wreturn-type` in benchmark builds without µnit, the compiler warned:

```text
src/vm_objects/object.c:361:1: warning: control reaches end of non-void function [-Wreturn-type]
```

If memory contains an unmapped or corrupted integer value (e.g., cast from raw payload), execution flows past the switch statement without executing a `return`, resulting in Undefined Behavior. Restoring an explicit fallback `return -3;` after the switch construct restored deterministic safety.

______

## 3. Architectural Solutions & Mental Models

### 3.1 Domain-Driven Partitioning: `vm/` vs `vm_objects/`

The monolithic `src/` directory was split into two cohesive subsystems:

1. **`src/vm/`**: Execution environment, frame lifecycle, stack evaluation, root sets, and mark/sweep GC infrastructure (`vm.c`, `stack.c`).
2. **`src/vm_objects/`**: Object memory layouts, tagged unions, and type-specific operations (`object.c`, `tuple.c`, `list.c`).

### 3.2 Granular Test Decomposition

Monolithic test files (`test_new.c`, 700+ line `test_refcount.c`) were broken into 1:1 mapped test suites:

- `test_list.c`, `test_tuple.c`, `test_none.c`, `test_object.c`, `test_vm.c`, `test_stack.c`, `test_frame.c`.
- **Fault Isolation**: When an invariant breaks, the test failure is immediately localized to a specific subsystem without noise from unrelated containers.

______

## 4. Hardware & Silicon Mechanics (What the Machine Did)

### 4.1 Linker Symbol Resolution and PLT Overhead

When all functions reside in a single translation unit, the compiler can perform intra-file jump optimizations (`b` / `jmp` relative offsets). When split across translation units:

- Calls between `vm.o` and `object.o` produce relocation entries (`R_X86_64_PLT32` or ARM64 `ARM64_RELOC_BRANCH26`) in the ELF/Mach-O binary.
- During link time, the static linker patches these displacement slots with direct 32-bit signed offsets, meaning intra-binary static calls incur **zero indirect branch penalty** compared to monolithic files.

### 4.2 Cache Line Layout of Enum Initializers

By allocating `INVALID = 0` as the initial enum value:

- Zero-initialized heap buffers allocated via `calloc()` (which fetches zeroed pages from the OS via copy-on-write `zero_page`) default to `kind = INVALID` without requiring explicit initialization writes.
- This creates an immediate CPU-level fault domain: if code attempts to read or compute the length of a partially initialized object before setting its concrete tag, the switch statement safely rejects it as `INVALID` rather than misinterpreting it as `INTEGER = 0`.

______

## 5. Tooling Insights & Workflow Takeaways

### 5.1 Parity Across Build Configurations

A critical lesson from this refactor: **Different build configurations reveal different compiler invariants**.

- Unit tests run with `-include bootlib.h` and AddressSanitizer.
- Benchmarks run with `-DBOOTLIB_NO_OVERRIDE` and pure system headers.
- Static analysis runs with `clang-tidy`.

Running `just bench-build` and `just build` alongside `just test` before publishing is essential to ensure that missing `#include` directives and compiler warnings are caught before continuous integration runs.

### 5.2 Dynamic Include Flag Discovery (`SRC_INCS`)

Hardcoding `-Isrc` in the `justfile` failed when files were moved into nested subdirectories (`src/vm/`, `src/vm_objects/`). Adopting dynamic directory discovery in `justfile`:

```makefile
SRC_INCS := `find src -type d | sort | sed 's|^|-I|' | tr '\n' ' '`
```

guarantees that all downstream tooling (`CFLAGS`, `BENCH_FLAGS`, `clang-tidy`, `compile_commands.json`) automatically inherit directory additions without manual configuration synchronization.
