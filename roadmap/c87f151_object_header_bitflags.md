# Milestone: Object Header Bitflags & Memory Layout

**ID:** `c87f151`\
**Status:** Planned\
**Difficulty:** 2 / 5\
**Focus:** Replace separate boolean fields in `object_t` with a packed bitflags field (`uint16_t flags`), mastering bitwise operations (`&`, `|`, `^`, `~`, `<<`), bitmasks, struct alignment boundaries, and padding reduction.\
**Prerequisites:** [Boolean Immortal Singletons & Truthiness](6c3a989_bool_singletons_and_truthiness.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Define bitmask constants for object header flags (`OBJ_FLAG_MARKED`, `OBJ_FLAG_IMMORTAL`, `OBJ_FLAG_TRACKED`).
   - Replace `bool is_marked` and related ad-hoc flags in `struct Object` with `uint16_t flags;`.
   - Implement bitwise inline helpers: `object_has_flag()`, `object_set_flag()`, `object_clear_flag()`, and `object_toggle_flag()`.
   - Profile `sizeof(struct Object)` before and after the change to measure memory padding savings across 64-bit architectures.
1. **Scope Boundaries**:
   - Tagged pointers (storing data in unused pointer address bits) are deferred to advanced optimization milestones.
   - Slab allocator integration is handled in Milestone `e10d642_object_slab_allocator.md`.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Bitfield layout inside `uint16_t flags`:

     ```text
     Bit:   15 ... 3      2            1            0
          +----------+----------+------------+------------+
          | Reserved | TRACKED  |  IMMORTAL  |   MARKED   |
          +----------+----------+------------+------------+
          Mask:        (1 << 2)    (1 << 1)     (1 << 0)
                         0x04        0x02         0x01
     ```

   - Struct padding comparison:

     ```text
     Before: [bool 1B][pad 7B][refcount 8B][tracker_id 8B][kind 4B][pad 4B][data 16B] = 48 Bytes
     After:  [flags 2B][kind 2B][pad 4B][refcount 8B][tracker_id 8B][data 16B]         = 40 Bytes
     ```

1. **Core Systems Invariants**:
   - Bitwise idempotency: Setting an already-set flag (`flags |= MASK`) or clearing an already-cleared flag (`flags &= ~MASK`) must produce stable results without corrupting adjacent flag bits.
   - Immortal protection: The `OBJ_FLAG_IMMORTAL` bit must never be cleared once an object is initialized as a singleton.
   - Mark invariant: During GC mark-and-sweep, only the `OBJ_FLAG_MARKED` bit may be toggled; all other flags must remain invariant.
1. **Architectural Trade-offs**: Using bitmasks requires bitwise CPU operations instead of direct boolean loads/stores, but reduces `struct Object` memory footprint and cache line pressure by up to 16% per object.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Bitwise arithmetic in C (`AND`, `OR`, `XOR`, `NOT`, left/right shifts); hardware struct alignment (natural alignment to 4 or 8 byte boundaries); structure padding bytes inserted by compilers; C11 `_Static_assert` and `offsetof` macro.
1. **Socratic Inquiries**:
   - Why does a `struct { bool a; uint64_t b; bool c; }` take 24 bytes in memory instead of 10 bytes on a 64-bit machine?
   - How does `flags &= ~MASK` work step-by-step in two's complement binary?
   - Why do production runtimes like CPython (`ob_flags`), Linux kernel (`page->flags`), and V8 rely heavily on header bitflags rather than `bool` members?
1. **Failure Modes & Pitfalls**: Forgetting parentheses around bitwise operations due to operator precedence (e.g., `flags & MASK == 0` evaluates `==` before `&`); integer promotion of `uint16_t` causing signed comparison bugs; accidentally modifying adjacent bits with incorrect bitshift masks.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Define flag bitmasks (`OBJ_FLAG_MARKED = 1U << 0`, etc.) in `src/object.h`.
   - Implement inline bit manipulation functions (`object_set_flag`, `object_clear_flag`, `object_has_flag`) in `src/object.h`.
   - Update `struct Object` in `src/object.h` to replace `bool is_marked` with `uint16_t flags`.
   - Add `_Static_assert(sizeof(struct Object) <= 40, "Object struct padding check");` in `src/object.c`.
   - Update `mark()`, `trace()`, `sweep()`, and immortality checks across `src/vm.c` and `src/object.c` to use the new bitwise helpers.
   - Add comprehensive unit tests in `tests/test_object.c` verifying flag operations and edge cases.
1. **File Touchpoints**:
   - `src/object.h`, `src/object.c`
   - `src/vm.c`
   - `tests/test_object.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Test independent setting, clearing, and querying of every individual flag bit; ensure setting one flag never mutates or resets other bits; verify GC mark and sweep behavior remains 100% correct.
1. **Zero-Leak Guarantee**: Verify zero memory leaks under `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [Bitwise Operators in ISO C](https://en.cppreference.com/w/c/language/operator_arithmetic#Bitwise_arithmetic_operators): Bit masks, bitwise shifts, inversion, and bitwise boolean logic.
   - [Structure Alignment, Padding, and \_Static_assert](https://en.cppreference.com/w/c/language/object#Alignment): Hardware alignment requirements, compiler padding rules, and compile-time size assertions.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [CPython Include/object.h ob_flags Architecture](https://github.com/python/cpython/blob/main/Include/object.h): How CPython packs GC tracking bits, immortality, and type flags into single-word headers.
   - [Linux Kernel Page Flags Design](https://www.kernel.org/doc/gorman/html/understand/understand005.html): Production bitflag packing representing page lifecycle states in high-performance kernel code.
