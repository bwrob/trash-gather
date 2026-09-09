#include "object.h"

#include "new.h"
#include "vm.h"

#include <string.h>

void refcount_inc(object_t *obj) {
  if (obj == NULL) {
    return;
  }

  obj->refcount++;
  return;
}

void refcount_dec(object_t *obj) {
  if (obj == NULL) {
    return;
  }
  obj->refcount--;
  if (obj->refcount == 0) {
    object_free(obj);
    return;
  }
  return;
}

void object_free_payload(object_t *obj) {
  switch (obj->kind) {
  case INTEGER:
  case FLOAT:
  case TUPLE:
    break;
  case STRING: {
    free(obj->data.v_string);
    break;
  }
  case LIST: {
    free(obj->data.v_list.elements);
    break;
  }
  }
}

void _refcount_dec(object_t *obj, bool live_only) {
  if (obj == NULL) {
    return;
  }
  if (!live_only || obj->is_marked) {
    refcount_dec(obj);
  }
}

void object_decref_children(object_t *obj, bool live_only) {
  switch (obj->kind) {
  case INTEGER:
  case FLOAT:
  case STRING:
    break;
  case TUPLE: {
    tuple_t tuple = obj->data.v_tuple;
    _refcount_dec(tuple.x, live_only);
    _refcount_dec(tuple.y, live_only);
    _refcount_dec(tuple.z, live_only);
    break;
  }
  case LIST: {
    list_t arr = obj->data.v_list;
    for (size_t i = 0; i < arr.size; i++) {
      _refcount_dec(arr.elements[i], live_only);
    }
    break;
  }
  }
}

void object_free(object_t *obj) {
  bool live_only = false;
  object_decref_children(obj, live_only);
  object_free_payload(obj);
  vm_untrack_object(obj);
  free(obj);
}

bool list_set(object_t *list, size_t index, object_t *value) {
  if (list == NULL || value == NULL) {
    return false;
  }

  if (list->kind != LIST) {
    return false;
  }

  if (index >= list->data.v_list.size) {
    return false;
  }

  if (list->data.v_list.elements[index] != NULL) {
    refcount_dec(list->data.v_list.elements[index]);
  }

  list->data.v_list.elements[index] = value;
  refcount_inc(value);
  return true;
}

object_t *list_get(object_t *list, size_t index) {
  if (list == NULL) {
    return NULL;
  }

  if (list->kind != LIST) {
    return NULL;
  }

  if (index >= list->data.v_list.size) {
    return NULL;
  }

  // Get the value directly now (already checked size constraint)
  return list->data.v_list.elements[index];
}

object_t *add(object_t *a, object_t *b) {
  if (a == NULL || b == NULL) {
    return NULL;
  }

  switch (a->kind) {
  case INTEGER:
    switch (b->kind) {
    case INTEGER:
      return new_integer(a->data.v_int + b->data.v_int);
    case FLOAT:
      return new_float((float)a->data.v_int + b->data.v_float);
    default:
      return NULL;
    }
  case FLOAT:
    switch (b->kind) {
    case FLOAT:
      return new_float(a->data.v_float + b->data.v_float);
    default:
      return add(b, a);
    }
  case STRING:
    switch (b->kind) {
    case STRING: {
      int a_len = strlen(a->data.v_string);
      int b_len = strlen(b->data.v_string);
      int len = a_len + b_len + 1;
      char *dst = malloc(len * sizeof(char));
      dst[0] = '\0';

      strcat(dst, a->data.v_string);
      strcat(dst, b->data.v_string);

      object_t *obj = new_string(dst);
      free(dst);

      return obj;
    }
    default:
      return NULL;
    }
  case TUPLE:
    switch (b->kind) {
    case TUPLE:
      return new_vector3(add(a->data.v_tuple.x, b->data.v_tuple.x),
                         add(a->data.v_tuple.y, b->data.v_tuple.y),
                         add(a->data.v_tuple.z, b->data.v_tuple.z));
    default:
      return NULL;
    }
  case LIST:
    switch (b->kind) {
    case LIST: {
      size_t a_len = a->data.v_list.size;
      size_t b_len = b->data.v_list.size;
      size_t length = a_len + b_len;

      object_t *list = new_list(length);

      for (size_t i = 0; i < a_len; i++) {
        list_set(list, i, list_get(a, i));
      }

      for (size_t i = 0; i < b_len; i++) {
        list_set(list, i + a_len, list_get(b, i));
      }

      return list;
    }
    default:
      return NULL;
    }
  default:
    return NULL;
  }
}
