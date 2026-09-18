/**
 * @file bench_template.cpp
 * @brief Template for Google Benchmark microbenchmarks in trash-gather.
 *
 * Links against C runtime objects compiled with -DBOOTLIB_NO_OVERRIDE.
 */

#include <benchmark/benchmark.h>

extern "C" {
#include "new.h"
#include "object.h"
#include "stack.h"
#include "vm.h"
}

static void BM_ExampleWorkload(
    benchmark::State &state
)
{
    for (auto _ : state)
    {
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
BENCHMARK(BM_ExampleWorkload);

BENCHMARK_MAIN();
