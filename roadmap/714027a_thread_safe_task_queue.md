# Milestone: Thread-Safe Task Queue (Mutex & Condvar)

**ID:** `714027a`\
**Status:** Planned\
**Difficulty:** 2 / 5\
**Focus:** Implement a concurrent bounded FIFO task queue using POSIX mutexes and condition variables, mastering mutual exclusion, condition wait loops, and thread shutdown protocols.\
**Prerequisites:** [Isolated Worker Thread & Thread-Local VM](99d514b_isolated_worker_thread.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   1. Design and implement a bounded FIFO task queue (`task_queue_t`) in `src/task_queue.h` and `src/task_queue.c` synchronized via `pthread_mutex_t` and `pthread_cond_t`.
   2. Support blocking push operations (`task_queue_push`) that wait when the queue is at capacity, and blocking pop operations (`task_queue_pop`) that wait when the queue is empty.
   3. Provide non-blocking and timeout polling mechanisms (`task_queue_try_pop`) for responsive worker threads.
   4. Implement a clean shutdown protocol (`task_queue_close`) utilizing `pthread_cond_broadcast` to wake all sleeping worker threads and signal queue termination without deadlocks or leaked allocations.
2. **Scope Boundaries**:
   1. Worker thread pool lifecycle management and task execution are deferred to Milestone `c217b6d`.
   2. Lock-free ring buffers, work stealing, and priority scheduling are explicitly out of scope.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:

   ```text
   task_queue_t
   ┌────────────────────────────────────────────────────────┐
   │ pthread_mutex_t lock                                   │
   │ pthread_cond_t not_empty                               │
   │ pthread_cond_t not_full                                │
   │ bool is_closed                                          │
   │ size_t capacity, head, tail, count                      │
   │ task_t *buffer                                         │
   └───────┬────────────────────────────────────────────────┘
           │
           ▼
     ┌───────────┬───────────┬───────────┬───────────┐
     │ task_t[0] │ task_t[1] │  ...      │ task_t[N] │
     └───────────┴───────────┴───────────┴───────────┘
   ```

   `task_t` definition:

   ```c
   typedef void (*task_fn_t)(void *arg);

   typedef struct Task {
       task_fn_t fn;
       void *arg;
   } task_t;
   ```

2. **Core Systems Invariants**:
   1. **Mutual Exclusion Invariant**: Any read or write to `buffer`, `head`, `tail`, `count`, or `is_closed` must occur exclusively while holding `lock`.
   2. **Loop Predicate Invariant**: All condition variable waits (`pthread_cond_wait`) must be enclosed within a `while` loop checking the state predicate (e.g. `while (count == 0 && !is_closed)`), never an `if` condition, to guarantee safety against spurious wakeups.
   3. **Closure & Broadcast Invariant**: When `task_queue_close()` is invoked, `is_closed` is set to true and `pthread_cond_broadcast()` is called on both `not_empty` and `not_full`, ensuring no thread remains suspended indefinitely.
   4. **Destruction Safety Invariant**: `task_queue_free()` requires that the queue is closed and all waiting threads have exited before calling `pthread_mutex_destroy()` and `pthread_cond_destroy()`.
3. **Architectural Trade-offs**:
   1. A contiguous circular ring buffer minimizes heap fragmentation and maximizes CPU cache locality compared to a dynamically allocated linked list of task nodes, at the trade-off of having a bounded capacity that blocks producers under high load.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**:
   1. The classic Producer-Consumer Problem and monitor pattern.
   2. Mesa condition variable semantics vs Hoare semantics (why predicate re-evaluation is necessary).
   3. Spurious wakeups at the OS kernel / hardware boundary.
   4. Signal (`pthread_cond_signal`) vs Broadcast (`pthread_cond_broadcast`).
2. **Socratic Inquiries**:
   1. Why is checking `while (queue_is_empty)` mandatory when calling `pthread_cond_wait`, whereas `if (queue_is_empty)` can cause catastrophic data races or buffer underflows?
   2. What is a "spurious wakeup", and why does the POSIX specification permit it on modern multi-core processors?
   3. Why must `pthread_cond_wait()` atomically release the associated mutex and place the calling thread to sleep? What race condition would occur if these were two separate operations?
   4. When should you use `pthread_cond_broadcast()` instead of `pthread_cond_signal()` during queue shutdown?
3. **Failure Modes & Pitfalls**:
   1. Missed wakeups caused by calling `pthread_cond_signal` without holding the mutex or without state modification.
   2. Deadlocks caused by producer threads blocking on a full queue while consumers are dead or unnotified.
   3. Destroying a mutex or condition variable while another thread is still blocked in `pthread_cond_wait`.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. Define `task_t` and `task_queue_t` in `src/task_queue.h`.
   2. Implement `task_queue_new(size_t capacity)` and `task_queue_free(task_queue_t *queue)` in `src/task_queue.c`.
   3. Implement `task_queue_push(task_queue_t *queue, task_fn_t fn, void *arg)` handling `not_full` condition waits.
   4. Implement `task_queue_pop(task_queue_t *queue, task_t *out_task)` handling `not_empty` condition waits and queue closure.
   5. Implement `task_queue_close(task_queue_t *queue)` using `pthread_cond_broadcast`.
   6. Add compilation targets for `src/task_queue.c` in `justfile`.
   7. Create comprehensive adversarial multi-producer multi-consumer tests in `tests/test_task_queue.c`.
2. **File Touchpoints**:
   1. `src/task_queue.h`
   2. `src/task_queue.c`
   3. `justfile`
   4. `tests/test_task_queue.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**:
   1. Single-producer single-consumer test pushing and popping 50,000 tasks sequentially and verifying FIFO order.
   2. Multiple-producer multiple-consumer stress test (4 producers, 4 consumers) passing 100,000 tasks without dropping or duplicating items.
   3. Shutdown verification: spawning 4 consumers blocked on an empty queue, triggering `task_queue_close()`, and verifying all 4 threads exit cleanly without hanging.
   4. Error validation: pushing to a closed queue fails gracefully returning false.
2. **Zero-Leak Guarantee**: All allocated buffers and synchronization objects are cleanly destroyed and confirmed via `assert(boot_all_freed())`.
3. **Tooling Quality Gates**: `just test` (100% pass under ASan/UBSan), `just lint`, and `just check` pass with zero warnings.
4. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update writeup and roadmap status, and generate educational lesson in `lessons/` following the `lesson-extraction` skill.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Condition Variables and Spurious Wakeups (OSTEP Chapter 30)](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-cv.pdf): Operating Systems: Three Easy Pieces canonical guide to condition variables and the producer-consumer problem.
   - [SEI CERT C CON36-C: Condition Variable Loop Wrapper](https://wiki.sei.cmu.edu/confluence/display/c/CON36-C.+Wrap+functions+that+wait+on+a+condition+variable+in+a+while+loop): Formal C coding standard requiring condition variables to be evaluated in a while loop.
2. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython Python/thread_pthread.h Implementation](https://github.com/python/cpython/blob/main/Python/thread_pthread.h): How CPython wraps POSIX mutexes and condition variables across operating systems.
   - [Lock-Free vs Mutex-Based Queue Architectures (1024cores)](https://www.1024cores.net/home/lock-free-algorithms/queues): Comparative systems review of lock-based synchronization vs lock-free ring buffers.
