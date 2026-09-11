#include "new.h"

#include "object.h"
#include "vm.h"

#include <stdlib.h>
#include <string.h>

object_t *_new_object()
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
    object_t *obj = _new_object();
    if (obj == NULL)
    {
        return NULL;
    }

    tuple_t *tuple = malloc(sizeof(tuple_t) + (tuple_size * sizeof(object_t)));
    if (tuple == NULL)
    {
        free(obj);
        return NULL;
    }

    tuple->size = tuple_size;
    obj->kind = TUPLE;
    obj->data.v_tuple = tuple;
    return obj;
}

object_t *new_tuple_0()
{
    object_t *obj = _new_tuple_obj(0);
    if (obj == NULL)
    {
        return NULL;
    }
    return obj;
}

object_t *new_tuple_1(
    object_t *x
)
{
    if (x == NULL)
    {
        return NULL;
    }

    object_t *obj = _new_tuple_obj(1);
    if (obj == NULL)
    {
        return NULL;
    }
    obj->data.v_tuple->elements[0] = x;
    refcount_inc(x);
    return obj;
}

object_t *new_tuple_2(
    object_t *x,
    object_t *y
)
{
    if (x == NULL || y == NULL)
    {
        return NULL;
    }

    object_t *obj = _new_tuple_obj(2);
    if (obj == NULL)
    {
        return NULL;
    }

    obj->data.v_tuple->elements[0] = x;
    obj->data.v_tuple->elements[1] = y;
    refcount_inc(x);
    refcount_inc(y);
    return obj;
}

object_t *new_tuple_3(
    object_t *x,
    object_t *y,
    object_t *z
)
{
    if (x == NULL || y == NULL || z == NULL)
    {
        return NULL;
    }

    object_t *obj = _new_tuple_obj(3);
    if (obj == NULL)
    {
        return NULL;
    }

    obj->data.v_tuple->elements[0] = x;
    obj->data.v_tuple->elements[1] = y;
    obj->data.v_tuple->elements[2] = z;
    refcount_inc(x);
    refcount_inc(y);
    refcount_inc(z);
    return obj;
}

object_t *new_tuple(
    object_t **objects,
    size_t size
)
{
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
