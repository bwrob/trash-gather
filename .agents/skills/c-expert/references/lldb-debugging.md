# Native C Runtime Debugging with LLDB (`just debug`)

This guide provides a runbook for debugging the C runtime, memory corruption, and AddressSanitizer (ASan) errors using LLDB and the project's test runner.

______

## 1. Quickstart & Launching LLDB

The project's `justfile` provides a direct entry point into LLDB with ASan/UBSan and `bootlib` leak tracking enabled:

```bash
# Debug the entire test suite
just debug

# Debug only tests matching a specific pattern or milestone
just debug test_tuple
just debug test_sequence
just debug test_alloc_fail
```

LLDB will load the test binary and pause at the prompt `(lldb)`. Type `r` (or `run`) to execute.

______

## 2. Essential LLDB Commands for C Runtimes

### 2.1 Navigation & Execution Flow

| Command      | Shorthand | Description                                      |
| :----------- | :-------- | :----------------------------------------------- |
| `run [args]` | `r`       | Start execution from the beginning.              |
| `continue`   | `c`       | Continue running until next breakpoint or crash. |
| `next`       | `n`       | Step over next source line.                      |
| `step`       | `s`       | Step into function call.                         |
| `finish`     | `fin`     | Step out of current function to the caller.      |
| `kill`       | `k`       | Terminate running process.                       |

### 2.2 Breakpoints & Conditional Traps

```bash
# Break at function entry
(lldb) b vm_alloc
(lldb) b tuple_new
(lldb) b gc_sweep

# Break at specific file and line
(lldb) b src/vm.c:142

# Conditional breakpoint (break only when condition holds)
(lldb) b src/vm.c:142 -c 'size > 1024'
(lldb) b vm_alloc -c 'g_boot_fail_alloc_after == 0'

# List and delete breakpoints
(lldb) breakpoint list
(lldb) breakpoint delete 1
```

### 2.3 Variable Inspection & Struct Printing

```bash
# Print variable value
(lldb) p obj
(lldb) p *obj

# Pretty-print specific fields of tagged union / struct
(lldb) p obj->type
(lldb) p obj->refcount
(lldb) p obj->v_tuple->size
(lldb) p obj->v_tuple->items[0]

# Cast void* or raw memory to runtime type
(lldb) p (object_t*)ptr
(lldb) p (tuple_t*)ptr
(lldb) p *(object_t*)ptr

# Format printing
(lldb) p/x (uintptr_t)ptr       # Print pointer in hex
(lldb) p/t (uint8_t)flags       # Print bitflags in binary
(lldb) p/d (int)size            # Print signed integer in decimal
```

### 2.4 Raw Memory Inspection (`memory read` / `x`)

When inspecting memory alignment, header bitmasks, or physical memory layouts:

```bash
# Examine 8 64-bit words in hex (x/8xg <address>)
(lldb) x/8xg ptr

# Examine 16 bytes in hex (x/16xb <address>)
(lldb) x/16xb ptr

# Examine string at address
(lldb) x/s str_ptr

# Inspect struct memory layout
(lldb) memory read --size 8 --format x --count 4 ptr
```

### 2.5 Hardware Watchpoints (Catching Memory Clobbering)

Watchpoints halt execution the **exact instant** a variable or memory address is modified:

```bash
# Break when a struct field is mutated
(lldb) watchpoint set variable obj->refcount

# Break when arbitrary memory address is written to
(lldb) watchpoint set expression -- (size_t*)&obj->refcount

# Break on read or write
(lldb) watchpoint set expression -w read_write -- &obj->flags

# List and delete watchpoints
(lldb) watchpoint list
(lldb) watchpoint delete 1
```

### 2.6 Call Stack & Frame Traversal

```bash
# Print full backtrace
(lldb) bt

# Print top 5 frames
(lldb) bt 5

# Select and inspect a specific caller frame
(lldb) frame select 2
(lldb) frame variable           # List all local variables in this frame
(lldb) p local_ptr
```

______

## 3. Diagnosing AddressSanitizer (ASan) Reports

When ASan traps an error under `just test`, run `just debug <filter>` to pinpoint the root cause:

### 3.1 Heap-Use-After-Free (UAF)

- **Symptom**: `ERROR: AddressSanitizer: heap-use-after-free on address 0x...`
- **Root Cause**: Object container freed before its children or payload, or a pointer was retained after `refcount_dec` or `vm_free`.
- **Debugging Protocol**:
  1. Inspect the two stack traces in the ASan report:
     - `READ of size 8 at 0x...` (where the invalid access happened).
     - `freed by thread T0 here:` (where the memory was prematurely freed).
  1. In LLDB, set a breakpoint at the function where the memory was freed:
     `(lldb) b <free_function>`
  1. Step through and inspect who holds borrowed pointers to that memory.

### 3.2 Heap-Buffer-Overflow

- **Symptom**: `ERROR: AddressSanitizer: heap-buffer-overflow on address 0x...`
- **Root Cause**: Reading or writing past `size` (e.g. flexible array member calculated with wrong size, missing `sizeof(header_t)`).
- **Debugging Protocol**:
  1. Run under LLDB: `just debug <filter>`.
  1. When ASan traps, LLDB pauses immediately at the offending instruction.
  1. Run `frame variable` and print container bounds: `p obj->v_tuple->size`, `p index`.

### 3.3 Stack Overflow / Infinite Cycle Recursion

- **Symptom**: `Segmentation fault: 11` (or stack-overflow).
- **Root Cause**: Graph traversal (`trace_blacken_object`, `print_object`) encountered a cyclic reference without cycle suppression.
- **Debugging Protocol**:
  1. Run `(lldb) bt 20` to see if the stack repeats the same 2-3 functions.
  1. Inspect the cycle: `p obj`, `p obj->v_tuple->items[0]`.

______

## 4. Leak Hunting with `bootlib`

When `assert(boot_all_freed())` fails:

1. `bootlib` prints the file, line, and size of every unfreed allocation.
1. In LLDB, set a breakpoint at that exact source line:
   `(lldb) b src/new.c:84`
1. Trace the lifecycle of the allocated pointer: where was its refcount incremented, and which teardown branch failed to release it?
