# Lesson 04: Polymorphic Sequence Protocol, Const Invariants & Preprocessor Macro Pitfalls

**Branch:** `polymorphic-sequence-length`
**Focus:** Implementing the polymorphic sequence protocol (`object_len`), understanding C `const` qualifiers and pointer contracts, avoiding preprocessor macro comma pitfalls in unit tests, and unifying runtime API namespacing.

______________________________________________________________________

## 1. System Engineering: The Polymorphic Sequence Protocol

### Dynamic Protocols in Statically Typed C

In dynamically typed languages like Python, built-in functions such as `len()` exhibit polymorphic behavior: `len("hello")`, `len([1, 2])`, and `len((1, 2, 3))` all invoke a unified sequence protocol (`PySequence_Size` / `sq_length`), while `len(42)` raises a `TypeError`.

In pure ISO C17 without runtime virtual method tables (vtables), this runtime polymorphism is modeled via **tagged union discriminator matching**:

```
object_len(const object_t *obj)
        │
        ├── obj == NULL   ───────► -2 (NULL pointer error)
        │
        ├── switch (obj->kind)
        │       ├── STRING ──────► strlen(v_string)
        │       ├── LIST   ──────► obj->data.v_list.size
        │       ├── TUPLE  ──────► obj->data.v_tuple->size
        │       ├── INTEGER/FLOAT ► -1 (type error: non-sequence)
        │       └── fallback ────► -3 (corrupted / invalid kind)
```

### The Error Sentinel Strategy: Differentiated Error Codes

A classic runtime design trap is returning `0` when an operation is invoked on an invalid type. However, `0` is a valid sequence length representing an **empty sequence** (`""`, `[]`, `()`). Returning `0` for an integer or float would cause non-sequence objects to falsely pass empty-container checks (`if len(x) == 0: ...`).

To provide fine-grained diagnostics and defensive safety across platforms, `object_len` adopts a tiered negative error code strategy:

- **`>= 0`**: Valid sequence element/character count.
- **`-1`**: Non-sequence runtime type (`INTEGER`, `FLOAT`).
- **`-2`**: `NULL` object pointer argument error.
- **`-3`**: Corrupted or unrecognized object kind discriminator.

______________________________________________________________________

## 2. Deep Dive: `const` Qualifiers and Pointer Contracts

### The Right-to-Left Rule

In C, `const` is a type qualifier that establishes compiler-checked read-only contracts. When applied to pointers, its meaning depends strictly on its position relative to the asterisk (`*`):

```
      const object_t        *        const        obj
      ──────────────        ─        ─────        ───
      Pointee Data          *       Pointer    Identifier
      is READ-ONLY                 is CONSTANT
```

1. **`const object_t *obj` (or `object_t const *obj`)**:
   - The **object** is read-only. Modifying fields (`obj->kind = INTEGER`) triggers a compiler error.
   - The **pointer** itself is mutable: `obj = other_obj` is permitted.
1. **`object_t * const obj`**:
   - The **pointer** address cannot change once initialized.
   - The **object** is mutable: `obj->refcount++` is permitted.
1. **`const object_t const *obj`**:
   - Both `const` keywords appear before the `*`. This is an idempotent, redundant qualifier on the object itself, equivalent to `const object_t *obj`.

### Subtyping and the "Widening" Invariant

A function that only inspects an object without modifying it should always accept `const object_t *obj`.

In C's type system:

- A mutable pointer (`object_t *`) can be safely passed to a function expecting `const object_t *` (widening safety / reducing permissions).
- A const pointer (`const object_t *`) **cannot** be passed to a function expecting mutable `object_t *` without triggering `-Wdiscarded-qualifiers`.

Declaring `int64_t object_len(const object_t *obj)` guarantees that both mutable and immutable objects can have their lengths queried without compiler warnings.

______________________________________________________________________

## 3. Pitfalls & Preprocessor Traps

### The Function-like Macro Comma Trap

When writing tests inside test harnesses that use macro blocks (e.g., µnit's `munit_case(RUN, test_name, { ... })`), a subtle C preprocessor trap emerges when initializing arrays:

```c
// ❌ COMPILE ERROR: C Preprocessor splits macro arguments on commas!
munit_case(RUN, test_tuples, {
    object_t *items[] = {i, f, s, i, f, s}; // Preprocessor sees 6 extra arguments!
    object_t *t = new_tuple(items, 6);
});
```

Because the C preprocessor expands macros before lexical analysis, any comma outside of parentheses `(...)` is treated as a macro parameter delimiter—even inside curly braces `{ ... }`!

**Solutions:**

1. **Explicit Index Assignment**:
   ```c
   object_t *items[6];
   items[0] = i; items[1] = f; items[2] = s;
   items[3] = i; items[4] = f; items[5] = s;
   ```
1. **Parenthesized Compound Literals**:
   ```c
   object_t *items[] = {(i), (f), (s), (i), (f), (s)};
   ```

### Switch Fall-Through Undefined Behavior

In ISO C17 (§6.9.1 ¶12), reaching the closing brace of a value-returning function without an explicit `return` triggers undefined behavior. When matching enum kinds in a `switch`:

- Always include an explicit `default: return -1;` or place `return -1;` after the switch block.
- This guarantees defense against corrupt or out-of-range discriminator values (e.g. `(object_kind_t)999`).

______________________________________________________________________

## 4. Architectural Cohesion: API Namespacing

In this milestone, renaming `add(...)` to `object_add(...)` reinforced consistent module namespacing across the runtime:

```c
// Unified object module interface
void      object_free(object_t *obj);
void      object_decref_children(object_t *obj, bool live_only);
object_t *object_add(object_t *a, object_t *b);
int64_t   object_len(const object_t *obj);
```

In C libraries without C++ namespaces or language-level modules, prefixing public API functions with their domain struct name (`object_*`, `list_*`, `vm_*`) prevents symbol collisions when linking against third-party libraries or the standard library.

______________________________________________________________________

## 5. Tooling Takeaways & Verification

1. **`just coverage` at 100.00%**:
   - `src/object.c` achieved 100.00% coverage across 165 lines.
   - Adversarial testing actively verified all sequence branches (`STRING`, `LIST`, `TUPLE`), non-sequence branches (`INTEGER`, `FLOAT`), and the `NULL` boundary condition.
1. **`pre-commit` Gatekeeping**:
   - Automated `clang-format`, `clang-tidy`, Doxygen docstring checks, and roadmap DAG validation ensure clean code hygiene on every milestone commit.
