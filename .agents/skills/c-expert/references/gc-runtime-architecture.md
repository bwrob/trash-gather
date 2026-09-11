# Garbage Collection & Runtime Architecture

This reference guides the design, implementation, and code review of runtime environments, object models, and garbage collection systems in C.

______________________________________________________________________

## 1. Object Models & Memory Layout

### Uniform Envelope vs. Heterogeneous Payloads

In managed runtimes (like Python, Lua, or custom VMs), objects typically fall into one of two layout architectures:

1. **Inline Tagged Union (Uniform `sizeof(object_t)`)**:

   - Every object has identical byte width, containing header flags (type tag, GC mark bits, refcount) and a `union` of primitive values (ints, floats, pointers).
   - *Advantages*: Uniform allocation size; simple object tracker pools; predictable spatial layout.
   - *Constraint*: Variable-length structures cannot be embedded directly by value into unions (C99 §6.7.2.1).

1. **Pointer to Payload (Two Allocations)**:

   - The uniform `object_t` holds a pointer to a dedicated heap-allocated structure (e.g., `tuple_t *v_tuple`).
   - *Advantages*: Preserves fixed `sizeof(object_t)` while accommodating dynamically sized payloads.
   - *Lifecycle Invariant*: When reclaiming the object, `object_free_payload()` must free the inner payload before the outer `object_t` is freed.

1. **Unified Single Allocation (Variable-Sized Object)**:

   - The object header and variable payload sit contiguous in a single heap block: `sizeof(object_t) + N * sizeof(item)`.
   - *Advantages*: Eliminates pointer chasing and cuts allocation count in half.
   - *Trade-off*: Requires size-segregated memory pools or dynamic allocators aware of varying object sizes.

______________________________________________________________________

## 2. Flexible Array Members (C99 §6.7.2.1)

Flexible array members provide contiguous payload storage without pointer indirection.

### Structure Definition

```c
typedef struct {
  size_t size;
  object_t *items[]; /* Flexible array member: must be the last struct member */
} tuple_t;
```

### Allocation Arithmetic (SEI CERT C MEM33-C)

Because `sizeof(tuple_t)` **omits** the flexible array member entirely under C99/C17 (MEM33-C), it contains space for `0` elements:

$$\\text{allocation_size} = \\text{sizeof}(\\text{tuple_t}) + N \\times \\text{sizeof}(\\text{object_t}\*)$$

### Correctness Rules

1. **Overflow Safety**: Always check for integer overflow before multiplication:
   ```c
   if (size > (SIZE_MAX - sizeof(tuple_t)) / sizeof(object_t *)) {
     return NULL; /* Overflow hazard prevented */
   }
   ```
1. **Slot Safety (Zero Initialization)**: All elements `items[0 .. size-1]` must be explicitly initialized to `NULL` (via `memset`, `calloc`, or explicit loop) immediately upon allocation. Leaving uninitialized pointers causes undefined behavior if GC tracing runs before population.
1. **Union Invariant**: A struct containing a flexible array member **cannot** appear directly by value inside a `union`. It must be referenced via pointer (`tuple_t *v_tuple`).

______________________________________________________________________

## 3. Reference Counting & Ownership Contracts

### Borrowed vs. Owned References

Every function accepting or returning an `object_t*` must declare ownership semantics:

- **Owned Reference**: The receiver assumes responsibility for the reference count.
  - Passing an owned reference transfers ownership (the caller drops its obligation or increments the refcount if keeping a copy).
- **Borrowed Reference**: The pointer is valid only as long as the parent container or caller frame retains its owned reference.
  - Accessors (`tuple_get(tuple, index)`) typically return borrowed references. If the caller stores the pointer long-term, it must explicitly call `refcount_inc()`.

### Symmetrical Balance

- Inserting a child object into a container (`tuple_set`, `list_append`) must increment the child's reference count (`refcount_inc(child)`).
- Releasing or overwriting an entry must decrement the old child's reference count (`refcount_dec(old)`).
- Container deallocation must recursively decrement children (`object_decref_children()`).

### The Cycle Hazard

Reference counting alone **cannot** reclaim cyclic reference graphs (e.g., $A \\to B \\to A$). Any object holding reference cycles will leak unless an auxiliary tracing collector (or cycle detector) breaks or sweeps the cycle.

______________________________________________________________________

## 4. Tracing & Mark-and-Sweep Integration

When implementing or reviewing a tracing GC:

### 1. Root Registration

The garbage collector traces reachability starting from the **root set**:

- Thread call frames / local variable slots (`frame_t`).
- VM operand / evaluation stack (`stack_t`).
- Global / static symbol tables.

*Rule*: Any newly allocated object that is not yet stored into a rooted container or frame slot must be rooted or tracked, or GC must be inhibited during its construction to prevent premature collection.

### 2. Tracing Completeness (`trace_blacken_object`)

Every container variant that can store object pointers must be handled in the GC tracing phase:

- **Tuples / Arrays**: Iterate $i \\in [0, \\text{size}-1]$ and blacken `items[i]`. Guard against `NULL` slots!
- **Cyclic Meshes**: The marker must check if an object is already marked before recursing; otherwise, cycles cause unbounded stack overflow.

### 3. Sweeping & Teardown Symmetry

During the sweep phase:

- Unmarked objects are unlinked from the VM object tracker.
- `object_free_payload(obj)` releases secondary heap buffers (`tuple_t*`, string buffers).
- `free(obj)` releases the object envelope.
- Marked objects have their mark bit reset to unmarked (white) for the next cycle.

______________________________________________________________________

## 5. Allocation Failure Rollback (`goto cleanup`)

When an operation requires multiple sequential allocations (e.g., allocating an `object_t` envelope followed by a `tuple_t` payload):

```c
object_t *new_tuple(size_t size) {
  object_t *obj = new_object(TUPLE);
  if (obj == NULL) {
    return NULL;
  }

  size_t alloc_bytes = sizeof(tuple_t) + size * sizeof(object_t *);
  tuple_t *tuple = malloc(alloc_bytes);
  if (tuple == NULL) {
    goto fail_tuple;
  }

  tuple->size = size;
  for (size_t i = 0; i < size; i++) {
    tuple->items[i] = NULL;
  }

  obj->data.v_tuple = tuple;
  return obj;

fail_tuple:
  /* Roll back: do not leak obj */
  object_free_raw(obj);
  return NULL;
}
```

*Rule*: Never return `NULL` from a multi-stage allocator without unwinding all previous allocations in reverse order.
