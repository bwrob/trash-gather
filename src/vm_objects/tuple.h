#pragma once

#include <stddef.h>

typedef struct Object object_t;

typedef struct
{
    size_t size;
    object_t *elements[];
} tuple_t;

object_t *new_tuple_0(
    void
);
object_t *new_tuple_1(
    object_t *x
);
object_t *new_tuple_2(
    object_t *x,
    object_t *y
);
object_t *new_tuple_3(
    object_t *x,
    object_t *y,
    object_t *z
);
object_t *new_tuple(
    object_t **objects,
    size_t size
);

object_t *tuple_add(
    object_t *a,
    object_t *b
);

object_t *create_empty_tuple_singleton(
    void
);
