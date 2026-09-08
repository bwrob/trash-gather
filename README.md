# trash-gather

Explorations in automatic memory management and virtual machine runtime implementation in C.

---

## 🚀 About & Boot.dev Origin

This project is a direct continuation and expansion of the **Memory Management & Garbage Collector** curriculum on [Boot.dev](https://www.boot.dev).

> [!IMPORTANT]
> **Endorsement**: [Boot.dev](https://www.boot.dev) is an incredible hands-on platform for learning backend development, systems programming, C, memory management, and computer science fundamentals. If you're interested in building real projects from scratch—from virtual machines to backend servers—we enthusiastically recommend checking out [Boot.dev](https://www.boot.dev)!

---

## 🎯 Roadmap & Extended Architecture

While the initial codebase established a baseline Mark-and-Sweep garbage collection runtime for Sneklang, this repository is actively expanding into advanced runtime mechanics:

- [x] **Core Mark-and-Sweep GC & VM Baseline**
- [x] **Tooling, CI & Safety Infrastructure** (ASan/UBSan, `bootlib`, `clang-format`, `clang-tidy`, Google Benchmark)
- [x] **Python Toolchain** (`uv`, `ruff`, `pyrefly`, `pre-commit` hooks)
- [ ] **Interactive Memory & GC REPL**: Live terminal CLI (`just run`) to allocate objects, push/pop stack frames, reference handles, and trigger GC passes interactively.
- [ ] **ASCII Heap Visualizer & Object Inspector**: Real-time visual tree inspection of stack frame roots, object reference graphs, and reachable vs unreachable heap states.
- [ ] **New Data Structures & Containers**: Custom hash maps, doubly linked lists, and dynamic buffer slices.
- [ ] **Expanded Object System**: Tuples, dictionaries, function objects, and environment closures.
- [ ] **Advanced Garbage Collection Algorithms**:
  - [ ] Tri-color Mark-and-Sweep
  - [ ] Generational Garbage Collection
  - [ ] Mark-Compact & Copying collectors
  - [ ] Reference Counting with cycle detection

---

## 🛠️ Developer Commands (`justfile`)

This project uses [`just`](https://github.com/casey/just) to automate development workflows:

| Command | Description |
| :--- | :--- |
| `just all` | Run tests, build the sandbox app, and generate `compile_commands.json` |
| `just test` | Run the unit test suite via µnit with ASan/UBSan and `bootlib` leak tracking |
| `just test-list` | List all available unit tests |
| `just test-filter <pattern>` | Run only tests matching a name prefix/pattern |
| `just coverage` | Measure line coverage using `gcov` / `llvm-cov` |
| `just bench` | Compile and run Google Benchmark microbenchmarks |
| `just bench-build` | Compile benchmark runner binary without running it |
| `just format` | Format all C/C++ files in-place using `clang-format` |
| `just format-check` | Check C/C++ formatting compliance without mutating files |
| `just format-py` | Format Python scripts in-place (`ruff format`) |
| `just lint` | Run `clang-tidy` static analysis + docstring lint + Python checks |
| `just lint-c` | Run `clang-tidy` static analysis on C source files |
| `just lint-py` | Check Python scripts (`ruff` + `pyrefly`) |
| `just lint-docs` | Check Doxygen docstrings across configured dirs |
| `just check` | Run all pre-commit hooks across the entire repo |
| `just build` | Compile the main sandbox application binary |
| `just run` | Build and execute the sandbox app |
| `just watch` | Watch `.c/.h/.cpp/.py` files and auto-rerun tests |
| `just debug [filter]` | Launch lldb on the test suite (optionally filtered) |
| `just leaks` | Inspect OS-level memory leaks on macOS |
| `just clean` | Remove build binaries and gcov artifacts |
| `just compiledb` | Regenerate `compile_commands.json` for clangd |
| `just install-deps` | Install all macOS dev dependencies via Homebrew |
| `just setup-hooks` | Install pre-commit git hooks |

> [!NOTE]
> On macOS, `just install-deps` installs `llvm` via Homebrew which provides `clang-tidy`, but it is keg-only. You must add it to your PATH: `export PATH="/opt/homebrew/opt/llvm/bin:$PATH"`

---

## 🧪 Continuous Integration

All commits and pull requests automatically trigger GitHub Actions (`.github/workflows/ci.yml`):

1. **pre-commit** — `ruff`, `pyrefly`, `clang-format`, Doxygen docstring lint, and unit tests
2. **clang-tidy** — deep static analysis on `src/*.c` (`just lint-c`)
3. **build** — compile the main sandbox binary (`just build`)
4. **benchmark build** — verify benchmark compilation (`just bench-build`)
5. **coverage** — line coverage via `gcov` (`just coverage`)
