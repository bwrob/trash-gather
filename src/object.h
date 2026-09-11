#pragma once

#include "stack.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct Object object_t;

typedef struct
{
    size_t size;
    object_t **elements;
} list_t;

typedef struct
{
    size_t size;
    object_t *elements[];
} tuple_t;

typedef enum ObjectKind
{
    INTEGER,
    FLOAT,
    STRING,
    TUPLE,
    LIST,
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

void refcount_inc(
    object_t *obj
);
void refcount_dec(
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

bool list_set(
    object_t *list,
    size_t index,
    object_t *value
);
object_t *list_get(
    object_t *list,
    size_t index
);

object_t *add(
    object_t *a,
    object_t *b
);
