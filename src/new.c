#include "new.h"

#include "object.h"
#include "vm.h"

#include <stdlib.h>
#include <string.h>

object_t *_new_object() {
  object_t *obj = calloc(1, sizeof(object_t));
  if (obj == NULL) {
    return NULL;
  }

  obj->is_marked = false;
  obj->refcount = 1;
  vm_track_object(obj);

  return obj;
}

object_t *new_list(size_t size) {
  object_t **elements = calloc(size, sizeof(object_t *));
  if (elements == NULL) {
    return NULL;
  }

  object_t *obj = _new_object();
  if (obj == NULL) {
    free(elements);
    return NULL;
  }

  obj->kind = LIST;
  obj->data.v_list = (list_t){.size = size, .elements = elements};

  return obj;
}

object_t *new_vector3(object_t *x, object_t *y, object_t *z) {
  if (x == NULL || y == NULL || z == NULL) {
    return NULL;
  }

  object_t *obj = _new_object();
  if (obj == NULL) {
    return NULL;
  }

  obj->kind = TUPLE;
  obj->data.v_tuple = (tuple_t){.x = x, .y = y, .z = z};
  refcount_inc(x);
  refcount_inc(y);
  refcount_inc(z);

  return obj;
}

object_t *new_integer(int value) {
  object_t *obj = _new_object();
  if (obj == NULL) {
    return NULL;
  }

  obj->kind = INTEGER;
  obj->data.v_int = value;

  return obj;
}

object_t *new_float(float value) {
  object_t *obj = _new_object();
  if (obj == NULL) {
    return NULL;
  }

  obj->kind = FLOAT;
  obj->data.v_float = value;
  return obj;
}

object_t *new_string(char *value) {
  int len = strlen(value);
  char *dst = malloc(len + 1);
  if (dst == NULL) {
    return NULL;
  }

  object_t *obj = _new_object();
  if (obj == NULL) {
    free(dst);
    return NULL;
  }

  strcpy(dst, value);

  obj->kind = STRING;
  obj->data.v_string = dst;
  return obj;
}
