# Milestone: Dictionary Tombstone Deletion & Dynamic Rehashing

**ID:** `e883213`\
**Status:** Planned\
**Difficulty:** 4 / 5\
**Focus:** Extend the dictionary with tombstone markers for safe key deletion (`dict_del`) without breaking probe sequences, and implement dynamic table growth and full rehashing when load factor $\\alpha > 2/3$.\
**Prerequisites:** [Fixed-Capacity Hash Table with Linear Probing](5895af9_hash_maps_and_dictionaries.md)

______________________________________________________________________

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Introduce entry states in `dict_entry_t`: `ENTRY_EMPTY`, `ENTRY_OCCUPIED`, and `ENTRY_TOMBSTONE` (dummy marker).
   - Implement `dict_del(object_t *dict, object_t *key)`: locate the key, decref key and value, and convert the slot to a tombstone without breaking downstream probe chains.
   - Update `dict_get` and `dict_set` to respect tombstones: lookups continue probing past tombstones, while insertions recycle the first encountered tombstone if the key is not found elsewhere.
   - Implement dynamic capacity growth: when active entries plus tombstones exceed the threshold ($\\text{count} \\times 3 \\ge \\text{capacity} \\times 2$), allocate a new table of double capacity ($2 \\times \\text{capacity}$) and rehash all live entries, purging tombstones.
1. **Scope Boundaries**:
   - Table shrinking (reducing capacity on massive deletions) is deferred to advanced dictionary optimizations.
   - Ordered dictionaries maintaining insertion order (PEP 468) are deferred to Python collection parity milestones.

______________________________________________________________________

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Tombstone probe sequence preservation:
     ```
     Before Deletion:
       Slot 1 [Hash A -> "foo"] -> Slot 2 [Collided Hash A -> "bar"] -> Slot 3 [EMPTY]
     Naive Deletion (Broken!):
       Slot 1 [EMPTY]           -> Slot 2 [Collided Hash A -> "bar"] (Looking up "bar" terminates at Slot 1!)
     Tombstone Deletion (Correct):
       Slot 1 [TOMBSTONE]       -> Slot 2 [Collided Hash A -> "bar"] (Lookup skips Slot 1, finds "bar"!)
     ```
   - Dynamic table expansion:
     ```
     Old Table (Cap: 8, Load > 66%):
       [0: Empty] [1: "foo"] [2: Tombstone] [3: "bar"] [4: "baz"] [5: "qux"] ...
                               |
                               v (Rehash live entries only)
     New Table (Cap: 16):
       [0: Empty] [3: "foo"] [7: "bar"] [11: "baz"] [14: "qux"] ... (Zero tombstones!)
     ```
1. **Core Systems Invariants**:
   - Probe chain continuity: An empty slot (`ENTRY_EMPTY`) terminates a probe sequence, but a tombstone (`ENTRY_TOMBSTONE`) must never terminate a lookup probe sequence.
   - Load factor invariant: The table must resize before the load factor reaches $100%$ ($\\alpha < 0.67$), guaranteeing that an empty slot is always reached to terminate failed lookups in finite steps.
   - Rollback safety on resize failure: If allocating the expanded table buffer fails, the existing table and its contents must remain completely intact and uncorrupted.
1. **Architectural Trade-offs**: Tombstones solve the probe-chain interruption problem, but accumulate over time, degrading search performance until cleaned up by a rehashing pass.

______________________________________________________________________

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Open addressing deletion strategies (tombstones vs backward-shift deletion); load factor thresholds ($\\alpha$); rehashing complexity (amortized $O(1)$ insertions); memory reallocation rollbacks.
1. **Socratic Inquiries**:
   - Why would simply setting an entry's key to `NULL` break subsequent lookups for keys that suffered hash collisions during insertion?
   - During insertion, why can you record the index of the first tombstone slot and reuse it *only after* verifying that the key does not already exist later in the probe sequence?
   - Why do tombstones disappear entirely during dynamic rehashing?
1. **Failure Modes & Pitfalls**: Inserting duplicate keys by reusing a tombstone slot before finishing the probe check for existing key; infinite loop on lookup if all slots become tombstones; memory leaks during failed rehash allocation.

______________________________________________________________________

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Update `dict_entry_t` in `src/object.h` to replace `bool occupied` with `uint8_t state` (`ENTRY_EMPTY`, `ENTRY_OCCUPIED`, `ENTRY_TOMBSTONE`).
   - Implement `dict_del(object_t *dict, object_t *key)` in `src/object.c`.
   - Update `dict_get` and `dict_set` in `src/object.c` to navigate tombstones.
   - Implement dynamic resizing helper `static bool dict_resize(object_t *dict, size_t new_capacity)` in `src/object.c`.
   - Hook capacity check into `dict_set` to trigger `dict_resize` when $\\text{count} \\times 3 \\ge \\text{capacity} \\times 2$.
   - Add unit tests in `tests/test_dict.c` testing deletion, collision chains across tombstones, and dynamic expansion to 1,000+ keys.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `tests/test_dict.c`

______________________________________________________________________

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Insert keys with identical hashes, delete the first, verify the second remains findable; verify reusing tombstone slots on new insertions; test continuous insertion causing multiple table resizes (from 8 to 1,024 slots); verify non-existent key deletion returns false.
1. **Zero-Leak Guarantee**: Verify zero memory leaks during heavy insert/delete churn via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______________________________________________________________________

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Deletion in Open Addressing: Tombstones vs Backward Shifts](https://en.wikipedia.org/wiki/Open_addressing#Deletion): Why setting deleted buckets to empty breaks probe chains and how tombstone sentinels resolve it.
   - [Amortized Dynamic Table Resizing and Load Factors](https://en.wikipedia.org/wiki/Dynamic_array#Geometric_resizing): Triggering table growth and full rehashing before load factor compromises search speed.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython Objects/dictobject.c Dummy Marker Recycling](https://github.com/python/cpython/blob/main/Objects/dictobject.c): How CPython recycles dummy entries and maintains search performance across deletion churn.
   - [PEP 468 – Preserving Keyword Argument Order](https://peps.python.org/pep-0468/): The historical evolution of Python dictionaries toward compact, ordered hash tables.
