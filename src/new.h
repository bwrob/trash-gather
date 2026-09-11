#pragma once

#include "object.h"
#include "vm.h"

object_t *new_integer(
    int value
);
object_t *new_float(
    float value
);
object_t *new_string(
    char *value
);
object_t *new_list(
    size_t size
);

object_t *new_tuple_0();
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
