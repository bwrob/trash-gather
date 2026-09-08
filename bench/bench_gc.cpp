#include <benchmark/benchmark.h>

extern "C" {
#include "sneknew.h"
#include "snekobject.h"
#include "stack.h"
#include "vm.h"
}

static void BM_ObjectAllocation(benchmark::State& state) {
  for (auto _ : state) {
    vm_t* vm = vm_new();
    for (int i = 0; i < 1000; ++i) {
      new_snek_integer(vm, i);
    }
    vm_free(vm);
  }
}
BENCHMARK(BM_ObjectAllocation);

static void BM_GarbageCollection_Sweep(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    vm_t* vm = vm_new();
    // Allocate 5,000 transient objects
    for (int i = 0; i < 5000; ++i) {
      new_snek_integer(vm, i);
    }
    state.ResumeTiming();

    // Trigger GC pass on 100% unreachable objects
    vm_collect_garbage(vm);

    state.PauseTiming();
    vm_free(vm);
    state.ResumeTiming();
  }
}
BENCHMARK(BM_GarbageCollection_Sweep);

static void BM_GarbageCollection_Retained(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    vm_t* vm = vm_new();
    frame_t* frame = vm_new_frame(vm);
    // Allocate 5,000 objects, retain every 2nd object in stack frame
    for (int i = 0; i < 5000; ++i) {
      snek_object_t* obj = new_snek_integer(vm, i);
      if (i % 2 == 0) {
        frame_reference_object(frame, obj);
      }
    }
    state.ResumeTiming();

    vm_collect_garbage(vm);

    state.PauseTiming();
    vm_free(vm);
    state.ResumeTiming();
  }
}
BENCHMARK(BM_GarbageCollection_Retained);

static void BM_NestedVectorTracing(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    vm_t* vm = vm_new();
    frame_t* frame = vm_new_frame(vm);

    snek_object_t* root = new_snek_integer(vm, 0);
    frame_reference_object(frame, root);

    // Build a deep vector hierarchy
    for (int i = 0; i < 500; ++i) {
      snek_object_t* a = new_snek_integer(vm, i);
      snek_object_t* b = new_snek_integer(vm, i + 1);
      root = new_snek_vector3(vm, root, a, b);
    }
    state.ResumeTiming();

    vm_collect_garbage(vm);

    state.PauseTiming();
    vm_free(vm);
    state.ResumeTiming();
  }
}
BENCHMARK(BM_NestedVectorTracing);

BENCHMARK_MAIN();
