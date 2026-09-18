# Milestone: Fixed-Capacity Hash Table with Linear Probing

**ID:** `5895af9`\
**Status:** Planned\
**Difficulty:** 3 / 5\
**Focus:** Implement an associative key-value dictionary (`dict_t`) using open addressing with linear probing, key hash caching, equality verification, and bidirectional GC key/value marking on fixed-capacity tables.\
**Prerequisites:** [Object Hashing Protocol & Bitwise Hash Mixing](8782a4d_object_hashing_protocol.md), [Rich Comparisons & In-Place List Sorting](3f1132d_rich_comparisons_and_sorting.md), [Variadic Object Packing Constructors](5b538ed_variadic_tuple_and_list_pack.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Introduce `DICT` as an `object_kind_t` backed by an embedded `dict_t` payload in `object_data_t`.
   - Define dictionary entry structure (`object_t *key`, `object_t *value`, `uint64_t hash`, `bool occupied`).
   - Implement `new_dict(size_t capacity)` allocating a fixed-capacity power-of-two bucket array.
   - Implement `dict_set(object_t *dict, object_t *key, object_t *val)` inserting or updating entries using open addressing with linear probing.
   - Implement `dict_get(object_t *dict, object_t *key)` returning the associated value or `NULL` if not found.
   - Enforce bidirectional GC reachability: trace and mark both `entry->key` and `entry->value` during cycle detection and mark phases.
1. **Scope Boundaries**:
   - Tombstone-based deletion and dynamic table growth/rehashing are deferred to Milestone `e883213_dict_tombstone_deletion_and_rehashing.md`.
   - CPython-style split table compact dictionaries are deferred to advanced optimization milestones.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Fixed-capacity open addressing hash table:

     ```text
     dict_t
       capacity: 8 (power of two)
       count: 2
       entries: ------------------------\
                                        v
     [0] EMPTY: key=NULL, val=NULL, hash=0
     [1] OCCUPIED: key="name", val="Alice", hash=0x83FA...
     [2] EMPTY: key=NULL, val=NULL, hash=0
     [3] OCCUPIED: key="age", val=30, hash=0x19B2...
     [4..7] EMPTY
     ```

   - Bitwise index masking:
     $$\\text{slot} = \\text{hash} \\ & \\ (\\text{capacity} - 1)$$
1. **Core Systems Invariants**:
   - The Python equality invariant: Two keys are considered identical if `key1 == key2` (pointer identity) OR (`hash(key1) == hash(key2)` AND `object_equal(key1, key2)`).
   - Bidirectional GC marking: Dictionaries can participate in reference cycles (e.g., a dictionary holding a list that references the dictionary). Marking a dict must mark all occupied keys and values.
   - Ownership transfer: Setting a key/value increments reference counts (`refcount_inc(key)` and `refcount_inc(val)`). Replacing an existing key's value decrements the old value's reference count.
1. **Architectural Trade-offs**: Open addressing with linear probing has superior CPU cache locality compared to separate chaining (linked lists per bucket), but requires keeping load factors moderate ($\\le 0.70$) to prevent clustering.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Open addressing vs separate chaining; primary clustering in linear probing; bitwise fast modulo using power-of-two capacities; garbage collection traversal of associative structures.
1. **Socratic Inquiries**:
   - Why does `hash & (capacity - 1)` only work as a modulo replacement when `capacity` is an exact power of two ($2^K$)?
   - Why must you check both hash equality AND `object_equal(key, entry_key)` before declaring a key match?
   - How does reference cycle detection behave when a dictionary contains a self-referential cycle?
1. **Failure Modes & Pitfalls**: Infinite loops during probe search if the table becomes completely full without resizing; failing to decref old values on key overwrite; forgetting to trace keys in GC marking.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Define `dict_entry_t` and `dict_t` in `src/object.h`.
   - Add `DICT` to `object_kind_t` and `dict_t v_dict;` to `object_data_t` in `src/object.h`.
   - Implement `new_dict` in `src/new.c`.
   - Implement `dict_set` and `dict_get` in `src/object.c` using linear probe loop.
   - Update `object_decref_children` and `object_free_payload` in `src/object.c` to decref and free all entries.
   - Update `trace_blacken_object` in `src/vm.c` to mark both keys and values.
   - Add unit tests in `tests/test_dict.c`.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/new.h`, `src/new.c`
   - `src/vm.c`
   - `tests/test_dict.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify key insertion and retrieval across strings, integers, and tuples; test probe collision resolution when multiple keys hash to the same bucket; verify overwrite of existing keys; test cycle reclamation when dict references itself.
1. **Zero-Leak Guarantee**: Verify zero memory leaks via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Open Addressing with Linear Probing](https://en.wikipedia.org/wiki/Open_addressing): Hash table collision resolution using contiguous bucket probe sequences.
   - [Primary Clustering and Load Factors](https://en.wikipedia.org/wiki/Primary_clustering): Mathematical analysis of probe degradation as hash table occupancy increases.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython Dictionary Implementation History (R. Hettinger)](https://mail.python.org/pipermail/python-dev/2012-December/123028.html): The classic explanation of Python's open addressing hash tables and collision probe algorithms.
   - [CPython PyDictObject C-API Reference](https://docs.python.org/3/c-api/dict.html): Dictionary C-API semantics, key lookups, and participating in cyclic garbage collection.
