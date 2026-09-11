#include "new.h"

#include "object.h"
#include "vm.h"

#include <stdlib.h>
#include <string.h>

static object_t *_new_object()
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

object_t *new_list(
    size_t size
)
{
    object_t **elements = calloc(size, sizeof(object_t *));
    if (elements == NULL)
    {
        return NULL;
    }

    object_t *obj = _new_object();
    if (obj == NULL)
    {
        free(elements);
        return NULL;
    }

    obj->kind = LIST;
    obj->data.v_list = (list_t){.size = size, .elements = elements};

    return obj;
}

/**
 * @brief Tuple constructor; empty variant.
 *
 * @return object_t*
 */

object_t *_new_tuple_obj(
    size_t tuple_size
)
{
    tuple_t *tuple = malloc(sizeof(*tuple) + (tuple_size * sizeof(tuple->elements[0])));
    if (tuple == NULL)
    {
        return NULL;
    }

    object_t *obj = _new_object();
    if (obj == NULL)
    {
        free(tuple);
        return NULL;
    }

    tuple->size = tuple_size;
    obj->kind = TUPLE;
    obj->data.v_tuple = tuple;
    return obj;
}

object_t *new_tuple_0()
{
    return new_tuple(NULL, 0);
}

object_t *new_tuple_1(
    object_t *x
)
{
    object_t *items[] = {x};
    return new_tuple(items, 1);
}

object_t *new_tuple_2(
    object_t *x,
    object_t *y
)
{
    object_t *items[] = {x, y};
    return new_tuple(items, 2);
}

object_t *new_tuple_3(
    object_t *x,
    object_t *y,
    object_t *z
)
{
    object_t *items[] = {x, y, z};
    return new_tuple(items, 3);
}

object_t *new_tuple(
    object_t **objects,
    size_t size
)
{
    if (size == 0 && objects == NULL)
    {
        return _new_tuple_obj(0);
    }

    if (objects == NULL)
    {
        return NULL;
    }

    for (size_t i = 0; i < size; i++)
    {
        if (objects[i] == NULL)
        {
            return NULL;
        }
    }

    object_t *obj = _new_tuple_obj(size);
    if (obj == NULL)
    {
        return NULL;
    }

    obj->kind = TUPLE;
    for (size_t i = 0; i < size; i++)
    {
        obj->data.v_tuple->elements[i] = objects[i];
        refcount_inc(obj->data.v_tuple->elements[i]);
    }
    return obj;
}

object_t *new_integer(
    int value
)
{
    object_t *obj = _new_object();
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
    object_t *obj = _new_object();
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

    object_t *obj = _new_object();
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
