# Milestone: Shared-Nothing VM Worker Pool

**ID:** `c217b6d`\
**Status:** Planned\
**Difficulty:** 3 / 5\
**Focus:** Build a persistent worker thread pool (`vm_pool_t`) where each worker thread maintains a dedicated, isolated VM instance, dispatching tasks through a thread-safe queue and collecting garbage locally.\
**Prerequisites:** [Isolated Worker Thread & Thread-Local VM](99d514b_isolated_worker_thread.md), [Thread-Safe Task Queue (Mutex & Condvar)](714027a_thread_safe_task_queue.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   1. Implement a persistent multi-threaded worker pool (`vm_pool_t`) in `src/vm_pool.h` and `src/vm_pool.c` that instantiates $N$ worker threads during pool initialization (`vm_pool_new`).
   2. Configure each worker thread to initialize its own dedicated `vm_t` once on thread boot and retain it across tasks, amortizing VM initialization costs over thousands of tasks.
   3. Provide a task submission interface (`vm_pool_submit`) pushing jobs to an internal `task_queue_t`.
   4. Establish an independent GC cadence per worker: each thread runs mark-and-sweep cycle collection locally between task executions without stalling sibling workers.
   5. Implement a robust shutdown and cleanup sequence (`vm_pool_shutdown`, `vm_pool_free`) that flushes remaining queue tasks, signals workers, joins all threads, destroys all worker VMs, and verifies zero memory leaks.
2. **Scope Boundaries**:
   1. Dynamic object marshaling across worker heaps is deferred to Milestone `dac4aea`. Tasks operate on plain C scalar or pointer payloads.
   2. Dynamic worker auto-scaling and thread affinity pinning are out of scope.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:

   ```text
   vm_pool_t
   ┌────────────────────────────────────────────────────────────┐
   │ size_t num_workers                                         │
   │ pthread_t *threads                                         │
   │ task_queue_t *queue                                        │
   │ _Atomic bool is_running                                    │
   └───────────────┬────────────────────────────────────────────┘
                   │
                   ▼
     ┌──────────────────────────────────────────────────────────┐
     │ Shared task_queue_t (Mutex + Condition Variables)         │
     └───────┬──────────────────────┬────────────────────┬──────┘
             │                      │                    │
             ▼                      ▼                    ▼
      Worker Thread 0        Worker Thread 1      Worker Thread N-1
     ┌─────────────────┐    ┌─────────────────┐  ┌─────────────────┐
     │ _Thread_local VM│    │ _Thread_local VM│  │ _Thread_local VM│
     │  vm_new() once  │    │  vm_new() once  │  │  vm_new() once  │
     │  Local GC       │    │  Local GC       │  │  Local GC       │
     └─────────────────┘    └─────────────────┘  └─────────────────┘
   ```

   `vm_pool_t` definition:

   ```c
   typedef void (*vm_task_fn_t)(void *arg);

   typedef struct VirtualMachinePool {
       size_t num_workers;
       pthread_t *worker_threads;
       task_queue_t *queue;
       _Atomic bool is_running;
   } vm_pool_t;
   ```

2. **Core Systems Invariants**:
   1. **Persistent Local VM Invariant**: A worker thread invokes `vm_new()` exactly once upon thread startup and `vm_free()` exactly once upon thread exit, preserving thread-local heap isolation across all task executions.
   2. **Zero-Contention Computation Invariant**: Mutex contention is strictly confined to pulling tasks from `task_queue_t`. All object allocation, frame tracking, and GC cycles occur with zero lock acquisition.
   3. **Sequential Join Invariant**: `vm_pool_shutdown()` must first close the queue, then sequentially join every worker thread via `pthread_join()`, guaranteeing that no worker thread is actively executing or touching memory when `vm_pool_free()` tears down the pool.
   4. **Clean Root Reset Invariant**: Between successive tasks, a worker thread must clear or pop any frame roots it pushed, preventing task-leaked root references from pinning memory into subsequent task iterations.
3. **Architectural Trade-offs**:
   1. Maintaining $N$ persistent VM instances consumes more baseline memory than spawning ephemeral threads, but eliminates the substantial kernel syscall and initialization overhead of thread creation on every task dispatch.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   1. Thread pool architectures and task distribution paradigms.
   2. Cost amortization of runtime resources (allocators, stacks, symbol tables).
   3. Amdahl's Law and limits of parallel speedup under shared synchronization.
2. **Socratic Inquiries**:
   1. What is the CPU and memory cost difference between creating a new `pthread_t` and `vm_t` for every single task versus dispatching tasks to a persistent pool of 4 worker threads?
   2. How does the pool coordinate shutting down worker threads that are currently blocked in `pthread_cond_wait` waiting for tasks to arrive?
   3. Why must each worker thread perform garbage collection on its own thread rather than having a centralized coordinator thread collect garbage for all workers?
   4. What would happen if a worker task pushed frames to `vm->frames` but failed to pop them before the task returned? How would this impact subsequent tasks assigned to that same worker?
3. **Failure Modes & Pitfalls**:
   1. Deadlocks during shutdown if worker threads are waiting on a queue that is never closed or broadcast.
   2. Memory leaks caused by freeing the pool struct before worker threads finish executing and terminate.
   3. Worker thread state pollution across tasks when previous task roots are not cleared.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. Define `vm_pool_t` and API declarations in `src/vm_pool.h`.
   2. Implement worker thread entry function `static void *vm_pool_worker_loop(void *arg)` in `src/vm_pool.c`.
   3. Implement `vm_pool_new(size_t num_workers, size_t queue_capacity)`.
   4. Implement `vm_pool_submit(vm_pool_t *pool, vm_task_fn_t fn, void *arg)`.
   5. Implement `vm_pool_shutdown(vm_pool_t *pool)` and `vm_pool_free(vm_pool_t *pool)`.
   6. Update `justfile` recipes to include `src/vm_pool.c`.
   7. Create multithreaded stress tests in `tests/test_vm_pool.c` testing massive task throughput, parallel GC passes, and clean shutdown.
2. **File Touchpoints**:
   1. `src/vm_pool.h`
   2. `src/vm_pool.c`
   3. `justfile`
   4. `tests/test_vm_pool.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**:
   1. Concurrency throughput: Submit 20,000 tasks across a 4-worker pool where each task allocates 100 objects, pushes frames, and triggers periodic GC, verifying all tasks complete without memory corruption.
   2. Immediate shutdown test: Initialize a pool, submit 500 tasks, and immediately invoke `vm_pool_shutdown()`, verifying clean teardown without dropped worker joins or deadlocks.
   3. Single-worker edge case: Verify correct execution and FIFO processing when pool size is configured to 1 worker.
2. **Zero-Leak Guarantee**: All pool memory, queue buffers, worker threads, and VM heap objects are confirmed 100% freed via `assert(boot_all_freed())`.
3. **Tooling Quality Gates**: `just test` (100% pass under ASan/UBSan), `just lint`, and `just check` pass with zero compiler warnings.
4. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update writeup and roadmap status, and generate educational lesson in `lessons/` following the `lesson-extraction` skill.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Thread Pools and Worker Queues (POSIX Design Patterns)](https://hpc-tutorials.llnl.gov/posix/): Design patterns for thread pools, synchronization, and persistent worker loops.
   - [Amdahl's Law and Parallel Scalability](https://en.wikipedia.org/wiki/Amdahl%27s_law): Theoretical principles governing multi-core parallel speedup and synchronization bottlenecks.
2. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython concurrent.futures ProcessPoolExecutor](https://github.com/python/cpython/blob/main/Lib/concurrent/futures/process.py): How Python structures isolated worker pools with separate interpreter processes.
   - [Nginx Worker Architecture and Process Model](https://www.nginx.com/blog/inside-nginx-how-we-designed-for-performance-scale/): Production architecture utilizing shared-nothing worker instances for multi-core scale.
