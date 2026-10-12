# Milestone: Isolated Worker Thread & Thread-Local VM

**ID:** `99d514b`\
**Status:** Planned\
**Difficulty:** 1 / 5\
**Focus:** Introduce thread-local VM context via ISO C17 `_Thread_local`, POSIX worker thread spawning (`vm_thread_spawn`, `vm_thread_join`), and parallel garbage collection across isolated execution heaps.\
**Prerequisites:** [Hybrid Reference Counting & Cycle Collection Runtime](393f420_hybrid_gc_runtime.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   1. Migrate the runtime's global VM pointer from a process-wide static variable to thread-local storage (`_Thread_local static vm_t *CURRENT_VM = NULL;`), enabling each OS thread to possess an independent active VM.
   2. Introduce a lightweight worker thread abstraction (`vm_thread_t`) with `vm_thread_spawn()` and `vm_thread_join()` wrapping POSIX threads (`<pthread.h>`).
   3. Implement a dedicated worker trampoline that automatically initializes a fresh `vm_t` on thread boot, runs user task callbacks, invokes `vm_free()` on thread exit, and returns computation results cleanly.
   4. Update testing infrastructure (`vendor/bootlib/bootlib.c`) with mutex synchronization so memory tracking and leak verification (`boot_all_freed()`) remain safe under concurrent execution.
2. **Scope Boundaries**:
   1. Persistent worker pools, task queues, and worker reuse are deferred to Milestone `714027a` and `c217b6d`.
   2. Cross-thread object sharing or pointer transfer across VM heaps is strictly disallowed; worker threads execute strictly within isolated heaps.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:

   ```text
   Main Thread (CPU Core 0)                    Worker Thread (CPU Core 1)
   -------------------------                    --------------------------
   pthread_self() = A                           pthread_self() = B
   _Thread_local CURRENT_VM                     _Thread_local CURRENT_VM
          │                                            │
          ▼                                            ▼
     ┌───────────┐                                ┌───────────┐
     │   vm_t    │ (Heap A)                       │   vm_t    │ (Heap B)
     ├───────────┤                                ├───────────┤
     │  frames   │                                │  frames   │
     │  objects  │                                │  objects  │
     │ immortals │                                │ immortals │
     └───────────┘                                └───────────┘
   ```

   `vm_thread_t` wrapper layout:

   ```c
   typedef void *(*vm_worker_fn_t)(void *arg);

   typedef struct VirtualMachineThread {
       pthread_t handle;
       vm_worker_fn_t worker_fn;
       void *arg;
       void *result;
       bool joined;
   } vm_thread_t;
   ```

2. **Core Systems Invariants**:
   1. **Heap Disjointness Invariant**: An `object_t` allocated in Heap A must never contain pointers to Heap B, nor may Heap B contain pointers to Heap A.
   2. **Zero-Lock VM Execution**: Because each thread owns a distinct `vm_t` instance, allocation, stack pushing, frame tracking, and garbage collection (`vm_collect_garbage()`) operate concurrently without holding any global runtime locks.
   3. **Trampoline Lifecycle Invariant**: The worker trampoline guarantees that `vm_new()` is invoked prior to executing `worker_fn`, and `vm_free()` is unconditionally called upon return before the thread terminates.
   4. **Single-Join Ownership Invariant**: Every successfully spawned `vm_thread_t` must be joined exactly once by the coordinator thread via `vm_thread_join()` to reclaim OS thread resources and free the wrapper struct.
3. **Architectural Trade-offs**:
   1. Using `_Thread_local` provides fast, lock-free access to the current thread's VM at the cost of requiring explicit serialization or value marshaling whenever data must cross thread boundaries.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   1. Storage duration in ISO C17 (automatic, static, allocated, and thread-local).
   2. POSIX threads (`pthread_create`, `pthread_join`) execution and lifecycle model.
   3. Shared-nothing memory architecture vs shared-memory concurrency with synchronization barriers.
2. **Socratic Inquiries**:
   1. Why did C11 introduce the `_Thread_local` keyword rather than relying on vendor-specific extensions such as `__thread` or `__declspec(thread)`?
   2. What occurs to operating system resources if a thread finishes execution but is never joined (`pthread_join`) or detached (`pthread_detach`)?
   3. If thread A passes a pointer to a stack variable (`int x = 42; vm_thread_spawn(fn, &x);`) and thread A exits its function before thread B reads `x`, what undefined behavior takes place?
   4. Why can two separate threads run mark-and-sweep garbage collection concurrently without a Stop-The-World (STW) pause, provided their heaps are disjoint?
3. **Failure Modes & Pitfalls**:
   1. Stack-use-after-return when passing thread arguments without heap allocation.
   2. Zombie threads caused by neglected `pthread_join` calls.
   3. Data races in global tracking allocators (`bootlib`) when multiple threads allocate simultaneously without a tracking lock.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. In `src/vm.c`, change `static vm_t *CURRENT_VM = NULL;` to `_Thread_local static vm_t *CURRENT_VM = NULL;`.
   2. In `vendor/bootlib/bootlib.c`, introduce a `pthread_mutex_t` protecting `g_allocs` tracking tables, ensuring thread-safe allocation accounting.
   3. Create `src/vm_thread.h` defining `vm_thread_t`, `vm_thread_spawn()`, and `vm_thread_join()`.
   4. Implement `src/vm_thread.c` containing the worker trampoline that initializes the local VM, runs the callback, and destroys the local VM.
   5. Update `justfile` build and test recipes to compile and link `src/vm_thread.c` with `-pthread`.
   6. Author adversarial tests in `tests/test_vm_thread.c` verifying concurrent allocations, parallel GC runs, and leak-free joins.
2. **File Touchpoints**:
   1. `src/vm.c`
   2. `src/vm_thread.h`
   3. `src/vm_thread.c`
   4. `vendor/bootlib/bootlib.c`
   5. `justfile`
   6. `tests/test_vm_thread.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**:
   1. Spawning a worker thread that creates 5,000 objects and runs GC while the main thread simultaneously creates 5,000 objects and runs GC.
   2. Spawning and joining 20 sequential worker threads to verify thread cleanup and OS handle reclamation.
   3. Returning scalar results, pointer payloads, and NULL from worker threads across `vm_thread_join()`.
2. **Zero-Leak Guarantee**: All allocations (VMs, frame stacks, object stacks, thread wrappers) are verified 100% freed via `assert(boot_all_freed())`.
3. **Tooling Quality Gates**: `just test` passes with zero leaks and clean ASan/UBSan diagnostics; `just lint` and `just check` pass.
4. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update writeup and roadmap status, and generate educational lesson in `lessons/` following the `lesson-extraction` skill.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [POSIX Threads Programming (LLNL Tutorial)](https://hpc-tutorials.llnl.gov/posix/): In-depth guide to thread creation, arguments, joining, and POSIX synchronization primitives.
   - [ISO C17 Thread-local Storage Duration (cppreference)](https://en.cppreference.com/w/c/language/storage_duration): Authoritative explanation of thread-local storage duration and keyword syntax in standard C.
2. **After Implementation (Deep Dives & Systems Context)**:
   - [PEP 684: A Per-Interpreter GIL in CPython](https://peps.python.org/pep-0684/): How CPython decoupled global state to provide thread-isolated interpreters with distinct heaps.
   - [Erlang Processes and Shared-Nothing Memory Architecture](https://www.erlang.org/doc/system/conc_prog.html): Systems design principles of isolated heaps and independent garbage collection per thread/process.
