# Milestone: Cross-VM Value Marshaling & Result Channels

**ID:** `dac4aea`\
**Status:** Planned\
**Difficulty:** 3 / 5\
**Focus:** Bridge isolated thread heaps by implementing deep-copy value marshaling and thread-synchronized result channels (futures), preventing cross-heap pointer aliasing and GC use-after-free bugs.\
**Prerequisites:** [Shared-Nothing VM Worker Pool](c217b6d_shared_nothing_vm_worker_pool.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   1. Design an intermediate marshaling format (`vm_marshal_t`) in `src/channel.h` and `src/channel.c` capable of serializing scalar primitives (integers, booleans, None) and immutable structures (tuples) from a source VM heap into an independent intermediate payload.
   2. Implement a deserializer that reconstructs marshaled payloads as fresh objects within a recipient VM's heap, establishing complete heap isolation without raw pointer sharing.
   3. Implement a thread-safe Future/Promise abstraction (`future_t`) supporting non-blocking status queries and blocking waits (`future_get`) via POSIX condition variables.
   4. Extend the VM worker pool (`vm_pool_submit_future`) to allow asynchronous task dispatch that returns a `future_t` handle for evaluating parallel jobs.
2. **Scope Boundaries**:
   1. Arbitrary cyclic graph serialization is deferred to Milestone `402c62c` (Binary Heap Graph Serialization). This milestone supports trees and DAGs of primitives and tuples.
   2. Shared-memory zero-copy IPC and network serialization are explicitly out of scope.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:

   ```text
   Sender Thread (VM A)               Intermediate Channel              Receiver Thread (VM B)
   ┌─────────────────────┐            ┌──────────────────────┐          ┌─────────────────────┐
   │ object_t *src_tuple │            │ future_t             │          │ object_t *dst_tuple │
   │ (Heap A)            │            │  pthread_mutex_t     │          │ (Heap B)            │
   └──────────┬──────────┘            │  pthread_cond_t      │          └──────────▲──────────┘
              │                       │  vm_marshal_t *data  │                     │
              │ marshal_object()      └──────────▲───────────┘                     │ unmarshal_object()
              ▼                                  │                                 │
     ┌───────────────────────────────────────────┴─────────────────────────────────┤
     │ Raw byte buffer / serialized tagged tree:                                   │
     │ [TAG_TUPLE, len=2] -> [[TAG_INT, 42], [TAG_INT, 100]]                       │
     └─────────────────────────────────────────────────────────────────────────────┘
   ```

   `future_t` and `vm_marshal_t` definitions:

   ```c
   typedef struct VirtualMachineMarshal {
       uint8_t *bytes;
       size_t size;
   } vm_marshal_t;

   typedef struct Future {
       pthread_mutex_t lock;
       pthread_cond_t ready_cond;
       bool is_ready;
       vm_marshal_t *payload;
   } future_t;
   ```

2. **Core Systems Invariants**:
   1. **Zero Pointer Aliasing Invariant**: No raw pointer to an `object_t` in VM A may ever be stored in a `future_t` or written to VM B's memory. All cross-thread transfers must transition through `vm_marshal_t`.
   2. **Recipient Ownership Invariant**: Deserializing a payload into VM B allocates new objects exclusively within VM B's heap, with initial reference counts of 1 and proper registration in `vm_b->objects`.
   3. **Single Resolution Invariant**: A `future_t` is fulfilled exactly once by the worker thread upon task completion. Subsequent reads by the coordinator return the resolved payload without re-executing.
   4. **Independent GC Safety Invariant**: Garbage collection runs on VM A or VM B at any moment without affecting in-flight futures, because intermediate marshal buffers exist entirely outside both GC heaps.
3. **Architectural Trade-offs**:
   1. Deep-copy marshaling incurs memory allocation and copy overhead compared to shared-memory pointer passing, but completely eliminates data races, lock contention, and cross-thread garbage collection synchronization pauses.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   1. The Actor Model and shared-nothing concurrency (Erlang, Pony, Rust).
   2. Futures and Promises for asynchronous synchronization.
   3. Data marshaling, endianness, and memory layout serialization.
2. **Socratic Inquiries**:
   1. What happens if Thread A passes a raw `object_t *obj` pointer to Thread B without marshaling, and Thread A then runs `vm_collect_garbage()` while Thread B is still reading `obj`?
   2. Why does Python's `multiprocessing` module require objects to be picklable before passing them through an IPC queue or process pool?
   3. How does deep-copying an immutable tuple from VM A to VM B preserve the invariant that VM B owns all pointers in its object graph?
   4. How does `future_get()` use a condition variable to block the coordinator thread until the worker thread has finished marshaling the result?
3. **Failure Modes & Pitfalls**:
   1. Dangling pointers resulting from accidental shallow copying of container elements across threads.
   2. Deadlocks caused by calling `future_get()` on a future whose worker task crashed or exited without fulfilling the promise.
   3. Memory leaks from allocated marshal buffers that are never retrieved or freed.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. Define `vm_marshal_t` and `future_t` interfaces in `src/channel.h`.
   2. Implement `marshal_object(object_t *obj)` in `src/channel.c` supporting integers, floats, booleans, None, and tuples.
   3. Implement `unmarshal_object(vm_marshal_t *payload, vm_t *target_vm)` in `src/channel.c`.
   4. Implement `future_new()`, `future_set()`, `future_get()`, and `future_free()`.
   5. Implement `vm_pool_submit_future(vm_pool_t *pool, vm_eval_fn_t fn, object_t *arg, vm_t *source_vm)` in `src/vm_pool.c`.
   6. Update `justfile` to compile `src/channel.c`.
   7. Author adversarial tests in `tests/test_channel.c` validating asynchronous evaluation, deep copy verification, and parallel GC stress testing.
2. **File Touchpoints**:
   1. `src/channel.h`
   2. `src/channel.c`
   3. `src/vm_pool.h`
   4. `src/vm_pool.c`
   5. `justfile`
   6. `tests/test_channel.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**:
   1. Parallel evaluation: Dispatch a task computing Fibonacci numbers inside a worker VM, wait on the `future_t`, unpack the result into the main VM, and verify mathematical correctness.
   2. Nested tuple marshaling: Serialize a 3-level deep nested tuple of integers and booleans, deserialize into a separate VM, and assert that all values match and pointer addresses are completely disjoint.
   3. GC isolation test: Force a sweep on the worker VM immediately before fulfilling the future, and force a sweep on the main VM immediately after receiving it, asserting zero memory corruption or use-after-free faults.
   4. Abandoned future cleanup: Create and immediately free an unfulfilled future, ensuring all synchronization primitives are cleanly destroyed.
2. **Zero-Leak Guarantee**: All intermediate serialization buffers, futures, and VM objects are confirmed 100% freed via `assert(boot_all_freed())`.
3. **Tooling Quality Gates**: `just test` (100% pass under ASan/UBSan), `just lint`, and `just check` pass with zero warnings.
4. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update writeup and roadmap status, and generate educational lesson in `lessons/` following the `lesson-extraction` skill.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Futures and Promises Pattern (Wikipedia)](https://en.wikipedia.org/wiki/Futures_and_promises): Standard architectural pattern for asynchronous synchronization and delayed value delivery.
   - [Data Marshaling and Serialization Principles](https://en.wikipedia.org/wiki/Marshalling_(computer_science)): Systems concepts behind serializing in-memory structures for thread-safe cross-boundary transmission.
2. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython multiprocessing.queues and Pickling](https://github.com/python/cpython/blob/main/Lib/multiprocessing/queues.py): How Python marshals objects across process and subinterpreter boundaries.
   - [Pony Reference Capabilities and Concurrency](https://tutorial.ponylang.io/reference-capabilities/reference-capabilities.html): Advanced language design for zero-copy memory safety and isolated message passing across concurrent actors.
