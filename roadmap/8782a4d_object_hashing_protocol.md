# Milestone: Object Hashing Protocol & Bitwise Hash Mixing

**ID:** `8782a4d`\
**Status:** Planned\
**Difficulty:** 2 / 5\
**Focus:** Implement the `object_hash(object_t *obj)` protocol using 64-bit non-cryptographic bit mixing (FNV-1a / Murmur), hash caching on immutable objects, and enforcing the fundamental Python equality-hash invariant.\
**Prerequisites:** [Object Header Bitflags & Memory Layout](c87f151_object_header_bitflags.md), [Heap-Allocated Variable-Length Tuple](f9c475f_heap_allocated_variable_length_tuple.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Implement `uint64_t object_hash(object_t *obj);` returning a 64-bit hash code.
   - Implement bitwise mixing algorithms for basic types: integer identity/folding, pointer address hashing for singletons, and FNV-1a byte streaming for strings.
   - Implement recursive composite hashing for immutable tuples with element bit rotation.
   - Cache computed hashes on strings and tuples to avoid redundant $O(N)$ hash traversals.
   - Enforce unhashable type errors (returning a dedicated error status or asserting) when attempting to hash mutable containers like `LIST`.
1. **Scope Boundaries**:
   - Hash table bucketing, open addressing, and dictionary lookups are deferred to Milestone `5895af9_hash_maps_and_dictionaries.md`.
   - Cryptographic hashing (SHA/MD5) is an explicit non-goal.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - FNV-1a 64-bit hashing mixing loop:

     ```text
     hash = 0xcbf29ce484222325ULL (FNV offset basis)
     For each byte b in string:
       hash = hash ^ b
       hash = hash * 0x100000001b3ULL (FNV prime)
     ```

   - Tuple composite hash recursion:

     ```text
     tuple_hash = INITIAL_SEED
     For each element e in tuple:
       e_hash = object_hash(e)
       tuple_hash = (tuple_hash ^ e_hash) * MULTIPLIER + ROTATE_LEFT(e_hash, 13)
     ```

1. **Core Systems Invariants**:
   - The Python Invariant: If `object_equal(a, b)` is true, then `object_hash(a) == object_hash(b)` must hold without exception.
   - Immutability invariant: Only immutable objects (Integers, Floats, Strings, Booleans, None, Tuples containing only hashable items) may be hashed. Mutable containers must reject hashing.
   - Stability invariant: The hash value of an immutable object must never change during its lifetime.
1. **Architectural Trade-offs**: Caching computed hashes adds 8 bytes to immutable sequence headers or uses memoization slots, but turns subsequent hash lookups in dictionaries and sets into instant $O(1)$ operations.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Hash distribution and avalanche effect; non-cryptographic hash functions (FNV-1a, Murmur3 bit mixers); unsigned 64-bit arithmetic overflow in C (well-defined modulo $2^{64}$); bit rotation vs bit shifting.
1. **Socratic Inquiries**:
   - Why does ISO C guarantee that unsigned integer overflow is well-defined (wrapping modulo $2^N$), whereas signed integer overflow is Undefined Behavior?
   - What is the "avalanche effect" in hash functions, and why is simple addition (`sum += byte`) insufficient for hash tables?
   - Why can Python lists not be used as dictionary keys, and how does `trash-gather` enforce this invariant at the C level?
1. **Failure Modes & Pitfalls**: Hashing uninitialized memory padding in structs; using signed types for bit mixing causing unintended sign extension; allowing a tuple containing a mutable list to be hashed.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Define hashing status codes and `uint64_t object_hash(object_t *obj);` in `src/object.h`.
   - Implement `static uint64_t hash_bytes(const void *data, size_t len)` using FNV-1a in `src/object.c`.
   - Implement `static uint64_t hash_combine(uint64_t seed, uint64_t hash)` bit rotation mixer.
   - Add cached `hash_val` field or memoized lookup to `tuple_t` and `STRING`.
   - Implement `object_hash` supporting all immutable types and returning error on `LIST`.
   - Add unit tests in `tests/test_hash.c` testing distribution, equality invariant, and mutable rejection.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `tests/test_hash.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify identical hashes for equal strings and identical tuples; verify distinct hashes for different strings; verify hashing a tuple with nested tuples works; verify attempting to hash a list fails safely.
1. **Zero-Leak Guarantee**: Verify zero memory leaks via `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Fowler–Noll–Vo (FNV-1a) Hash Algorithm](http://www.isthe.com/chongo/tech/comp/fnv/): Non-cryptographic bitwise mixing, prime multipliers, and 64-bit offset bases.
   - [Python Hash Contract and Equality Invariant](https://docs.python.org/3/reference/datamodel.html#object.__hash__): The formal rule that equal objects must have equal hash codes and mutable containers must reject hashing.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython Python/pyhash.c Implementation](https://github.com/python/cpython/blob/main/Python/pyhash.c): Production hashing algorithms used for strings, numbers, and composite tuples.
   - [MurmurHash3 and Hash Avalanche Properties](https://en.wikipedia.org/wiki/MurmurHash): Bit rotation and mixing techniques to achieve uniform distribution across hash table buckets.
