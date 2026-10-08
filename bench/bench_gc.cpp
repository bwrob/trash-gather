/**
 * @file bench_gc.cpp
 * @brief Google Benchmark suite for Garbage Collector throughput, pause
 * durations, and pointer graph traversal.
 */

#include <benchmark/benchmark.h>

extern "C"
{
#include "object.h"
#include "stack.h"
#include "vm.h"
}

/**
 * @brief Benchmark object allocation throughput without garbage collection.
 * @param state Google Benchmark state iterator.
 */
static void BM_ObjectAllocation(
    benchmark::State &state
)
{
    for (auto _ : state)
    {
        vm_new();
        for (int i = 0; i < 1000; ++i)
        {
            integer_new(i);
        }
        vm_free();
    }
}
BENCHMARK(
    BM_ObjectAllocation
);

/**
 * @brief Benchmark mark-and-sweep GC pause time when 100% of objects are
 * unreachable.
 * @param state Google Benchmark state iterator.
 */
static void BM_GarbageCollection_Sweep(
    benchmark::State &state
)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        vm_new();
        // Allocate 5,000 transient objects
        for (int i = 0; i < 5000; ++i)
        {
            integer_new(i);
        }
        state.ResumeTiming();

        // Trigger GC pass on 100% unreachable objects
        vm_collect_garbage();

        state.PauseTiming();
        vm_free();
        state.ResumeTiming();
    }
}
BENCHMARK(
    BM_GarbageCollection_Sweep
);

/**
 * @brief Benchmark mark-and-sweep GC pause time when 50% of objects are
 * retained in root frames.
 * @param state Google Benchmark state iterator.
 */
static void BM_GarbageCollection_Retained(
    benchmark::State &state
)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        vm_new();
        frame_t *frame = vm_new_frame();
        // Allocate 5,000 objects, retain every 2nd object in stack frame
        for (int i = 0; i < 5000; ++i)
        {
            object_t *obj = integer_new(i);
            if (i % 2 == 0)
            {
                frame_reference_object(frame, obj);
            }
        }
        state.ResumeTiming();

        vm_collect_garbage();

        state.PauseTiming();
        vm_free();
        state.ResumeTiming();
    }
}
BENCHMARK(
    BM_GarbageCollection_Retained
);

/**
 * @brief Benchmark recursive pointer graph tracing performance across deep
 * object trees.
 * @param state Google Benchmark state iterator.
 */
static void BM_NestedVectorTracing(
    benchmark::State &state
)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        vm_new();
        frame_t *frame = vm_new_frame();

        object_t *root = integer_new(0);
        frame_reference_object(frame, root);

        // Build a deep vector hierarchy
        for (int i = 0; i < 500; ++i)
        {
            object_t *a = integer_new(i);
            object_t *b = integer_new(i + 1);
            root = tuple_new_3(root, a, b);
        }
        state.ResumeTiming();

        vm_collect_garbage();

        state.PauseTiming();
        vm_free();
        state.ResumeTiming();
    }
}
BENCHMARK(
    BM_NestedVectorTracing
);

BENCHMARK_MAIN();
