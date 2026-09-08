/**
 * @file test_new.c
 * @brief Unit tests for object allocation constructors (integers, floats,
 * strings, vectors, arrays) and failure injection.
 */

#include "bootlib.h"
#include "munit.h"
#include "new.h"
#include "object.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Test allocating positive integer objects.
 */
munit_case(RUN, test_positive_integer, {
  vm_new();
  object_t *int_object = new_integer(42);
  assert_int(int_object->data.v_int, ==, 42, "must allow positive numbers");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test allocating zero integer objects.
 */
munit_case(RUN, test_zero_integer, {
  vm_new();
  object_t *int_object = new_integer(0);

  assert_int(int_object->kind, ==, INTEGER, "must be INTEGER type");
  assert_int(int_object->data.v_int, ==, 0, "must equal zero");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test allocating negative integer objects.
 */
munit_case(SUBMIT, test_negative_integer, {
  vm_new();
  object_t *int_object = new_integer(-5);

  assert_int(int_object->kind, ==, INTEGER, "must be INTEGER type");
  assert_int(int_object->data.v_int, ==, -5, "must allow negative numbers");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test allocating floating-point objects.
 */
munit_case(RUN, test_float_object, {
  vm_new();
  object_t *float_object = new_float(3.14f);

  assert_int(float_object->kind, ==, FLOAT, "must be FLOAT type");
  assert_double_equal((double)float_object->data.v_float, 3.14, 2);

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test allocating copied string objects.
 */
munit_case(RUN, test_string_object, {
  vm_new();
  object_t *string_object = new_string("Hello ");

  assert_int(string_object->kind, ==, STRING, "must be STRING type");
  assert_string_equal(string_object->data.v_string, "Hello ",
                      "must copy string content");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test allocating 3D vector objects referencing component objects.
 */
munit_case(RUN, test_vector3_object, {
  vm_new();
  object_t *x = new_integer(1);
  object_t *y = new_integer(2);
  object_t *z = new_integer(3);
  object_t *vec = new_vector3(x, y, z);

  assert_int(vec->kind, ==, VECTOR3, "must be VECTOR3 type");
  assert_ptr_equal(vec->data.v_vector3.x, x);
  assert_ptr_equal(vec->data.v_vector3.y, y);
  assert_ptr_equal(vec->data.v_vector3.z, z);

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test vector allocation safety when passed NULL component references.
 */
munit_case(RUN, test_vec_returns_null, {
  vm_new();
  object_t *vec = new_vector3(NULL, NULL, NULL);

  assert_null(vec, "Should return null when input is null");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test vector object reference identity across multiple objects.
 */
munit_case(RUN, test_vec_multiple_objects, {
  vm_new();
  object_t *x = new_integer(1);
  object_t *y = new_integer(2);
  object_t *z = new_integer(3);
  object_t *vec = new_vector3(x, y, z);

  assert_ptr_not_null(vec, "should allocate a new object");

  // Vectors should not copy objects, they get the reference to the objects.
  assert_ptr(x, ==, vec->data.v_vector3.x, "should reference x");
  assert_ptr(y, ==, vec->data.v_vector3.y, "should reference y");
  assert_ptr(z, ==, vec->data.v_vector3.z, "should reference z");

  // Assert we have integer values correct
  assert_int(vec->data.v_vector3.x->data.v_int, ==, 1, "should have correct x");
  assert_int(vec->data.v_vector3.y->data.v_int, ==, 2, "should have correct y");
  assert_int(vec->data.v_vector3.z->data.v_int, ==, 3, "should have correct z");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test vector object sharing identical reference across dimensions.
 */
munit_case(SUBMIT, test_vec_same_object, {
  vm_new();
  object_t *i = new_integer(1);
  object_t *vec = new_vector3(i, i, i);

  assert_ptr_not_null(vec, "should allocate a new object");

  // Vectors should not copy objects, they get the reference to the objects.
  assert_ptr(i, ==, vec->data.v_vector3.x, "should reference x");
  assert_ptr(i, ==, vec->data.v_vector3.y, "should reference y");
  assert_ptr(i, ==, vec->data.v_vector3.z, "should reference z");

  // Assert we have integer values correct
  assert_int(vec->data.v_vector3.x->data.v_int, ==, 1, "should have correct x");
  assert_int(vec->data.v_vector3.y->data.v_int, ==, 1, "should have correct y");
  assert_int(vec->data.v_vector3.z->data.v_int, ==, 1, "should have correct z");

  i->data.v_int = 2;

  // Assert we have integer values correct, after update
  assert_int(vec->data.v_vector3.x->data.v_int, ==, 2, "should have correct x");
  assert_int(vec->data.v_vector3.y->data.v_int, ==, 2, "should have correct y");
  assert_int(vec->data.v_vector3.z->data.v_int, ==, 2, "should have correct z");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test allocating non-empty array objects.
 */
munit_case(RUN, test_array_object, {
  vm_new();
  object_t *arr = new_array(5);

  assert_int(arr->kind, ==, ARRAY, "must be ARRAY type");
  assert_size(arr->data.v_array.size, ==, 5, "size must be 5");
  assert_ptr_not_null(arr->data.v_array.elements,
                      "elements array must be allocated");
  assert_ptr_null(arr->data.v_array.elements[0],
                  "elements must be initialized to NULL");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test allocating zero-sized empty array objects.
 */
munit_case(RUN, test_array_empty, {
  vm_new();
  object_t *arr = new_array(0);

  assert_int(arr->kind, ==, ARRAY, "must be ARRAY type");
  assert_size(arr->data.v_array.size, ==, 0, "size must be 0");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test memory allocation failure simulation across object constructors.
 */
munit_case(RUN, test_alloc_failures, {
  vm_new();

  boot_set_fail_alloc_after(0);
  assert_null(new_integer(1));

  boot_set_fail_alloc_after(0);
  assert_null(new_float(1.0f));

  boot_set_fail_alloc_after(0);
  assert_null(new_string("test"));

  boot_set_fail_alloc_after(1);
  assert_null(new_string("test"));

  boot_set_fail_alloc_after(0);
  assert_null(new_array(5));

  boot_set_fail_alloc_after(1);
  assert_null(new_array(5));

  object_t *x = new_integer(1);
  object_t *y = new_integer(2);
  object_t *z = new_integer(3);
  boot_set_fail_alloc_after(0);
  assert_null(new_vector3(x, y, z));

  vm_free();
  assert(boot_all_freed());
});

MunitTest new_tests[] = {
    munit_test("/integer_positive", test_positive_integer),
    munit_test("/integer_zero", test_zero_integer),
    munit_test("/integer_negative", test_negative_integer),
    munit_test("/float_object", test_float_object),
    munit_test("/string_object", test_string_object),
    munit_test("/vector3_object", test_vector3_object),
    munit_test("/vector3_returns_null", test_vec_returns_null),
    munit_test("/vector3_multiple_objects", test_vec_multiple_objects),
    munit_test("/vector3_same_object", test_vec_same_object),
    munit_test("/array_object", test_array_object),
    munit_test("/array_empty", test_array_empty),
    munit_test("/alloc_failures", test_alloc_failures),
    munit_null_test,
};
