#include "list.h"

#include "new.h"
#include "object.h"

#include <stdint.h>

bool list_set(
    object_t *list,
    size_t index,
    object_t *value
)
{
    if (list == NULL || value == NULL)
    {
        return false;
    }
    if (list->kind != LIST)
    {
        return false;
    }
    if (index >= list->data.v_list.size)
    {
        return false;
    }

    if (list->data.v_list.elements[index] != NULL)
    {
        refcount_dec(list->data.v_list.elements[index]);
    }
    list->data.v_list.elements[index] = value;
    refcount_inc(value);
    return true;
}

object_t *list_get(
    object_t *list,
    size_t index
)
{
    if (list == NULL)
    {
        return NULL;
    }
    if (list->kind != LIST)
    {
        return NULL;
    }
    if (index >= list->data.v_list.size)
    {
        return NULL;
    }

    // Get the value directly now (already checked size constraint)
    return list->data.v_list.elements[index];
}

object_t *add_lists(
    object_t *a,
    object_t *b
)
{
    size_t a_len = a->data.v_list.size;
    size_t b_len = b->data.v_list.size;
    size_t length = a_len + b_len;

    object_t *list = new_list(length);
    if (list == NULL)
    {
        return NULL;
    }

    for (size_t i = 0; i < a_len; i++)
    {
        list_set(list, i, list_get(a, i));
    }
    for (size_t i = 0; i < b_len; i++)
    {
        list_set(list, i + a_len, list_get(b, i));
    }
    return list;
}
