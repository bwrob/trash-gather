#pragma once

#include <stddef.h>

typedef struct Object object_t;

typedef struct
{
    size_t size;
    object_t *elements[];
} tuple_t;

object_t *tuple_new_0(
    void
);
object_t *tuple_new_1(
    object_t *x
);
object_t *tuple_new_2(
    object_t *x,
    object_t *y
);
object_t *tuple_new_3(
    object_t *x,
    object_t *y,
    object_t *z
);
object_t *tuple_new(
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
