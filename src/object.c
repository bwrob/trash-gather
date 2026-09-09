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
  case ARRAY: {
    free(obj->data.v_array.elements);
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
  case ARRAY: {
    array_t arr = obj->data.v_array;
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

bool array_set(object_t *array, size_t index, object_t *value) {
  if (array == NULL || value == NULL) {
    return false;
  }

  if (array->kind != ARRAY) {
    return false;
  }

  if (index >= array->data.v_array.size) {
    return false;
  }

  if (array->data.v_array.elements[index] != NULL) {
    refcount_dec(array->data.v_array.elements[index]);
  }

  array->data.v_array.elements[index] = value;
  refcount_inc(value);
  return true;
}

object_t *array_get(object_t *array, size_t index) {
  if (array == NULL) {
    return NULL;
  }

  if (array->kind != ARRAY) {
    return NULL;
  }

  if (index >= array->data.v_array.size) {
    return NULL;
  }

  // Get the value directly now (already checked size constraint)
  return array->data.v_array.elements[index];
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
  case ARRAY:
    switch (b->kind) {
    case ARRAY: {
      size_t a_len = a->data.v_array.size;
      size_t b_len = b->data.v_array.size;
      size_t length = a_len + b_len;

      object_t *array = new_array(length);

      for (size_t i = 0; i < a_len; i++) {
        array_set(array, i, array_get(a, i));
      }

      for (size_t i = 0; i < b_len; i++) {
        array_set(array, i + a_len, array_get(b, i));
      }

      return array;
    }
    default:
      return NULL;
    }
  default:
    return NULL;
  }
}
