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
| `just test` | Run the unit test suite via µnit with ASan/UBSan and `bootlib` leak tracking |
| `just coverage` | Measure line coverage using `gcov` / `llvm-cov` |
| `just format` | Format code in-place using `clang-format` |
| `just format-check` | Check formatting compliance without mutating files |
| `just lint` | Run static analysis using `clang-tidy` |
| `just build` | Compile the main sandbox application binary |
| `just run` | Build and execute the sandbox app |
| `just clean` | Remove build binaries and gcov artifacts |

---

## 🧪 Continuous Integration

All commits and pull requests automatically trigger GitHub Actions (`.github/workflows/ci.yml`) to verify compilation, test execution, formatting compliance (`clang-format`), and static analysis (`clang-tidy`).
