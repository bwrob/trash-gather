# Milestone: Binary Heap Graph Serialization

**ID:** `402c62c`\
**Status:** Planned\
**Difficulty:** 3 / 5\
**Focus:** Serialize and deserialize heap object graphs to and from binary files using standard C streams (`FILE*`, `fwrite`, `fread`), mastering binary file format layouts, magic headers, endianness awareness, and stream error handling.\
**Prerequisites:** [Dynamic String Builder & Safe Formatting](4911b8b_dynamic_string_builder.md), [Cycle-Safe String Representation & Object Printing](bf0a981_cycle_safe_string_repr.md)

______

## 1. Objective & Technical Scope

1. **Primary Goals**:
   - Design a simple binary file format ("TG_DUMP") with a 4-byte magic header (`0x54474743` - "TGGC"), format version, object count, and encoded records.
   - Implement `bool vm_dump_to_file(const object_t *root, const char *filepath);` writing object graphs to disk via `fwrite()`.
   - Implement `object_t *vm_load_from_file(const char *filepath);` reconstructing object graphs using `fread()` and integrating newly created objects into VM tracking.
   - Handle acyclic graphs and simple shared references safely during serialization.
1. **Scope Boundaries**:
   - Complex cyclic graph preservation across files is deferred to advanced serializer extensions.
   - Cross-architecture big-endian/little-endian bit swapping is an optional extension.

______

## 2. Architectural Design & Invariants

1. **Memory Layout & Pointer Graph**:
   - Binary serialization stream layout:

     ```text
     +-----------------------------------------------------------------+
     | HEADER: Magic "TGGC" (4B) | Version (2B) | Root Kind (2B)       |
     +-----------------------------------------------------------------+
     | RECORD: Tag (1B) | Length (4B) | Payload Data (N Bytes)         |
     +-----------------------------------------------------------------+
     | For Containers (TUPLE/LIST):                                    |
     |   Element Count (4B) | Nested Record 1 | Nested Record 2 ...    |
     +-----------------------------------------------------------------+
     ```

1. **Core Systems Invariants**:
   - File handle safety: Every `fopen()` must be strictly closed with `fclose()` on all success and failure paths, preventing file descriptor leaks.
   - Header validation: `vm_load_from_file` must reject any file missing the magic header or bearing an unsupported version number before reading payload bytes.
   - Stream error detection: Check both `ferror(file)` and return values of `fread`/`fwrite` to prevent partial reads or writing to full disks.
   - Allocation rollback on corruption: If an error or unexpected EOF occurs during deserialization, all partially reconstructed objects must be decremented and freed before returning `NULL`.
1. **Architectural Trade-offs**: Binary serialization is significantly faster and more compact than text formats (JSON/XML), but requires strict schema versioning and careful defensive bounds checks against corrupted files.

______

## 3. Systems Concepts & Guiding Questions

1. **Underlying Theory**: Standard C buffered I/O streams (`<stdio.h>`); binary vs text file modes on different operating systems (`"wb"` vs `"rb"`); serialization formats (Python pickle, marshal); defensive deserialization against malformed input (AFL-style fuzzing vectors).
1. **Socratic Inquiries**:
   - Why must you use mode `"wb"` and `"rb"` rather than `"w"` and `"r"` when working with binary data in C?
   - How can an attacker craft a malicious serialized file with an artificially huge length field (e.g. `count = 0xFFFFFFFF`) to cause an integer overflow or out-of-memory crash in `malloc`?
   - Why must you check both the return value of `fread()` and `feof()`/`ferror()`?
1. **Failure Modes & Pitfalls**: Leaking open `FILE*` handles on error return branches; trusting size headers without bounds checks causing huge allocations; unaligned multi-byte reads causing crashes on strict-alignment architectures.

______

## 4. Implementation Steps & Touchpoints

1. **Step-by-Step Execution Sequence**:
   - Declare serialization functions in `src/serialize.h`:
     - `bool vm_dump_to_file(const object_t *root, const char *filepath);`
     - `object_t *vm_load_from_file(const char *filepath);`
   - Create `src/serialize.c` implementing binary format serialization for Scalars (Int, Float, String, Bool, None) and Containers (Tuple, List).
   - Implement defensive header verification (`MAGIC == 0x54474743`) and EOF detection.
   - Implement rollback deallocation helper in case of corrupted file payloads.
   - Add unit tests in `tests/test_serialize.c` verifying dump, reload, equality comparison, and corrupted file rejection.
1. **File Touchpoints**:
   - `src/serialize.h`, `src/serialize.c`
   - `tests/test_serialize.c`

______

## 5. Verification & Acceptance Criteria

1. **Unit & Adversarial Tests**: Verify dump and reload of primitive objects, deep nested tuples/lists, and strings; test loading truncated/corrupted files and verify safe rejection without crash; test non-existent file paths.
1. **Zero-Leak Guarantee**: Verify zero file descriptor leaks and zero memory leaks under `assert(boot_all_freed())`.
1. **Tooling Quality Gates**: `just test`, `just lint`, and `just check` pass cleanly with zero compiler warnings.
1. **Milestone Completion & Lesson Extraction**: Upon green tests and zero leaks, update status to `Completed` in this writeup and `✅ Completed` in `roadmap/README.md`, update Mermaid node styling to `:::completed`, and generate the educational lesson in `lessons/`.

______

## 6. Recommended Reading & External References

1. **Before Implementation (Conceptual Foundations)**:
   - [ISO C Binary Stream I/O Operations](https://en.cppreference.com/w/c/io): Standard rules for `fopen`, `fread`, `fwrite`, `ferror`, and managing binary streams.
   - [Designing Binary File Formats (Magic Numbers and Headers)](<https://en.wikipedia.org/wiki/Magic_number_(programming)>): Structuring robust binary files with format identifiers, schemas, and versioning.
1. **After Implementation (Deep Dives & Systems Context)**:
   - [Python marshal and pickle Binary Protocol Internals](https://docs.python.org/3/library/marshal.html): How Python serializes internal object graphs, bytecode, and constants to disk.
   - [Cap'n Proto and FlatBuffers Serialization Architecture](https://capnproto.org/): Modern zero-copy binary serialization principles and defense against malformed inputs.
