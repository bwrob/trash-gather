/**
 * @file test_snekobject.c
 * @brief Unit tests for snekobject field properties, array bounds/mutation, and
 * polymorphic addition operations.
 */

#include "bootlib.h"
#include "munit.h"
#include "sneknew.h"
#include "snekobject.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Test existence of marking flags on allocated objects.
 */
munit_case(RUN, test_field_exists, {
  vm_new();
  snek_object_t *lane_courses = new_snek_integer(20);
  snek_object_t *teej_courses = new_snek_integer(1);
  (void)lane_courses->is_marked;
  (void)teej_courses->is_marked;
  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test that newly allocated objects are unmarked by default.
 */
munit_case(SUBMIT, test_marked_is_false, {
  vm_new();
  snek_object_t *lane_courses = new_snek_integer(20);
  snek_object_t *teej_courses = new_snek_integer(1);
  assert_false(lane_courses->is_marked);
  assert_false(teej_courses->is_marked);
  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test integer type enum constant definition.
 */
munit_case(RUN, test_integer_constant,
           { assert_int(INTEGER, ==, 0, "INTEGER is defined as 0"); });

/**
 * @brief Test raw integer object structure initialization.
 */
munit_case(RUN, test_integer_obj, {
  snek_object_t *obj = malloc(sizeof(snek_object_t));
  obj->kind = INTEGER;
  obj->data.v_int = 0;
  assert_int(obj->kind, ==, INTEGER, "must be INTEGER type");
  assert_int(obj->data.v_int, ==, 0, "must equal zero");

  free(obj);
});

/**
 * @brief Test array object creation with specified element capacity.
 */
munit_case(RUN, test_create_empty_array, {
  vm_new();
  snek_object_t *obj = new_snek_array(2);

  assert_int(obj->kind, ==, ARRAY, "Must set type to ARRAY");
  assert_int(obj->data.v_array.size, ==, 2, "Must set size to 2");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test zero-initialization of allocated array slots.
 */
munit_case(SUBMIT, test_used_calloc, {
  vm_new();
  snek_object_t *obj = new_snek_array(2);

  assert_ptr_null(obj->data.v_array.elements[0], "Should use calloc");
  assert_ptr_null(obj->data.v_array.elements[1], "Should use calloc");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test setting elements within valid array index bounds.
 */
munit_case(RUN, test_array_set, {
  vm_new();
  snek_object_t *obj = new_snek_array(2);
  snek_object_t *first = new_snek_string("First");
  snek_object_t *second = new_snek_integer(3);

  assert(snek_array_set(obj, 0, first));
  assert(snek_array_set(obj, 1, second));

  assert_ptr(obj->data.v_array.elements[0], ==, first,
             "Should set the first element");
  assert_ptr(obj->data.v_array.elements[1], ==, second,
             "Should set the second element");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test array set rejection for out-of-bounds indices.
 */
munit_case(RUN, test_array_set_outside_bounds, {
  vm_new();
  snek_object_t *obj = new_snek_array(2);
  snek_object_t *outside = new_snek_string("First");

  assert(snek_array_set(obj, 1, outside));
  assert_false(snek_array_set(obj, 2, outside));
  assert_false(snek_array_set(obj, 100, outside));
  assert_ptr(obj->data.v_array.elements[1], ==, outside,
             "Should preserve existing elements");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test array set error handling for NULL pointers or non-array inputs.
 */
munit_case(SUBMIT, test_array_set_rejects_invalid_inputs, {
  vm_new();
  snek_object_t *array = new_snek_array(1);
  snek_object_t *value = new_snek_integer(3);
  snek_object_t *not_array = new_snek_integer(5);

  assert_false(snek_array_set(NULL, 0, value));
  assert_false(snek_array_set(array, 0, NULL));
  assert_false(snek_array_set(not_array, 0, value));

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test retrieving elements from populated array slots.
 */
munit_case(RUN, test_array_get, {
  vm_new();
  snek_object_t *obj = new_snek_array(2);
  snek_object_t *first = new_snek_string("First");
  snek_object_t *second = new_snek_integer(3);

  assert(snek_array_set(obj, 0, first));
  assert(snek_array_set(obj, 1, second));

  snek_object_t *retrieved_first = snek_array_get(obj, 0);
  assert_not_null(retrieved_first, "Should find the first object");
  assert_int(retrieved_first->kind, ==, STRING, "Should be a string");
  assert_ptr(first, ==, retrieved_first, "Should be the same object");

  snek_object_t *retrieved_second = snek_array_get(obj, 1);
  assert_not_null(retrieved_second, "Should find the second object");
  assert_int(retrieved_second->kind, ==, INTEGER, "Should be an integer");
  assert_ptr(second, ==, retrieved_second, "Should be the same object");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test retrieving elements from uninitialized empty array slots.
 */
munit_case(RUN, test_array_get_empty_slot, {
  vm_new();
  snek_object_t *obj = new_snek_array(2);

  assert_null(snek_array_get(obj, 1), "Empty array slots should be NULL");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test array get rejection for out-of-bounds indices.
 */
munit_case(SUBMIT, test_array_get_outside_bounds, {
  vm_new();
  snek_object_t *obj = new_snek_array(1);
  snek_object_t *first = new_snek_string("First");
  assert(snek_array_set(obj, 0, first));

  assert_null(snek_array_get(obj, 1), "Should not access outside the array");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test array get rejection for NULL or non-array inputs.
 */
munit_case(SUBMIT, test_array_get_rejects_invalid_inputs, {
  vm_new();
  snek_object_t *not_array = new_snek_integer(5);

  assert_null(snek_array_get(NULL, 0), "Should reject NULL input");
  assert_null(snek_array_get(not_array, 0), "Should reject non-array input");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test adding integer objects together.
 */
munit_case(RUN, test_add_integers, {
  vm_new();
  snek_object_t *a = new_snek_integer(10);
  snek_object_t *b = new_snek_integer(20);
  snek_object_t *res = snek_add(a, b);

  assert_not_null(res);
  assert_int(res->kind, ==, INTEGER);
  assert_int(res->data.v_int, ==, 30);

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test polymorphic addition promoting integer and float operands to
 * float.
 */
munit_case(RUN, test_add_integer_and_float, {
  vm_new();
  snek_object_t *a = new_snek_integer(5);
  snek_object_t *b = new_snek_float(2.5f);
  snek_object_t *res1 = snek_add(a, b);
  snek_object_t *res2 = snek_add(b, a);

  assert_not_null(res1);
  assert_int(res1->kind, ==, FLOAT);
  assert_float(res1->data.v_float, ==, 7.5f);

  assert_not_null(res2);
  assert_int(res2->kind, ==, FLOAT);
  assert_float(res2->data.v_float, ==, 7.5f);

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test adding float objects together.
 */
munit_case(RUN, test_add_floats, {
  vm_new();
  snek_object_t *a = new_snek_float(1.5f);
  snek_object_t *b = new_snek_float(2.5f);
  snek_object_t *res = snek_add(a, b);

  assert_not_null(res);
  assert_int(res->kind, ==, FLOAT);
  assert_float(res->data.v_float, ==, 4.0f);

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test concatenating string objects together.
 */
munit_case(RUN, test_add_strings, {
  vm_new();
  snek_object_t *a = new_snek_string("Hello ");
  snek_object_t *b = new_snek_string("World!");
  snek_object_t *res = snek_add(a, b);

  assert_not_null(res);
  assert_int(res->kind, ==, STRING);
  assert_string_equal(res->data.v_string, "Hello World!");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test component-wise addition of 3D vector objects.
 */
munit_case(RUN, test_add_vectors, {
  vm_new();
  snek_object_t *x1 = new_snek_integer(1);
  snek_object_t *y1 = new_snek_integer(2);
  snek_object_t *z1 = new_snek_integer(3);
  snek_object_t *v1 = new_snek_vector3(x1, y1, z1);

  snek_object_t *x2 = new_snek_integer(4);
  snek_object_t *y2 = new_snek_integer(5);
  snek_object_t *z2 = new_snek_integer(6);
  snek_object_t *v2 = new_snek_vector3(x2, y2, z2);

  snek_object_t *res = snek_add(v1, v2);

  assert_not_null(res);
  assert_int(res->kind, ==, VECTOR3);
  assert_int(res->data.v_vector3.x->data.v_int, ==, 5);
  assert_int(res->data.v_vector3.y->data.v_int, ==, 7);
  assert_int(res->data.v_vector3.z->data.v_int, ==, 9);

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test array concatenation via addition operator.
 */
munit_case(RUN, test_add_arrays, {
  vm_new();
  snek_object_t *arr1 = new_snek_array(2);
  snek_object_t *elem1 = new_snek_integer(10);
  snek_object_t *elem2 = new_snek_integer(20);
  snek_array_set(arr1, 0, elem1);
  snek_array_set(arr1, 1, elem2);

  snek_object_t *arr2 = new_snek_array(1);
  snek_object_t *elem3 = new_snek_integer(30);
  snek_array_set(arr2, 0, elem3);

  snek_object_t *res = snek_add(arr1, arr2);

  assert_not_null(res);
  assert_int(res->kind, ==, ARRAY);
  assert_size(res->data.v_array.size, ==, 3);
  assert_int(snek_array_get(res, 0)->data.v_int, ==, 10);
  assert_int(snek_array_get(res, 1)->data.v_int, ==, 20);
  assert_int(snek_array_get(res, 2)->data.v_int, ==, 30);

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test addition rejection for invalid or mismatched object types.
 */
munit_case(RUN, test_add_invalid_mismatched, {
  vm_new();
  snek_object_t *i = new_snek_integer(1);
  snek_object_t *f = new_snek_float(1.0f);
  snek_object_t *s = new_snek_string("hi");
  snek_object_t *v = new_snek_vector3(i, i, i);
  snek_object_t *a = new_snek_array(1);

  assert_null(snek_add(NULL, i));
  assert_null(snek_add(i, NULL));
  assert_null(snek_add(i, s));
  assert_null(snek_add(f, s));
  assert_null(snek_add(s, i));
  assert_null(snek_add(v, i));
  assert_null(snek_add(a, i));

  snek_object_t invalid_obj = {.kind = (snek_object_kind_t)999};
  assert_null(snek_add(&invalid_obj, i));

  vm_free();
  assert(boot_all_freed());
});

MunitTest snekobject_tests[] = {
    munit_test("/field_exists", test_field_exists),
    munit_test("/marked_is_false", test_marked_is_false),
    munit_test("/integer_constant", test_integer_constant),
    munit_test("/integer_obj", test_integer_obj),
    munit_test("/create_empty_array", test_create_empty_array),
    munit_test("/used_calloc", test_used_calloc),
    munit_test("/array_set", test_array_set),
    munit_test("/array_set_outside", test_array_set_outside_bounds),
    munit_test("/array_set_invalid", test_array_set_rejects_invalid_inputs),
    munit_test("/array_get", test_array_get),
    munit_test("/array_get_empty", test_array_get_empty_slot),
    munit_test("/array_get_outside", test_array_get_outside_bounds),
    munit_test("/array_get_invalid", test_array_get_rejects_invalid_inputs),
    munit_test("/add_integers", test_add_integers),
    munit_test("/add_integer_and_float", test_add_integer_and_float),
    munit_test("/add_floats", test_add_floats),
    munit_test("/add_strings", test_add_strings),
    munit_test("/add_vectors", test_add_vectors),
    munit_test("/add_arrays", test_add_arrays),
    munit_test("/add_invalid_mismatched", test_add_invalid_mismatched),
    munit_null_test,
};
