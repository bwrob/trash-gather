#include "object.h"

#include "list.h"
#include "new.h"
#include "tuple.h"
#include "vm/vm.h"

#include <stdint.h>
#include <string.h>

void refcount_inc(
    object_t *obj
)
{
    if (obj == NULL || object_is_immortal(obj))
    {
        return;
    }
    obj->refcount++;
    return;
}

void refcount_dec(
    object_t *obj
)
{
    if (obj == NULL || object_is_immortal(obj))
    {
        return;
    }
    obj->refcount--;

    if (obj->refcount == 0)
    {
        object_free(obj);
        return;
    }
    return;
}

void object_free_payload(
    object_t *obj
)
{
    if (obj == NULL)
    {
        return;
    }
    switch (obj->kind)
    {
        case INTEGER:
        case FLOAT:
        case NONE:
            break;
        case TUPLE:
        {
            free(obj->data.v_tuple);
            break;
        }
        case STRING:
        {
            free(obj->data.v_string);
            break;
        }
        case LIST:
        {
            free(obj->data.v_list.elements);
            break;
        }
    }
}

static void _refcount_dec(
    object_t *obj,
    bool live_only
)
{
    if (obj == NULL)
    {
        return;
    }
    if (!live_only || obj->is_marked)
    {
        refcount_dec(obj);
    }
}

void object_decref_children(
    object_t *obj,
    bool live_only
)
{
    switch (obj->kind)
    {
        case INTEGER:
        case FLOAT:
        case STRING:
        case NONE:
            break;
        case TUPLE:
        {
            tuple_t *t = obj->data.v_tuple;
            for (size_t i = 0; i < t->size; i++)
            {
                _refcount_dec(t->elements[i], live_only);
            }
            break;
        }
        case LIST:
        {
            list_t arr = obj->data.v_list;
            for (size_t i = 0; i < arr.size; i++)
            {
                _refcount_dec(arr.elements[i], live_only);
            }
            break;
        }
    }
}

void object_free(
    object_t *obj
)
{
    bool live_only = false;
    object_decref_children(obj, live_only);
    object_free_payload(obj);
    vm_untrack_object(obj);
    free(obj);
}

static object_t *_add_strings(
    object_t *a,
    object_t *b
)
{
    int a_len = strlen(a->data.v_string);
    int b_len = strlen(b->data.v_string);
    int len = a_len + b_len + 1;

    char *dst = malloc(len * sizeof(char));
    if (dst == NULL)
    {
        return NULL;
    }

    dst[0] = '\0';
    strcat(dst, a->data.v_string);
    strcat(dst, b->data.v_string);

    object_t *obj = new_string(dst);
    free(dst);

    return obj;
}

object_t *object_add(
    object_t *a,
    object_t *b
)
{
    if (a == NULL || b == NULL)
    {
        return NULL;
    }

    switch (a->kind)
    {
        case INTEGER:
        {
            switch (b->kind)
            {
                case INTEGER:
                    return new_integer(a->data.v_int + b->data.v_int);
                case FLOAT:
                    return new_float((float)a->data.v_int + b->data.v_float);
                default:
                    return NULL;
            }
        }
        case FLOAT:
        {
            switch (b->kind)
            {
                case FLOAT:
                    return new_float(a->data.v_float + b->data.v_float);
                default:
                    return object_add(b, a);
            }
        }
        case STRING:
        {
            switch (b->kind)
            {
                case STRING:
                {
                    return _add_strings(a, b);
                }
                default:
                    return NULL;
            }
        }
        case TUPLE:
        {
            // Tuples are constant length, addition is done over coordinates.
            switch (b->kind)
            {
                case TUPLE:
                {
                    return tuple_add(a, b);
                }
                default:
                    return NULL;
            }
        }
        case LIST:
        {
            // Lists are variable length, addition is concatenation.
            switch (b->kind)
            {
                case LIST:
                {
                    return list_add(a, b);
                }
                default:
                    return NULL;
            }
        }
        default:
            return NULL;
    }
}

int64_t object_len(
    const object_t *obj
)
{
    if (obj == NULL)
    {
        return -2;
    }
    switch (obj->kind)
    {
        case INTEGER:
        case FLOAT:
        case NONE:
            return -1;
        case STRING:
            return strlen(obj->data.v_string);
        case TUPLE:
            return obj->data.v_tuple->size;
        case LIST:
            return obj->data.v_list.size;
    }
    return -3;
}

bool object_is_immortal(
    const object_t *obj
)
{
    if (obj == NULL)
    {
        return false;
    }
    return obj->refcount == OBJECT_IMMORTAL_REFCOUNT;
}
