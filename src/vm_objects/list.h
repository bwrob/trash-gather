#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct Object object_t;

typedef struct
{
    size_t size;
    size_t capacity;
    object_t **elements;
} list_t;

object_t *new_list(
    size_t size
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

object_t *list_add(
    object_t *a,
    object_t *b
);
