# Milestone: Doubly Linked Lists (`linked_list_t`)

**ID:** `11c2c6a`\
**Status:** Planned\
**Focus:** Implement node-based doubly linked lists with mutual `prev`/`next` reference cycles to stress-test cyclic GC discovery, traversal, and reclamation.\
**Prerequisites:** [Full CPython-Style Offset-0 Hierarchy](222f6ce_cpython_offset0_hierarchy.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Implement a bidirectional sequence container `linked_list_t` composed of individually heap-allocated `list_node_t` objects with mutual reference cycles ($A \\leftrightarrow B$).
1. **Scope Boundaries**: Unrolled linked lists and lock-free lists are deferred to advanced concurrency milestones.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Node and list structures:
     ```c
     typedef struct {
       object_t *value;
       object_t *prev;
       object_t *next;
     } list_node_t;

     typedef struct {
       size_t count;
       object_t *head;
       object_t *tail;
     } linked_list_t;
     ```
1. **Core Systems Invariants**:
   - Immediate cycle invariant: Every adjacent node pair forms a reference cycle where `node->next->prev == node`.
   - Cycle termination invariant: Tracing must terminate despite bidirectional loops by checking `!child->is_marked` before pushing to the gray stack.
   - Decoupling invariant: Unlinking or popping a node must clear adjacent pointer references and decrement reference counts appropriately.
1. **Architectural Trade-offs**: Doubly linked lists permit $O(1)$ head/tail splicing without array shifts, but scatter node allocations across the heap, increasing pointer overhead and cache misses.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Dense cyclic graph reachability; bidirectional digraph traversal; pointer chasing vs cache line locality.
1. **Socratic Inquiries**:
   - Why are doubly linked lists the canonical leak source for pure reference counting without cycle collectors?
   - How does two-phase sweep prevent use-after-free when unrooted linked list nodes are ordered arbitrarily in the VM registry?
   - How does traversal latency compare between `list_t` (flat array) and `linked_list_t` (node graph)?
1. **Failure Modes & Pitfalls**: Infinite recursion on untracked back-pointers; dangling pointers after partial unlinking; stack overflow on deep linear chains.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Define `list_node_t` and `linked_list_t` in `src/object.h`.
   - Implement `new_list_node()` and `new_linked_list()` in `src/new.c` and `src/new.h`.
   - Implement `linked_list_push_back()`, `push_front()`, `pop_back()`, and `pop_front()` in `src/object.c`.
   - Integrate node types into `trace_blacken_object()` in `src/vm.c`.
   - Integrate decref and payload reclamation in `src/object.c`.
   - Write adversarial tests in `tests/test_linked_list.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/new.h`, `src/new.c`
   - `src/vm.c`
   - `tests/test_linked_list.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify push, pop, bidirectional traversal, arbitrary node splicing, and deep $1{,}000$-node chains.
1. **Zero-Leak Guarantee**: Detaching a $1{,}000$-node list reclaims all $1{,}001$ objects during `vm_collect_garbage()` with `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly.
