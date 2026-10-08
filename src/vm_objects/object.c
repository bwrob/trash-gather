#include "object.h"

#include "vm/vm.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void object_refcount_inc(
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

void object_refcount_dec(
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
        case INVALID:
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

static void object_refcount_dec_helper(
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
        object_refcount_dec(obj);
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
        case INVALID:
            break;
        case TUPLE:
        {
            tuple_t *t = obj->data.v_tuple;
            for (size_t i = 0; i < t->size; i++)
            {
                object_refcount_dec_helper(t->elements[i], live_only);
            }
            break;
        }
        case LIST:
        {
            list_t arr = obj->data.v_list;
            for (size_t i = 0; i < arr.size; i++)
            {
                object_refcount_dec_helper(arr.elements[i], live_only);
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

object_t *object_new()
{
    object_t *obj = calloc(1, sizeof(*obj));
    if (obj == NULL)
    {
        return NULL;
    }

    obj->is_marked = false;
    obj->refcount = 1;
    vm_track_object(obj);

    return obj;
}

object_t *immortal_new()
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

object_t *integer_new(
    int value
)
{
    object_t *obj = object_new();
    if (obj == NULL)
    {
        return NULL;
    }

    obj->kind = INTEGER;
    obj->data.v_int = value;

    return obj;
}

object_t *float_new(
    float value
)
{
    object_t *obj = object_new();
    if (obj == NULL)
    {
        return NULL;
    }

    obj->kind = FLOAT;
    obj->data.v_float = value;
    return obj;
}

object_t *string_new(
    char *value
)
{
    size_t len = strlen(value);
    char *dst = malloc(len + 1);
    if (dst == NULL)
    {
        return NULL;
    }

    object_t *obj = object_new();
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

object_t *none_create(
    void
)
{
    object_t *obj = immortal_new();
    if (obj == NULL)
    {
        return NULL;
    }
    obj->kind = NONE;
    return obj;
}

object_t *none_get(
    void
)
{
    return vm_get_none();
}

static object_t *add_strings(
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

    object_t *obj = string_new(dst);
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
                    return integer_new(a->data.v_int + b->data.v_int);
                case FLOAT:
                    return float_new((float)a->data.v_int + b->data.v_float);
                default:
                    return NULL;
            }
        }
        case FLOAT:
        {
            switch (b->kind)
            {
                case FLOAT:
                    return float_new(a->data.v_float + b->data.v_float);
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
                    return add_strings(a, b);
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
        case INVALID:
            return -3;
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
