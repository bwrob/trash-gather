#include "vm_objects/new.h"

#include "vm/vm.h"
#include "vm_objects/object.h"

#include <stdlib.h>
#include <string.h>

object_t *new_object()
{
    object_t *obj = calloc(1, sizeof(object_t));
    if (obj == NULL)
    {
        return NULL;
    }

    obj->is_marked = false;
    obj->refcount = 1;
    vm_track_object(obj);

    return obj;
}

object_t *immortal_object()
{
    object_t *obj = calloc(1, sizeof(*obj));
    if (obj == NULL)
    {
        return NULL;
    }

    obj->is_marked = false;
    obj->refcount = OBJECT_IMMORTAL_REFCOUNT;
    return obj;
}

object_t *new_integer(
    int value
)
{
    object_t *obj = new_object();
    if (obj == NULL)
    {
        return NULL;
    }

    obj->kind = INTEGER;
    obj->data.v_int = value;

    return obj;
}

object_t *new_float(
    float value
)
{
    object_t *obj = new_object();
    if (obj == NULL)
    {
        return NULL;
    }

    obj->kind = FLOAT;
    obj->data.v_float = value;
    return obj;
}

object_t *new_string(
    char *value
)
{
    int len = strlen(value);
    char *dst = malloc(len + 1);
    if (dst == NULL)
    {
        return NULL;
    }

    object_t *obj = new_object();
    if (obj == NULL)
    {
        free(dst);
        return NULL;
    }

    strcpy(dst, value);

    obj->kind = STRING;
    obj->data.v_string = dst;
    return obj;
}

object_t *create_none_singleton()
{
    object_t *obj = immortal_object();
    if (obj == NULL)
    {
        return NULL;
    }
    obj->kind = NONE;
    return obj;
}

object_t *new_none()
{
    return vm_get_none();
}
