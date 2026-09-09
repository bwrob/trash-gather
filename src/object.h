#pragma once

#include "stack.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct Object object_t;

typedef struct {
  size_t size;
  object_t **elements;
} array_t;

typedef struct {
  object_t *x;
  object_t *y;
  object_t *z;
} tuple_t;

typedef enum ObjectKind {
  INTEGER,
  FLOAT,
  STRING,
  TUPLE,
  ARRAY,
} object_kind_t;

typedef union ObjectData {
  int v_int;
  float v_float;
  char *v_string;
  tuple_t v_tuple;
  array_t v_array;
} object_data_t;

struct Object {
  bool is_marked;
  size_t refcount;
  size_t tracker_id;

  object_kind_t kind;
  object_data_t data;
};

void refcount_inc(object_t *obj);
void refcount_dec(object_t *obj);
void object_decref_children(object_t *obj, bool live_only);

void object_free_payload(object_t *obj);
void object_free(object_t *obj);
void _refcount_dec(object_t *obj, bool live_only);

bool array_set(object_t *array, size_t index, object_t *value);
object_t *array_get(object_t *array, size_t index);
object_t *add(object_t *a, object_t *b);
