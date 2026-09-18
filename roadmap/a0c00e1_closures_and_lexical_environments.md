# Milestone: Function Objects & Closures (`closure_t`)

**ID:** `a0c00e1`\
**Status:** Planned\
**Difficulty:** 5 / 5\
**Focus:** Implement first-class function objects that capture lexical environment scopes, holding references to parent frames and variables beyond their stack lifetimes.\
**Prerequisites:** [Dictionary Tombstone Deletion & Dynamic Rehashing](e883213_dict_tombstone_deletion_and_rehashing.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**: Implement first-class callable `closure_t` objects capturing lexical environments (upvalues), transitioning frame lifetime management so captured frames outlive stack popping.
1. **Scope Boundaries**: Full bytecode virtual machine interpreters are deferred to bytecode engine milestones; closures invoke C function pointers with environment dictionaries.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Closure structure:

     ```c
     typedef object_t *(*native_fn_t)(object_t *args);

     typedef struct {
       native_fn_t fn;
       char *name;
       object_t *env; // Points to an environment dict or captured frame
     } closure_t;
     ```

1. **Core Systems Invariants**:
   - Upvalue retention invariant: Popping a call frame (`vm_frame_pop()`) must not free captured variables if a live closure holds a reference to that scope.
   - Invocation invariant: Calling a closure binds its captured `env` as the parent scope for the newly pushed execution frame.
   - Tracing invariant: The GC mark phase must blacken the closure's captured `env` and all reachable bound variables.
1. **Architectural Trade-offs**: Hoisting captured stack variables to the heap allows flexible functional programming patterns, but shifts allocation cost from zero-overhead stack frames to garbage-collected heap blocks.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: The upward and downward Funarg Problem; open vs closed upvalues; lexical closures and static scoping.
1. **Socratic Inquiries**:
   - Why does returning a function from an inner scope break simple LIFO call-stack deallocation?
   - How does Lua avoid heap-allocating upvalues until the enclosing frame returns?
   - What reference cycles can arise between closures and environments (e.g. recursive closures)?
1. **Failure Modes & Pitfalls**: Dangling pointer access to popped stack frames; cyclic memory retention in recursive closures; incorrect lexical scope resolution.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Define `closure_t` in `src/object.h`.
   - Implement `new_closure()` in `src/new.c` and `src/new.h`.
   - Update `frame_t` in `src/vm.h` to allow promotion to GC-managed heap scopes.
   - Implement invocation dispatcher `vm_call(object_t *closure, object_t *args)` in `src/vm.c`.
   - Integrate `CLOSURE` into `trace_blacken_object()` in `src/vm.c`.
   - Integrate decref and payload reclamation in `src/object.c`.
   - Write unit tests in `tests/test_closure.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/new.h`, `src/new.c`
   - `src/vm.h`, `src/vm.c`
   - `tests/test_closure.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify higher-order functions (e.g. counter generators), multi-level nested scopes, and parameter binding.
1. **Zero-Leak Guarantee**: Escaped closure test verifies variables survive frame popping and are 100% reclaimed when closure root is dropped via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Lexical Scoping and Closures in Programming Languages](<https://en.wikipedia.org/wiki/Closure_(computer_programming)>): First-class functions, lexical environments, and capturing variables beyond stack scope.
   - [Call Stacks and Activation Records](https://en.wikipedia.org/wiki/Call_stack): Stack frames, activation lifetimes, and moving captured variables from stack to heap.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython Objects/cellobject.c and Upvalue Storage](https://github.com/python/cpython/blob/main/Objects/cellobject.c): How Python uses `cell` objects to share variables between nested lexical closures.
   - [The Implementation of Lua 5.0 (Ierusalimschy et al.)](https://www.lua.org/doc/jucs05.pdf): The classic paper explaining the open vs closed upvalue architecture for efficient closures.
