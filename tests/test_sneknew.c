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
  assert_string_equal(string_object->data.v_string, "Hello Snek", "must copy string content");

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

munit_case(RUN, test_vector3_null, {
  vm_t *vm = vm_new();
  snek_object_t *x = new_snek_integer(vm, 1);
  snek_object_t *vec = new_snek_vector3(vm, x, NULL, NULL);

  assert_ptr_null(vec, "must return NULL if any component is NULL");

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
    munit_test("/vector3_null", test_vector3_null),
    munit_null_test,
};
