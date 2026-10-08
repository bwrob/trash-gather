#pragma once

#include "vm/vm.h"
#include "vm_objects/list.h"
#include "vm_objects/object.h"

object_t *new_object();

object_t *new_integer(
    int value
);
object_t *new_float(
    float value
);
object_t *new_string(
    char *value
);
object_t *new_none(
    void
);
object_t *create_none_singleton(
    void
);
