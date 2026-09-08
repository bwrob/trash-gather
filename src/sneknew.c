#include "sneknew.h"

#include "snekobject.h"
#include "vm.h"

#include <stdlib.h>
#include <string.h>

snek_object_t *_new_snek_object() {
  snek_object_t *obj = calloc(1, sizeof(snek_object_t));
  if (obj == NULL) {
    return NULL;
  }

  obj->is_marked = false;
  obj->refcount = 1;
  vm_track_object(obj);

  return obj;
}

snek_object_t *new_snek_array(size_t size) {
  snek_object_t **elements = calloc(size, sizeof(snek_object_t *));
  if (elements == NULL) {
    return NULL;
  }

  snek_object_t *obj = _new_snek_object();
  if (obj == NULL) {
    free(elements);
    return NULL;
  }

  obj->kind = ARRAY;
  obj->data.v_array = (snek_array_t){.size = size, .elements = elements};

  return obj;
}

snek_object_t *new_snek_vector3(snek_object_t *x, snek_object_t *y,
                                snek_object_t *z) {
  if (x == NULL || y == NULL || z == NULL) {
    return NULL;
  }

  snek_object_t *obj = _new_snek_object();
  if (obj == NULL) {
    return NULL;
  }

  obj->kind = VECTOR3;
  obj->data.v_vector3 = (snek_vector_t){.x = x, .y = y, .z = z};
  refcount_inc(x);
  refcount_inc(y);
  refcount_inc(z);

  return obj;
}

snek_object_t *new_snek_integer(int value) {
  snek_object_t *obj = _new_snek_object();
  if (obj == NULL) {
    return NULL;
  }

  obj->kind = INTEGER;
  obj->data.v_int = value;

  return obj;
}

snek_object_t *new_snek_float(float value) {
  snek_object_t *obj = _new_snek_object();
  if (obj == NULL) {
    return NULL;
  }

  obj->kind = FLOAT;
  obj->data.v_float = value;
  return obj;
}

snek_object_t *new_snek_string(char *value) {
  int len = strlen(value);
  char *dst = malloc(len + 1);
  if (dst == NULL) {
    return NULL;
  }

  snek_object_t *obj = _new_snek_object();
  if (obj == NULL) {
    free(dst);
    return NULL;
  }

  strcpy(dst, value);

  obj->kind = STRING;
  obj->data.v_string = dst;
  return obj;
}
