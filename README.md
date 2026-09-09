# trash-gather

Explorations in automatic memory management and virtual machine runtime implementation in C.

______________________________________________________________________

## 🚀 About & Boot.dev Origin

This project is a direct continuation and expansion of the **Memory Management & Garbage Collector** curriculum on [Boot.dev](https://www.boot.dev).

> [!IMPORTANT]
> **Endorsement**: [Boot.dev](https://www.boot.dev) is an incredible hands-on platform for learning backend development, systems programming, C, memory management, and computer science fundamentals. If you're interested in building real projects from scratch—from virtual machines to backend servers—we enthusiastically recommend checking out [Boot.dev](https://www.boot.dev)!

______________________________________________________________________

## 🎯 Roadmap & Extended Architecture

While the initial codebase established a baseline Mark-and-Sweep garbage collection runtime for lang, this repository is actively expanding into advanced runtime mechanics:

- [x] **Core Mark-and-Sweep GC & VM Baseline**
- [x] **Hybrid Garbage Collection**: Immediate Reference Counting + Mark-and-Sweep Cycle Collector (CPython-style hybrid model)
- [x] **Tooling, CI & Safety Infrastructure** (ASan/UBSan, `bootlib`, `clang-format`, `clang-tidy`, Google Benchmark)
- [x] **Python Toolchain** (`uv`, `ruff`, `pyrefly`, `pre-commit` hooks)
- [ ] **Arbitrary-Arity Tuples (Python-style)**: Immutable, heterogeneous $n$-element sequence containers (`tuple_t`) with GC tracking and traversal support.
- [ ] **Hash Maps / Dictionaries (`dict_t`)**: Key-value associative mappings with open addressing and bidirectional GC traversal across keys and values.
- [ ] **Doubly Linked Lists (`linked_list_t`)**: Node-based bidirectional lists with intentional cyclic node references (`next`/`prev`) to stress-test cyclic GC reclamation.
- [ ] **Dynamic Slices & Byte Buffers (`slice_t`)**: Non-owning sub-views and resizable contiguous byte storage.
- [ ] **Function Objects & Closures (`closure_t`)**: First-class callable objects capturing lexical environments and variable bindings.
- [ ] **Interactive Memory & GC REPL**: Live terminal CLI (`just run`) to allocate objects, push/pop stack frames, reference handles, and trigger GC passes interactively.
- [ ] **ASCII Heap Visualizer & Object Inspector**: Real-time visual tree inspection of stack frame roots, object reference graphs, and reachable vs unreachable heap states.
- [ ] **Advanced Garbage Collection Algorithms**:
  - [ ] Tri-color Mark-and-Sweep
  - [ ] Generational Garbage Collection
  - [ ] Mark-Compact & Copying collectors
  - [x] ~~Reference Counting with cycle detection~~ (Completed via Hybrid GC)

______________________________________________________________________

## 🛠️ Developer Commands (`justfile`)

This project uses [`just`](https://github.com/casey/just) to automate development workflows:

| Command                      | Description                                                                  |
| :--------------------------- | :--------------------------------------------------------------------------- |
| `just all`                   | Run tests, build the sandbox app, and generate `compile_commands.json`       |
| `just test`                  | Run the unit test suite via µnit with ASan/UBSan and `bootlib` leak tracking |
| `just test-list`             | List all available unit tests                                                |
| `just test-filter <pattern>` | Run only tests matching a name prefix/pattern                                |
| `just coverage`              | Measure line coverage using `gcov` / `llvm-cov`                              |
| `just bench`                 | Compile and run Google Benchmark microbenchmarks                             |
| `just bench-build`           | Compile benchmark runner binary without running it                           |
| `just format`                | Format all C/C++ files in-place using `clang-format`                         |
| `just format-check`          | Check C/C++ formatting compliance without mutating files                     |
| `just format-py`             | Format Python scripts in-place (`ruff format`)                               |
| `just lint`                  | Run `clang-tidy` static analysis + docstring lint + Python checks            |
| `just lint-c`                | Run `clang-tidy` static analysis on C source files                           |
| `just lint-py`               | Check Python scripts (`ruff` + `pyrefly`)                                    |
| `just lint-docs`             | Check Doxygen docstrings across configured dirs                              |
| `just check`                 | Run all pre-commit hooks across the entire repo                              |
| `just build`                 | Compile the main sandbox application binary                                  |
| `just run`                   | Build and execute the sandbox app                                            |
| `just watch`                 | Watch `.c/.h/.cpp/.py` files and auto-rerun tests                            |
| `just debug [filter]`        | Launch lldb on the test suite (optionally filtered)                          |
| `just leaks`                 | Inspect OS-level memory leaks on macOS                                       |
| `just clean`                 | Remove build binaries and gcov artifacts                                     |
| `just compiledb`             | Regenerate `compile_commands.json` for clangd                                |
| `just install-deps`          | Install all macOS dev dependencies via Homebrew                              |
| `just setup-hooks`           | Install pre-commit git hooks                                                 |

> [!NOTE]
> On macOS, `just install-deps` installs `llvm` via Homebrew which provides `clang-tidy`, but it is keg-only. You must add it to your PATH: `export PATH="/opt/homebrew/opt/llvm/bin:$PATH"`

______________________________________________________________________

## 🧪 Continuous Integration

All commits and pull requests automatically trigger GitHub Actions (`.github/workflows/ci.yml`):

1. **pre-commit** — `ruff`, `pyrefly`, `clang-format`, Doxygen docstring lint, and unit tests
1. **clang-tidy** — deep static analysis on `src/*.c` (`just lint-c`)
1. **build** — compile the main sandbox binary (`just build`)
1. **benchmark build** — verify benchmark compilation (`just bench-build`)
1. **coverage** — line coverage via `gcov` (`just coverage`)
