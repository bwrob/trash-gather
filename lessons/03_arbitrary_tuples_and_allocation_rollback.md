# Lesson 03: Variable-Length Tuples, Allocation Rollback & Reference Parity

**Branch:** `tuples-arbitrary-length`
**Focus:** Memory representation of variable-length composite objects, two-phase allocation rollback, container reference count parity, `realloc` pointer traps, and adversarial test heuristics.

______________________________________________________________________

## 1. System Engineering: Variable-Length Composite Layouts

### From Rigid Structs to Dynamic Tuples

Early iterations of runtime objects often rely on rigid, fixed-size structs:

```c
// Early prototype: hardcoded 3-element vector
typedef struct vector3 {
    object_t *x;
    object_t *y;
    object_t *z;
} vector3_t;
```

While straightforward, fixed structures fail to generalize to language constructs like arbitrary tuples (`()`, `(x,)`, `(a, b, c, d)`). Supporting arbitrary-length tuples requires an architectural decision regarding memory layout.

### Memory Layout: Double Indirection vs. Contiguous Flexible Buffers

```
Approach 1: Double Indirection (Separate Buffer)
+-------------+      +-------------------+
| tuple_t     | ---> | object_t *elem[N] |
| size: N     |      +-------------------+
+-------------+

Approach 2: Contiguous Flexible Buffer (Single Allocation)
+------------------------------------+
| tuple_t | elem[0] | elem[1] | ...  |
| size: N |         |         |      |
+------------------------------------+
```

1. **Double Indirection (Separate Pointer Array):**
   - The `tuple_t` header holds a pointer to a separately allocated array of pointers (`object_t **elements`).
   - **Trade-off:** Requires two heap allocations (`malloc` for struct + `malloc` for buffer), introduces pointer chasing and cache misses during iteration, and requires multiple `free` calls during teardown.
1. **Contiguous Layout (C99 Flexible Array Member):**
   - The struct tail contains an unsized array:
     ```c
     typedef struct tuple {
         size_t size;
         object_t *elements[]; // C99 Flexible Array Member
     } tuple_t;
     ```
   - Allocated with a single contiguous allocation:
     ```c
     tuple_t *t = malloc(sizeof(*t) + (size * sizeof(t->elements[0])));
     ```
   - **Trade-off:** Eliminates pointer indirection between the header and elements, guarantees spatial cache locality, and a single `free(tuple)` releases both the header and all element slots.

> [!IMPORTANT]
> **Allocation Sizing Arithmetic:**
> When allocating flexible array members, always size element slots using `sizeof(t->elements[0])` (pointer size, typically 8 bytes) rather than `sizeof(object_t)` (struct size, 48 bytes). Sizing against the struct type instead of the pointer element wastes up to $6\\times$ more memory per slot.

______________________________________________________________________

## 2. The Two-Phase Construction & Allocation Rollback Trap

### The Problem: Unwinding Tracked Allocations

In a runtime where the VM tracks all allocated objects in an internal registry, composite object creation involves two distinct steps:

1. Allocating the generic object envelope (`object_t`) and registering it with the VM's active object tracker (`vm_track_object(obj)`).
1. Allocating the specific payload data (e.g., `tuple_t` buffer).

#### The Naive Failure Flow:

```c
// ANTI-PATTERN: Registration before payload verification
object_t *obj = _new_object(); // Immediately calls vm_track_object(obj)
if (obj == NULL) return NULL;

tuple_t *tuple = malloc(sizeof(*tuple) + (tuple_size * sizeof(tuple->elements[0])));
if (tuple == NULL) {
    free(obj); // FATAL: Frees memory, but leaves a dangling pointer in the VM!
    return NULL;
}
```

### The Diagnostic Symptom (AddressSanitizer `heap-use-after-free`)

When simulating allocation failures (`boot_set_fail_alloc_after`), AddressSanitizer catches this exact defect during VM teardown:

```text
=================================================================
==ERROR: AddressSanitizer: heap-use-after-free on address 0x6040000007a8
READ of size 4 at 0x6040000007a8 thread T0
    #0 object_free_payload object.c:42
    #1 vm_free vm.c:213
...
0x6040000007a8 was freed by thread T0 here:
    #0 free (libclang_rt.asan_osx_dynamic.dylib)
    #1 _new_tuple_obj new.c:66
```

### Why Naive Unwinding Fails

When `_new_object()` executes `vm_track_object(obj)`, `obj` is appended to `CURRENT_VM->objects`.
If the subsequent payload allocation fails and the caller invokes `free(obj)`:

- The heap block is returned to the OS allocator.
- **The VM tracker is never updated.** The pointer to `obj` remains active in `CURRENT_VM->objects`.
- Later, when `vm_free()` or `vm_collect_garbage()` sweeps the heap, it attempts to inspect `obj->kind` $\\rightarrow$ **Use-After-Free / Memory Corruption**.

### The Architectural Invariant: "Payload First"

Whenever an object requires multiple allocations, **allocate all untracked payloads first before introducing the object to global runtime state**:

```c
// SAFE PATTERN: Allocate payload before registering with the VM
tuple_t *tuple = malloc(sizeof(*tuple) + (tuple_size * sizeof(tuple->elements[0])));
if (tuple == NULL) {
    return NULL; // Failed before any global state was altered
}

object_t *obj = _new_object();
if (obj == NULL) {
    free(tuple); // Safely clean up unshared buffer; VM has no dangling reference
    return NULL;
}

obj->kind = TUPLE;
obj->data.v_tuple = tuple;
return obj;
```

> [!TIP]
> **Transactional Construction Rule:**
> An object must only be registered with the garbage collector's tracking structures once all requisite sub-allocations have succeeded and the object is structurally sound.

______________________________________________________________________

## 3. Container Ownership, Reference Parity & The `vm_free()` Masking Trap

### The Lifecycle of Nested Allocations

When a container operation creates new child elements to populate a new parent container (such as component-wise tuple addition `add(tupleA, tupleB)`):

1. `add(a[i], b[i])` produces a new element with **`refcount = 1`** (owned by the local frame).
1. `new_tuple(added_objects, N)` loops over elements and calls **`refcount_inc(objects[i])`**, raising each child's refcount to **`2`**.
1. The function returns `tuple` to the caller.

### The Problem: Zombie Objects

If the creator function does not explicitly relinquish its temporary references:

- When the caller later destroys `tuple` via `refcount_dec(tuple)`, `object_decref_children` decrements each element from **2 to 1**.
- Because their refcount is still 1, **the child elements are never freed by reference counting**. They remain orphaned in memory until swept by a full GC pass or program termination.

### The Solution: Ownership Transfer Handshake

Whenever a function creates objects purely to insert them into a container that increments reference counts, ownership is transferred to the container. The creator must drop its local references:

```c
object_t *tuple = new_tuple(added_objects, a_len);

// Ownership was passed to the tuple: release local temporary references
for (size_t i = 0; i < a_len; i++) {
    refcount_dec(added_objects[i]);
}
free(added_objects);
return tuple;
```

Now each child element goes: `1 (created) -> 2 (new_tuple) -> 1 (refcount_dec)`. When `tuple` is decremented to 0, its children cascade to 0 and are freed immediately.

### Why Initial Tests Missed It: The `vm_free()` Masking Trap

Standard teardown in test fixtures typically calls:

```c
vm_free();
assert(boot_all_freed());
```

`vm_free()` iterates over `CURRENT_VM->objects` and unconditionally frees **all** allocated objects regardless of reference counts. Relying solely on `vm_free()` **masks reference count leaks**!

To detect reference leaks, tests must explicitly drop the parent container via pure reference counting:

```c
refcount_dec(res);
vm_cleanup_after_refcount(); // Clears tracking array so only refcount drops free memory
assert(boot_all_freed());    // Fails if any child refcount was 2 instead of 1!
```

______________________________________________________________________

## 4. The Classic `realloc` Pointer Overwrite and Double-Free Traps

### The Pointer Overwrite Bug

When dynamic arrays expand (e.g. `stack_push`), resizing must use `realloc`. A frequent mistake is writing:

```c
// ANTI-PATTERN: Direct assignment to target pointer
stack->capacity *= 2;
stack->data = realloc(stack->data, stack->capacity * sizeof(void *));
if (stack->data == NULL) {
    return false;
}
```

- When `realloc` fails, it returns `NULL`.
- **Crucially: the original buffer is NOT freed by the OS.**
- Because `stack->data` was overwritten with `NULL`, the pointer to the original memory block is lost forever $\\rightarrow$ **Permanent Heap Leak**.
- Furthermore, `stack->capacity` was already mutated, corrupting struct consistency.

### The Double-Free Bug

Another trap is attempting to "clean up" the buffer inside the push function:

```c
void *new_data = realloc(stack->data, new_capacity * sizeof(void *));
if (new_data == NULL) {
    free(stack->data); // ANTI-PATTERN: Double-Free hazard!
    return false;
}
```

`stack_push` is an insertion operation, not a destructor. Existing elements in `stack->data` are still valid. Freeing `stack->data` turns it into a dangling pointer. When the caller later invokes `stack_free(stack)`, it calls `free(stack->data)` again $\\rightarrow$ **AddressSanitizer: attempting double-free abort**.

### The Canonical `realloc` Pattern

Never mutate the struct until allocation success is guaranteed:

```c
if (stack->count == stack->capacity) {
    size_t new_capacity = stack->capacity * 2;
    void *new_data = realloc(stack->data, new_capacity * sizeof(void *));
    if (new_data == NULL) {
        return false; // Struct is untouched, caller's existing data remains intact
    }
    stack->data = new_data;
    stack->capacity = new_capacity;
}
stack->data[stack->count++] = obj;
return true;
```

______________________________________________________________________

## 5. Mid-Loop Rollback ($K$-of-$N$) vs. Uninitialized Pointer Traps

### Partial Failure in Iterative Allocations

When populating $N$ elements in a loop, failure can occur at step $k$ (where $0 < k < N$) due to incompatible element types or heap exhaustion:

```c
for (size_t i = 0; i < a_len; i++) {
    added_objects[i] = add(a->elements[i], b->elements[i]);
    if (added_objects[i] == NULL) {
        failure_index = i;
        break;
    }
}
```

### The Uninitialized Pointer Trap

If `added_objects = malloc(...)` was used, slots $k+1 \\dots N-1$ contain **wild uninitialized pointers**.
If the failure cleanup loop attempts to iterate across all $N$ elements:

```c
// DANGEROUS: Dereferences uninitialized pointers in slots k+1 .. N-1
for (size_t i = 0; i < a_len; i++) {
    refcount_dec(added_objects[i]); // CRASH / Wild pointer dereference!
}
```

### The Robust Unwind Pattern

Failure cleanup must strictly unwind only the range of elements that were successfully initialized ($0 \\dots \\text{failure_index} - 1$):

```c
if (failure_index < a_len) {
    for (size_t i = 0; i < failure_index; i++) {
        refcount_dec(added_objects[i]); // Safely drops only allocated items
    }
    free(added_objects);
    return NULL;
}
```

______________________________________________________________________

## 6. Defensive Pointer Validation: Containers vs. Elements

### The Indexing Precondition Trap

When constructing containers from variable-length pointer arrays, API functions must validate both the **container array pointer itself** and its **individual elements**:

```c
object_t *new_tuple(object_t **objects, size_t size);
```

#### The Naive Implementation:

```c
// BUGGY: Assumes `objects` is non-NULL if size > 0
for (size_t i = 0; i < size; i++) {
    if (objects[i] == NULL) { // Dereferences NULL if objects == NULL!
        return NULL;
    }
}
```

In C, array indexing `objects[i]` is identical to `*(objects + i)`.
If a caller passes `new_tuple(NULL, 3)`:
`i = 0` calculates `*(NULL + 0)` $\\rightarrow$ **Page 0 Read Segfault (`SEGV on unknown address 0x000000000000`)**.

### The Invariant: Container Bounds Precede Content Inspection

Validation must occur in strict logical hierarchy:

1. If `size == 0 && objects == NULL`, return an empty container (`new_tuple_0()`).
1. If `size > 0 && objects == NULL`, reject immediately (`return NULL;`).
1. Only once the array pointer is verified valid may individual slots (`objects[i]`) be probed.

______________________________________________________________________

## 7. Tooling & Testing Heuristics: Beyond the Line Coverage Illusion

### Why Line Coverage Was an Illusion

Early in this milestone, `gcov` reported **100% line coverage** on `_add_tuples`, even though:

1. Child elements had incorrect reference counts (`refcount == 2` instead of `1`).
1. Mid-loop failures leaked intermediate allocations.

A single happy-path test executed every line in the function sequentially (`malloc`, `for`, `new_tuple`, `free`, `return`), satisfying line coverage while testing zero edge cases.

### The Obligatory Adversarial Heuristics

To prevent coverage illusions, test generation is governed by three non-negotiable heuristics derived from [`c-expert`](.agents/skills/c-expert/SKILL.md):

1. **The Container Lifecycle Probe:** Always test container destruction via `refcount_dec()` with `vm_cleanup_after_refcount(); assert(boot_all_freed())`. Never rely solely on `vm_free()`.
1. **The $K$-of-$N$ Mid-Loop Probe:** Always construct inputs where element $0$ succeeds and element $1$ fails, verifying early abort and clean unwinding.
1. **Heap Exhaustion Sweeps:** Sweep `boot_set_fail_alloc_after(i)` across all fallible allocation steps to verify rollback.
