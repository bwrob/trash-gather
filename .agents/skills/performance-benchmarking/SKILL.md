---
name: performance-benchmarking
description: >-
  Guidelines, workload patterns, and analysis runbooks for creating and maintaining Google Benchmark
  microbenchmarks in bench/ (e.g., bench/bench_gc.cpp). Use whenever evaluating runtime performance,
  allocation throughput, GC pause times, or pointer traversal latency.
---

# Performance Benchmarking Skill (`performance-benchmarking`)

This skill defines the methodology, workload profiles, and best practices for creating, maintaining, and analyzing microbenchmarks in `trash-gather` using Google Benchmark.

As an AI agent, you are explicitly authorized to create and maintain benchmarks in `bench/` (Role 3 in `AGENTS.md`) to provide empirical data on architectural trade-offs.

______________________________________________________________________

## 🎯 Core Objectives

1. **Quantify Trade-offs**: Measure allocation throughput, GC pause durations, memory fragmentation, and graph traversal overhead.
1. **Isolate Measurements**: Use `state.PauseTiming()` and `state.ResumeTiming()` to ensure setup/teardown costs do not pollute the measured operation.
1. **Realistic Workloads**: Design benchmarks reflecting real-world access patterns (transient churn, deep trees, cyclic clusters, varying retention ratios).
1. **Zero Compiler Tricks**: Prevent dead-code elimination using `benchmark::DoNotOptimize`.

______________________________________________________________________

## 📐 Benchmark Architecture & C/C++ Integration

Benchmarks are written in C++17 utilizing Google Benchmark, linking against the C runtime objects compiled with `-DBOOTLIB_NO_OVERRIDE`.

### Structure of a Benchmark File (`bench/bench_gc.cpp`)

```cpp
#include <benchmark/benchmark.h>

extern "C" {
#include "new.h"
#include "object.h"
#include "stack.h"
#include "vm.h"
}

static void BM_Example(benchmark::State &state) {
  for (auto _ : state) {
    // 1. Setup phase (paused timing)
    state.PauseTiming();
    vm_new();
    // ... setup data structures ...
    state.ResumeTiming();

    // 2. Measured phase (active timing)
    vm_collect_garbage();

    // 3. Teardown phase (paused timing)
    state.PauseTiming();
    vm_free();
    state.ResumeTiming();
  }
}
BENCHMARK(BM_Example);

BENCHMARK_MAIN();
```

______________________________________________________________________

## 🔬 Standard Benchmark Workload Profiles

When benchmarking the runtime or evaluating new data structures, implement across these standard profiles:

### 1. Pure Allocation Throughput

- **Goal**: Measures raw object creation and allocator speed without garbage collection pauses.
- **Pattern**: Allocate $N$ objects in a tight loop (`new_integer`, `new_array`, `new_tuple`).
- **Optimization Guard**: Use `benchmark::DoNotOptimize(obj)` to prevent the compiler from optimizing away unused returned pointers.

### 2. Full Sweep (0% Retention — Pure Garbage)

- **Goal**: Measures maximum reclamation throughput when 100% of objects are unreachable.
- **Pattern**: Pre-allocate $N$ unreferenced objects, start timing, trigger `vm_collect_garbage()`, and record time to reclaim all headers and buffers.

### 3. Partial Root Retention (e.g., 50% Live / 50% Dead)

- **Goal**: Simulates standard application execution where long-lived state coexists with transient allocations.
- **Pattern**: Allocate $N$ objects, registering every $k$-th object into active call frames (`frame_reference_object`). Measure `vm_collect_garbage()` pause time during mixed live/dead sweeps.

### 4. Deep Graph Traversal (Stack Depth & Cache Locality)

- **Goal**: Measures pointer traversal overhead and CPU L1/L2 cache locality during tracing.
- **Pattern**: Construct deep linear or binary hierarchies (e.g., 500-level deep `vector3` or linked lists). Measure trace duration from root to leaf.

### 5. Cyclic Cluster Reclamation (Hybrid GC Specific)

- **Goal**: Quantifies cycle collector pause latency when resolving isolated multi-node cyclic meshes vs. linear objects.
- **Pattern**: Construct rings of arrays ($A \\rightarrow B \\rightarrow C \\rightarrow A$) unrooted, measure cycle detection and sweep latency.

______________________________________________________________________

## 🛠️ Execution & Profiling Workflow

All benchmarks are orchestrated via `just`:

| Command                                           | Action                                                              |
| :------------------------------------------------ | :------------------------------------------------------------------ |
| `just bench-build`                                | Compile the benchmark runner (`bin/bench_runner`) without executing |
| `just bench`                                      | Build and run all benchmarks                                        |
| `./bin/bench_runner --benchmark_filter=<regex>`   | Run only benchmarks matching a regex pattern                        |
| `./bin/bench_runner --benchmark_repetitions=5`    | Run multiple iterations for statistical significance                |
| `./bin/bench_runner --benchmark_out=results.json` | Export results to JSON for comparison                               |

______________________________________________________________________

## 📊 Analyzing Benchmark Results

When reporting benchmark comparisons to the user:

- **Baseline vs. Candidate**: Always compare before/after numbers side-by-side.
- **Identify the Bottleneck**: Distinguish allocator latency (`malloc`/`free`) from runtime pointer traversal or array compaction (`stack_remove_nulls`).
- **Empirical Guidance**: Use benchmark data to guide architectural decisions (e.g., proving whether untracking primitive types significantly accelerates GC pause times).
