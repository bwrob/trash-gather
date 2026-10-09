# Bibliography & Reference Sources Index (`resources/INDEX.md`)

This index catalogs reference volumes, technical manuals, and standard specifications available in `resources/`. Each entry provides a high-level focus summary, complete Table of Contents, and concise pedagogical descriptions of each chapter and section to guide pre-flight milestone reading.

______

## 📚 Quick Reference & Catalog

| Author & Title | Format & Size | Core Pedagogical Focus | Key Relevancy to trash-gather |
| :--- | :--- | :--- | :--- |
| **Kenneth A. Reek**<br>*Pointers on C* | PDF, 3.98 MB | Deep mechanics of pointers, memory organization, calling conventions, stack frames, and ADTs. | **Variadics (`<stdarg.h>`)**, Stack frame layout, pointer arithmetic, linked data structures. |
| **Robert C. Seacord**<br>*Effective C (2020)* | PDF, 5.65 MB | Professional C17 development, SEI CERT C coding standards, memory safety, dynamic allocators. | **Flexible Array Members**, Heap memory states, alignment, integer overflow, AddressSanitizer. |
| **Jens Gustedt**<br>*Modern C (3rd Edition)* | EPUB, 6.11 MB | Modern ISO C (C17/C23) abstract state machine, formal memory model, type-generic programming, atomics. | Formal memory model, variable-length argument lists, object lifecycles, undefined behavior prevention. |

______

## 1. Pointers on C

- **Author**: Kenneth A. Reek
- **Publisher / Edition**: Addison-Wesley (1998)
- **File**: `resources/reek_1998_pointers_on_c.pdf` (624 pages)
- **Pedagogical Scope**: The quintessential systems-level text on pointer mechanics and runtime memory in C. It demystifies how the C abstract machine maps onto physical silicon, hardware stack frames, and heap allocators.

### Chapter Guides & Descriptive Table of Contents

#### Chapter 1: A Quick Start (pp. 2–25)

- **1.1–1.8 Core Introduction**: Dissects a complete C program reading lines and rearranging characters, highlighting array indexing, string termination (`\0`), standard I/O streams, and compiler preprocessing.

#### Chapter 2: Basic Concepts (pp. 26–39)

- **2.1 Environment**: Separation of translation environment (preprocessor, compiler, assembler, linker) from execution environment.
- **2.2 Lexical Rules**: Identifiers, keywords, operators, and comment syntax.

#### Chapter 3: Data (pp. 40–71)

- **3.1 Basic Types**: Integer families (signed vs unsigned), floating-point representations, and character literals.
- **3.2 Basic Declarations**: Type specifiers, initializers, and typedef abstractions.
- **3.3 Properties of Variables**: Scope (block, file, prototype), linkage (external, internal, none), and storage duration (automatic, static).
- **3.4 Constants**: Literal constants, `#define` constants, `const` keyword nuances, and enumeration constants.

#### Chapter 4: Statements (pp. 72–93)

- **4.1–4.4 Control Constructs**: Expression statements, block statements, conditional branches (`if`/`else`, `switch`), and loops (`while`, `for`, `do-while`).

#### Chapter 5: Operators and Expressions (pp. 94–129)

- **5.1–5.4 Operators**: Arithmetic, shift, bitwise logic, relational, equality, logical, assignment, and conditional operators.
- **5.5 Order of Evaluation & Sequence Points**: Precedence, associativity, unsequenced side-effects, and undefined expression evaluation.

#### Chapter 6: Pointers (pp. 130–165)

- **6.1–6.4 Memory and Addresses**: How addresses represent physical byte offsets in memory.
- **6.5–6.8 Pointer Values, Indirection, and Variables**: Declaring pointer types, dereference operator (`*`), and address-of operator (`&`).
- **6.9–6.14 Pointer Arithmetic & Expressions**: Scaling pointer arithmetic by `sizeof(T)`, comparing pointers within array bounds, NULL pointer constants, and pointer safety rules.

#### Chapter 7: Functions (pp. 166–197)

- **7.1–7.3 Function Definition & Arguments**: Call-by-value semantics, simulating call-by-reference via pointers.
- **7.4 ADTs and Black Boxes**: Data hiding through static file-scope variables.
- **7.5 Recursion**: Stack call chains and termination conditions.
- **7.6 Variable Argument Lists (pp. 190–196)**: **Directly relevant to `new_tuple_pack` and `new_list_pack`.** In-depth coverage of `<stdarg.h>`, `va_list`, `va_start`, `va_arg`, and `va_end`. Explains why the callee cannot determine argument count or types without explicit external parameters, and how types promote across variadic call boundaries.

#### Chapter 8: Arrays (pp. 198–243)

- **8.1–8.3 Array Mechanics**: Array identifier decay to pointer to first element, pointer arithmetic indexing equivalency (`a[i] == *(a + i)`).
- **8.4 Multidimensional Arrays & Matrices**: Row-major memory layout, contiguous flattening, and array parameter passing.

#### Chapter 9: Strings, Characters, and Bytes (pp. 244–269)

- **9.1–9.3 String Operations**: String lengths (`strlen`), bounded vs unbounded copies (`strcpy`, `strncpy`), concatenation (`strcat`), and comparisons (`strcmp`).
- **9.4 Memory Byte Operations**: Raw byte manipulations (`memcpy`, `memmove`, `memset`, `memcmp`) across heap buffers. Crucial distinction between `memcpy` and overlapping `memmove`.

#### Chapter 10: Structures and Unions (pp. 270–303)

- **10.1–10.2 Structures & Members**: Struct declarations, member access (`.` and `->`), and nested structures.
- **10.3 Structure Storage Allocation**: Hardware alignment boundaries, internal padding holes, and `sizeof` struct calculations. Crucial for understanding `object_t` and `tuple_t` memory layout.
- **10.6 Unions (pp. 291–297)**: Memory overlap in unions, variant records, and tagged unions. Directly models `trash-gather`'s `object_data_t`.

#### Chapter 11: Dynamic Memory Allocation (pp. 304–321)

- **11.1–11.3 Malloc, Calloc, Realloc, Free**: Heap allocation mechanics, zero-initialization via `calloc`, and dynamic buffer resizing via `realloc`.
- **11.4–11.5 Common Dynamic Memory Errors**: Memory leaks, accessing freed memory (use-after-free), double frees, and freeing non-heap pointers.

#### Chapter 12: Using Structures and Pointers (pp. 322–350)

- **12.1–12.3 Singly & Doubly Linked Lists**: Dynamic node linking, insertion invariants, deletion invariants, and sentinel nodes.

#### Chapter 13: Advanced Pointers Topics (pp. 351–384)

- **13.1 Pointers to Pointers**: Indirection levels, modifying caller pointers inside helper functions.
- **13.3 Function Pointers**: Callback architectures, jump tables, and polymorphic dispatch. Foundation for virtual method tables (vtables) in VMs.

#### Chapter 14: The Preprocessor (pp. 385–409)

- **14.1–14.4 Macros & Inclusion**: `#define` macro expansion, side effects in macro arguments, header guards, and conditional compilation (`#ifdef`).

#### Chapter 15: Input/Output Functions (pp. 410–453)

- **15.1–15.16 Standard I/O**: Streams, buffering modes, formatted I/O (`printf`, `scanf`), and binary I/O (`fread`, `fwrite`).

#### Chapter 16: Standard Library (pp. 454–493)

- **16.6 Printing Variable Argument Lists (pp. 475)**: `vprintf`, `vsnprintf`, and passing `va_list` across multiple function layers.
- **16.7–16.8 Assertions & Sorting**: `assert()` macro diagnostics and `qsort` polymorphic sorting with function pointer comparators.

#### Chapter 17: Classic Abstract Data Types (pp. 494–537)

- **17.1–17.4 Stacks, Queues, Trees**: Implementing ADTs with dynamic arrays versus linked representations. Directly informs our VM value stack and heap traversal trees.

#### Chapter 18: Runtime Environment (pp. 538–561)

- **18.1.3 The Stack Frame**: Activation records, stack pointer (`%esp`/`%rsp`), and base/frame pointer (`%ebp`/`%rbp`).
- **18.1.6 Determining Stack Frame Layout (pp. 546–555)**: **Essential hardware mental model.** How arguments are pushed, function prologues allocate local stack space, argument order on stack, function epilogues restore caller state, and return values are passed via CPU registers.

______

## 2. Effective C: An Introduction to Professional C Programming

- **Author**: Robert C. Seacord
- **Publisher / Edition**: No Starch Press (2020)
- **File**: `resources/seacord_2020_effective_c.pdf` (274 pages)
- **Pedagogical Scope**: An authoritative, modern guide to ISO C17 systems programming written by the former chair of the SEI CERT C coding standard committee. Emphasizes undefined behavior elimination, dynamic memory safety, object models, and robust verification.

### Chapter Guides & Descriptive Table of Contents

#### Chapter 1: Getting Started with C (pp. 31–42)

- **Developing Your First Program**: Compiling with modern toolchains (GCC, Clang), preprocessor stages, checking function return values.
- **Portability Categories**: Crystal-clear definitions of Implementation-Defined Behavior, Unspecified Behavior, Undefined Behavior (UB), and Common Extensions.

#### Chapter 2: Objects, Functions, and Types (pp. 43–64)

- **Objects & Pointers**: Conceptual model of an object as a region of data storage whose contents can represent values.
- **Storage Duration & Scope**: Automatic, static, thread-local, and allocated storage durations.
- **Alignment (pp. 50–51)**: Fundamental CPU alignment constraints, hardware bus cycles, natural alignment, and `_Alignof` / `alignof`.
- **Derived Types & Tags**: Arrays, function types, struct declarations, member alignment holes, union memory overlap, and type tags for variant records.
- **Type Qualifiers**: Precise semantic definitions of `const`, `volatile`, and `restrict`.

#### Chapter 3: Arithmetic Types (pp. 65–86)

- **Integers**: Two's complement representation, padding bits, range boundaries via `<limits.h>`, signed vs unsigned integer wrap-around rules.
- **Arithmetic Conversions**: Integer promotion rules, integer conversion rank, and avoiding sign-extension bugs during implicit casts.

#### Chapter 4: Expressions and Operators (pp. 87–110)

- **Order of Evaluation & Sequence Points**: Unsequenced side-effects, indeterminate sequencing, and avoiding UB in complex expressions.
- **sizeof and Alignof Operators**: Compile-time byte sizing, operand evaluation rules, and flexible array sizing calculations.
- **Pointer Arithmetic**: Valid pointer offset calculations within allocated object bounds.

#### Chapter 5: Control Flow (pp. 111–128)

- **Branching & Loops**: Structured control flow, selection statements, switch fallthrough mechanics, and loop invariants.
- **Jump Statements**: Safe usage of `return`, `break`, `continue`, and disciplined `goto` for multi-stage allocation error rollbacks.

#### Chapter 6: Dynamically Allocated Memory (pp. 129–148)

- **The Heap & Memory Managers**: Dynamic storage duration, heap fragmentation, and allocator mechanics.
- **Standard Allocator Functions**: Precise contracts of `malloc`, `aligned_alloc`, `calloc`, `realloc`, `reallocarray`, and `free`.
- **Memory States (pp. 139–140)**: Formal lifecycle states of allocated storage: unallocated, allocated-uninitialized, allocated-initialized, and deallocated/indeterminate.
- **Flexible Array Members (pp. 140–141)**: **Directly models `trash-gather`'s `tuple_t`.** How to correctly declare `type elements[]` as the trailing member of a struct, calculate `malloc(sizeof(struct) + n * sizeof(elem))`, and ensure single-allocation cache locality.
- **Storage Pitfalls**: Diagnosing use-after-free, double free, heap buffer overflows, and pointer leaks.

#### Chapter 7: Characters and Strings (pp. 149–176)

- **String Fundamentals**: Null-terminated byte strings (NTBS), character encodings (ASCII, UTF-8), and string literal storage in read-only data segments (`.rodata`).
- **String Functions & Annex K**: Safe string manipulation, bounds-checking interfaces, and memory-safe string copying.

#### Chapter 8: Input/Output (pp. 177–198)

- **Streams & Files**: Text vs binary streams, stream buffering (unbuffered, line buffered, fully buffered), file descriptors, and flushing.
- **Binary I/O**: Direct memory page reading and writing via `fread`/`fwrite`, byte-serialization principles.

#### Chapter 9: Preprocessor (pp. 199–214)

- **Preprocessing Pipeline**: Tokenization, macro expansion, stringification (`#`), token pasting (`##`), type-generic macros (`_Generic`), and robust include guards.

#### Chapter 10: Program Structure (pp. 215–228)

- **Componentization**: Encapsulation, opaque types (incomplete struct pointers), minimizing coupling, maximizing cohesion. Foundation for `trash-gather`'s modular architecture (`vm.h`, `object.h`, `gc.h`).
- **Linkage & Translation Units**: External vs internal linkage (`static`), translation unit boundaries, and symbol resolution.

#### Chapter 11: Debugging, Testing, and Analysis (pp. 229–252)

- **Assertions**: `_Static_assert` for compile-time layout verification, runtime `assert()` for contract invariants.
- **Compiler Sanitizers**: Modern instrumentation using **AddressSanitizer (ASan)**, UndefinedBehaviorSanitizer (UBSan), and compiler warning flags (`-Wall -Wextra -Wpedantic`).

______

## 3. Modern C (Third Edition, covers C23/C17)

- **Author**: Jens Gustedt
- **Publisher / Edition**: Manning Publications (3rd Edition, 2024)
- **File**: `resources/gustedt_2023_modern_c.epub` (Level 0 through Level 3)
- **Pedagogical Scope**: A rigorous, modern conceptual treatment of C that trains the programmer to think in terms of the C abstract state machine, strict typing, modern language idioms, and low-level execution invariants.

### Level Guides & Descriptive Table of Contents

#### Level 0: Encounter (Chapters 1–2)

- **Chapter 1: Getting Started**: Basic environment, imperative execution model, compilation steps.
- **Chapter 2: The Principal Structure of a Program**: Grammar, declarations vs definitions, statements, function call stack frames.

#### Level 1: Acquaintance and Buckle up (Chapters 3–8)

- **Chapter 3: Everything is about Control**: Conditional flow, loop iterations, switch-case dispatch.
- **Chapter 4: Expressing Computations**: Operands, operators, arithmetic conversions, boolean contexts, evaluation ordering.
- **Chapter 5: Basic Values and Data**: The abstract state machine, binary representations, basic types, initializers, and named constants.
- **Chapter 6: Derived Data Types**: Arrays, pointers as opaque types, structures, member coalescing, and type aliases.
- **Chapter 7: Functions**: Function interfaces, call chains, recursion, stack frame instantiation.
- **Chapter 8: C Library Functions**: Arithmetic, numerics, string processing, runtime assertions, and process termination.

#### Level 2: Cognition (Chapters 9–15)

- **Chapter 9–10: Style, Organization and Documentation**: Clean C naming, API documentation, modular translation unit design.
- **Chapter 11: Pointers**: Pointer validity, arithmetic, indirection, pointers to structures, function pointers, and null pointer constants.
- **Chapter 12: The C Memory Model**:
  - **12.1 A Uniform Memory Model**: How the abstract state machine views memory as contiguous arrays of bytes.
  - **12.2 Unions**: Type punning vs variant representations.
  - **12.3–12.4 Memory, State & Unspecific Pointers**: Void pointers (`void*`), alignment, and effective types.
- **Chapter 13: Storage**:
  - **13.1 Malloc and Friends**: Dynamic allocation consistency, allocation sizes, failure handling.
  - **13.2 Storage Duration, Lifetime, and Visibility**: Automatic, static, allocated, and thread lifetimes.
  - **13.5 Digression: A Machine Model**: Physical registers, caches, RAM, MMU, and translation lookaside buffers (TLB).
- **Chapter 14: More Involved Processing and I/O**: Formatted text streams, binary serialization, UTF-8 processing.
- **Chapter 15: Program Failure**: Exceptional states, degraded invariants, defensive error checking, and resource cleanup strategies.

#### Level 3: Experience (Chapters 16–21)

- **Chapter 16: Performance**: Inline functions, `restrict` pointer qualifiers, cache locality, and runtime measurement.
- **Chapter 17: Function-Like Macros & Variadics**:
  - **17.2 Argument Checking**: Defensive parameter verification in complex macros.
  - **17.4 Variable-Length Argument Lists (17.4.2 Variadic Functions)**: **Directly relevant to `new_tuple_pack`.** Mechanics of variadic parameter passing, default argument promotions (floats to doubles, small integers to `int`), and why variadic functions require strict signaling (counts or sentinels).
  - **17.5 Default Arguments**: Emulating optional and default parameters in C.
- **Chapter 18: Type-Generic Programming**: Inherent type-generic capabilities, `_Generic` selection expressions, and anonymous function techniques.
- **Chapter 19: Variations in Control Flow**: Advanced control sequencing, setjmp/longjmp non-local jumps, and signal handlers.
- **Chapter 20–21: Threads & Atomics**: Thread synchronization, critical sections, atomic memory operations, and memory consistency models.
