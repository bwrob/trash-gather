#include "bootlib.h"
#include "munit.h"
#include "sneknew.h"
#include "snekobject.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

munit_case(RUN, test_positive_integer, {
  vm_t *vm = vm_new();
  snek_object_t *int_object = new_snek_integer(vm, 42);
  assert_int(int_object->data.v_int, ==, 42, "must allow positive numbers");

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(RUN, test_zero_integer, {
  vm_t *vm = vm_new();
  snek_object_t *int_object = new_snek_integer(vm, 0);

  assert_int(int_object->kind, ==, INTEGER, "must be INTEGER type");
  assert_int(int_object->data.v_int, ==, 0, "must equal zero");

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(SUBMIT, test_negative_integer, {
  vm_t *vm = vm_new();
  snek_object_t *int_object = new_snek_integer(vm, -5);

  assert_int(int_object->kind, ==, INTEGER, "must be INTEGER type");
  assert_int(int_object->data.v_int, ==, -5, "must allow negative numbers");

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(RUN, test_float_object, {
  vm_t *vm = vm_new();
  snek_object_t *float_object = new_snek_float(vm, 3.14f);

  assert_int(float_object->kind, ==, FLOAT, "must be FLOAT type");
  assert_double_equal((double)float_object->data.v_float, 3.14, 2);

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(RUN, test_string_object, {
  vm_t *vm = vm_new();
  snek_object_t *string_object = new_snek_string(vm, "Hello Snek");

  assert_int(string_object->kind, ==, STRING, "must be STRING type");
  assert_string_equal(string_object->data.v_string, "Hello Snek",
                      "must copy string content");

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(RUN, test_vector3_object, {
  vm_t *vm = vm_new();
  snek_object_t *x = new_snek_integer(vm, 1);
  snek_object_t *y = new_snek_integer(vm, 2);
  snek_object_t *z = new_snek_integer(vm, 3);
  snek_object_t *vec = new_snek_vector3(vm, x, y, z);

  assert_int(vec->kind, ==, VECTOR3, "must be VECTOR3 type");
  assert_ptr_equal(vec->data.v_vector3.x, x);
  assert_ptr_equal(vec->data.v_vector3.y, y);
  assert_ptr_equal(vec->data.v_vector3.z, z);

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(RUN, test_vec_returns_null, {
  vm_t *vm = vm_new();
  snek_object_t *vec = new_snek_vector3(vm, NULL, NULL, NULL);

  assert_null(vec, "Should return null when input is null");

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(RUN, test_vec_multiple_objects, {
  vm_t *vm = vm_new();
  snek_object_t *x = new_snek_integer(vm, 1);
  snek_object_t *y = new_snek_integer(vm, 2);
  snek_object_t *z = new_snek_integer(vm, 3);
  snek_object_t *vec = new_snek_vector3(vm, x, y, z);

  assert_ptr_not_null(vec, "should allocate a new object");

  // Vectors should not copy objects, they get the reference to the objects.
  assert_ptr(x, ==, vec->data.v_vector3.x, "should reference x");
  assert_ptr(y, ==, vec->data.v_vector3.y, "should reference y");
  assert_ptr(z, ==, vec->data.v_vector3.z, "should reference z");

  // Assert we have integer values correct
  assert_int(vec->data.v_vector3.x->data.v_int, ==, 1, "should have correct x");
  assert_int(vec->data.v_vector3.y->data.v_int, ==, 2, "should have correct y");
  assert_int(vec->data.v_vector3.z->data.v_int, ==, 3, "should have correct z");

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(SUBMIT, test_vec_same_object, {
  vm_t *vm = vm_new();
  snek_object_t *i = new_snek_integer(vm, 1);
  snek_object_t *vec = new_snek_vector3(vm, i, i, i);

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

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(RUN, test_array_object, {
  vm_t *vm = vm_new();
  snek_object_t *arr = new_snek_array(vm, 5);

  assert_int(arr->kind, ==, ARRAY, "must be ARRAY type");
  assert_size(arr->data.v_array.size, ==, 5, "size must be 5");
  assert_ptr_not_null(arr->data.v_array.elements,
                      "elements array must be allocated");
  assert_ptr_null(arr->data.v_array.elements[0],
                  "elements must be initialized to NULL");

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(RUN, test_array_empty, {
  vm_t *vm = vm_new();
  snek_object_t *arr = new_snek_array(vm, 0);

  assert_int(arr->kind, ==, ARRAY, "must be ARRAY type");
  assert_size(arr->data.v_array.size, ==, 0, "size must be 0");

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(RUN, test_alloc_failures, {
  vm_t *vm = vm_new();

  boot_set_fail_alloc_after(0);
  assert_null(new_snek_integer(vm, 1));

  boot_set_fail_alloc_after(0);
  assert_null(new_snek_float(vm, 1.0f));

  boot_set_fail_alloc_after(0);
  assert_null(new_snek_string(vm, "test"));

  boot_set_fail_alloc_after(1);
  assert_null(new_snek_string(vm, "test"));

  boot_set_fail_alloc_after(0);
  assert_null(new_snek_array(vm, 5));

  boot_set_fail_alloc_after(1);
  assert_null(new_snek_array(vm, 5));

  snek_object_t *x = new_snek_integer(vm, 1);
  snek_object_t *y = new_snek_integer(vm, 2);
  snek_object_t *z = new_snek_integer(vm, 3);
  boot_set_fail_alloc_after(0);
  assert_null(new_snek_vector3(vm, x, y, z));

  vm_free(vm);
  assert(boot_all_freed());
});

MunitTest sneknew_tests[] = {
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
