#pragma once

#include "list.h"
#include "tuple.h"
#include "vm/stack.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define OBJECT_IMMORTAL_REFCOUNT SIZE_MAX

// Object

typedef enum ObjectKind
{
    INTEGER,
    FLOAT,
    STRING,
    TUPLE,
    LIST,
    NONE,
} object_kind_t;

typedef union ObjectData
{
    int v_int;
    float v_float;
    char *v_string;
    tuple_t *v_tuple;
    list_t v_list;
} object_data_t;

struct Object
{
    bool is_marked;
    size_t refcount;
    size_t tracker_id;
    object_kind_t kind;
    object_data_t data;
};

// Memory managment

void object_refcount_inc(
    object_t *obj
);
void object_refcount_dec(
    object_t *obj
);
void object_decref_children(
    object_t *obj,
    bool live_only
);
void object_free_payload(
    object_t *obj
);
void object_free(
    object_t *obj
);

// Immortality

object_t *object_immortal(
    void
);
bool object_is_immortal(
    const object_t *obj
);

// Construction

object_t *object_new(
    void
);
object_t *integer_new(
    int value
);
object_t *float_new(
    float value
);
object_t *string_new(
    char *value
);

// None

object_t *new_none(
    void
);
object_t *create_none_singleton(
    void
);

// Polymorfic object functions

object_t *object_add(
    object_t *a,
    object_t *b
);
int64_t object_len(
    const object_t *obj
);
