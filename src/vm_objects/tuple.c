#include "tuple.h"

#include "new.h"
#include "object.h"
#include "vm/vm.h"

static object_t *_tuple_new_obj(
    size_t tuple_size
)
{
    tuple_t *tuple = malloc(sizeof(*tuple) + (tuple_size * sizeof(tuple->elements[0])));
    if (tuple == NULL)
    {
        return NULL;
    }

    object_t *obj = new_object();
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

object_t *create_empty_tuple_singleton()
{
    tuple_t *tuple = malloc(sizeof(*tuple));
    if (tuple == NULL)
    {
        return NULL;
    }

    object_t *obj = immortal_object();
    if (obj == NULL)
    {
        free(tuple);
        return NULL;
    }

    tuple->size = 0;
    obj->kind = TUPLE;
    obj->data.v_tuple = tuple;
    return obj;
}

object_t *tuple_new_0()
{
    return tuple_new(NULL, 0);
}

object_t *tuple_new_1(
    object_t *x
)
{
    object_t *items[] = {x};
    return tuple_new(items, 1);
}

object_t *tuple_new_2(
    object_t *x,
    object_t *y
)
{
    object_t *items[] = {x, y};
    return tuple_new(items, 2);
}

object_t *tuple_new_3(
    object_t *x,
    object_t *y,
    object_t *z
)
{
    object_t *items[] = {x, y, z};
    return tuple_new(items, 3);
}

object_t *tuple_new(
    object_t **objects,
    size_t size
)
{
    if (size == 0)
    {
        return vm_get_empty_tuple();
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

    object_t *obj = _tuple_new_obj(size);
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

object_t *tuple_add(
    object_t *a,
    object_t *b
)
{
    size_t a_len = a->data.v_tuple->size;
    size_t b_len = b->data.v_tuple->size;
    if (a_len != b_len)
    {
        return NULL;
    }
    if (a_len == 0)
    {
        return tuple_new_0();
    }

    object_t **added_objects = malloc(sizeof(object_t *) * a_len);
    if (added_objects == NULL)
    {
        return NULL;
    }

    size_t failure_index = SIZE_MAX;
    for (size_t i = 0; i < a_len; i++)
    {
        added_objects[i] =
            object_add(a->data.v_tuple->elements[i], b->data.v_tuple->elements[i]);
        if (added_objects[i] == NULL)
        {
            failure_index = i;
            break;
        }
    }

    // Mid-addition failure cleanup
    if (failure_index < a_len)
    {
        for (size_t i = 0; i < failure_index; i++)
        {
            refcount_dec(added_objects[i]);
        }
        free(added_objects);
        return NULL;
    }

    object_t *tuple = tuple_new(added_objects, a_len);

    // Ownership was passed to the tuple, we need to release
    // the reference and memory.
    for (size_t i = 0; i < a_len; i++)
    {
        refcount_dec(added_objects[i]);
    }
    free(added_objects);

    return tuple;
}
