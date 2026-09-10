# Milestone: Hash Maps & Dictionaries (`dict_t`)

**ID:** `5895af9`\
**Status:** Planned\
**Focus:** Implement associative key-value dictionaries with open addressing, collision resolution, tombstone markers, and bidirectional GC traversal.\
**Prerequisites:** [Full CPython-Style Offset-0 Hierarchy](222f6ce_cpython_offset0_hierarchy.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**: Implement a mutable associative container `dict_t` mapping heap-allocated keys to heap-allocated values using open addressing, equality checking, and hash code caching.
1. **Scope Boundaries**: Concurrent or thread-safe dictionaries are deferred to multi-threading milestones.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Contiguous entry table:
     ```c
     typedef struct {
       uint64_t hash;
       object_t *key;
       object_t *value;
     } dict_entry_t;

     typedef struct {
       size_t count;
       size_t capacity;
       size_t num_tombstones;
       dict_entry_t *entries;
     } dict_t;
     ```
1. **Core Systems Invariants**:
   - Bidirectional reference invariant: A dictionary holds strong reference edges to both its keys (`entry->key`) and values (`entry->value`). The GC mark phase must blacken both for every active entry.
   - Tombstone & deletion invariant: Deleting an entry marks the slot as a tombstone (`key == TOMBSTONE`) and decrements references to the former key and value immediately.
   - Load factor invariant: Exceeding the load factor threshold (e.g. 66%) triggers reallocation, rebuilding probe sequences and purging tombstones.
1. **Architectural Trade-offs**: Open addressing delivers high cache locality compared to separate chaining linked lists, but requires tombstone management and rehashing on growth.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Open addressing collision resolution; quadratic probing / double hashing; load factors and amortization.
1. **Socratic Inquiries**:
   - Why does open addressing offer superior cache locality over separate chaining?
   - What happens if a mutable container (such as a list) is used as a key and then modified?
   - How does tri-color marking handle cycles formed when a dictionary references itself (`d["self"] = d`)?
1. **Failure Modes & Pitfalls**: Infinite probe loops on full tables; dangling references during deletions; hash collision clustering.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Define `dict_entry_t` and `dict_t` in `src/object.h`.
   - Implement hash functions (e.g. FNV-1a or SipHash for strings) in `src/object.c`.
   - Implement `new_dict()` in `src/new.c` and `src/new.h`.
   - Implement `dict_set()`, `dict_get()`, and `dict_delete()` in `src/object.c`.
   - Update `trace_blacken_object()` in `src/vm.c` to trace both `key` and `value`.
   - Update `object_decref_children()` and `object_free_payload()` in `src/object.c`.
   - Write unit tests in `tests/test_dict.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/new.h`, `src/new.c`
   - `src/vm.c`
   - `tests/test_dict.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify insertion, retrieval, overwriting, deletion, collision resolution, and resizing with $10{,}000$ unique keys.
1. **Zero-Leak Guarantee**: Cycle test `d["self"] = d` is collected and freed cleanly by `vm_collect_garbage()` with `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly.
