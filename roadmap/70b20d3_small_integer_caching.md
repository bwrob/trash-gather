# Milestone: Small Integer Caching

**ID:** `70b20d3`\
**Status:** Planned\
**Focus:** Pre-allocate an immortal static cache of small integer objects (`[-128, 127]`), eliminating heap allocation churn for common numbers and introducing pointer identity semantics.\
**Prerequisites:** [The None Immortal Singleton Object](fc1cc81_none_immortal_singleton.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Initialize a lookup table of 256 pre-allocated, immortal `object_t` integer instances spanning `[-128, 127]` during VM initialization; route `new_integer(n)` to return cached pointers when within range; guarantee that garbage collection sweeps never deallocate cached integers; introduce pointer identity testing (`object_is(a, b)`).
1. **Scope Boundaries**: Arbitrary-precision integers (`bignum`) and floating-point flyweights are explicitly deferred to future numeric milestones.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Array of 256 immortal integer object pointers embedded in `vm_t`:
     ```
     [VM Runtime State]
       └── small_ints[-128..127]
             ├── [-128] ──> [Object: INT -128 (rc=IMMORTAL, marked=1)]
             ├── [   0] ──> [Object: INT    0 (rc=IMMORTAL, marked=1)]
             ├── [   1] ──> [Object: INT    1 (rc=IMMORTAL, marked=1)]
             └── [ 127] ──> [Object: INT  127 (rc=IMMORTAL, marked=1)]

     User References:
       var 'x' = 1  ───┐
                       ├───> Same Pointer (Pointer Identity: x is y == true)
       var 'y' = 1  ───┘
     ```
1. **Core Systems Invariants**:
   - Immortality invariant: Small integer objects are marked as immortal (`is_immortal = true`), ensuring neither immediate `dec_refcount()` nor `gc_sweep()` can ever free them during program execution.
   - Immutability invariant: Integer payload values within the cache table are strictly read-only and must never be modified after VM boot.
   - Identity consistency invariant: For any two integer allocations `a` and `b` with identical values within `[-128, 127]`, `new_integer(val)` must always return the exact same memory address (`a == b`).
1. **Architectural Trade-offs**: Static memory footprint (256 objects $\\approx$ 6-8 KB) vs heap allocation churn; because small integers dominate loops, sequence indexing, and arithmetic counters, pre-allocating them eliminates thousands of `malloc`/`free` calls per second.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Flyweight design pattern; pointer identity (`is`) vs value equality (`==`); CPython `NSMALLPOSINTS`/`NSMALLNEGINTS` optimization; JVM `IntegerCache`.
1. **Socratic Inquiries**:
   - Why do Python expressions like `a = 100; b = 100; a is b` evaluate to `True`, while `a = 1000; b = 1000; a is b` evaluates to `False`?
   - How does refcount management behave when an immortal object is shared across dozens of containers without ever reaching zero?
   - If an immortal integer is placed inside a dead cyclic structure, how should the cycle collector handle it during the mark and sweep phases?
1. **Failure Modes & Pitfalls**: Forgetting to check the immortal flag during `gc_sweep()`, causing double-free crashes; modifying an integer's value in-place, which silently corrupts all other variables in the runtime sharing that cached pointer.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   1. Define small integer cache bounds (`SMALL_INT_MIN = -128`, `SMALL_INT_MAX = 127`) in `include/object.h`.
   1. Add the cache pointer array `object_t *small_ints[...]` to `vm_t` in `include/vm.h`.
   1. Pre-allocate and initialize the 256 cached integer objects during `vm_new()` in `src/vm.c`.
   1. Update `new_integer()` in `src/new.c` to check bounds and return the cached pointer on hits, falling back to dynamic allocation on misses.
   1. Ensure `vm_free()` cleanly releases the static cache upon engine shutdown.
   1. Write unit tests in `tests/test_object.c` verifying pointer identity, out-of-bounds fresh allocation, and GC sweep survival.
1. **File Touchpoints**:
   1. `include/object.h`
   1. `include/vm.h`
   1. `src/vm.c`
   1. `src/new.c`
   1. `tests/test_object.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Assert that `new_integer(42) == new_integer(42)`, assert that `new_integer(1000) != new_integer(1000)` (distinct addresses), and assert that running multiple GC cycles retains all small integer pointers without memory corruption.
1. **Zero-Leak Guarantee**: Shutting down the VM releases all cached integer objects cleanly with zero remaining allocations, verified via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero warnings under ASan/UBSan.
