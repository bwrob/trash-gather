#include "bootlib.h"
#include "munit.h"
#include "sneknew.h"
#include "snekobject.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>

munit_case(RUN, test_field_exists, {
  vm_t *vm = vm_new();
  snek_object_t *lane_courses = new_snek_integer(vm, 20);
  snek_object_t *teej_courses = new_snek_integer(vm, 1);
  (void)lane_courses->is_marked;
  (void)teej_courses->is_marked;
  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(SUBMIT, test_marked_is_false, {
  vm_t *vm = vm_new();
  snek_object_t *lane_courses = new_snek_integer(vm, 20);
  snek_object_t *teej_courses = new_snek_integer(vm, 1);
  assert_false(lane_courses->is_marked);
  assert_false(teej_courses->is_marked);
  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(RUN, test_integer_constant,
           { assert_int(INTEGER, ==, 0, "INTEGER is defined as 0"); });

munit_case(RUN, test_integer_obj, {
  snek_object_t *obj = malloc(sizeof(snek_object_t));
  obj->kind = INTEGER;
  obj->data.v_int = 0;
  assert_int(obj->kind, ==, INTEGER, "must be INTEGER type");
  assert_int(obj->data.v_int, ==, 0, "must equal zero");

  free(obj);
});

munit_case(RUN, test_create_empty_array, {
  vm_t *vm = vm_new();
  snek_object_t *obj = new_snek_array(vm, 2);

  assert_int(obj->kind, ==, ARRAY, "Must set type to ARRAY");
  assert_int(obj->data.v_array.size, ==, 2, "Must set size to 2");

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(SUBMIT, test_used_calloc, {
  vm_t *vm = vm_new();
  snek_object_t *obj = new_snek_array(vm, 2);

  assert_ptr_null(obj->data.v_array.elements[0], "Should use calloc");
  assert_ptr_null(obj->data.v_array.elements[1], "Should use calloc");

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(RUN, test_array_set_valid, {
  vm_t *vm = vm_new();
  snek_object_t *arr = new_snek_array(vm, 2);
  snek_object_t *val = new_snek_integer(vm, 100);

  bool ok = snek_array_set(arr, 0, val);
  assert_true(ok);
  assert_ptr_equal(snek_array_get(arr, 0), val);

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(RUN, test_array_set_out_of_bounds, {
  vm_t *vm = vm_new();
  snek_object_t *arr = new_snek_array(vm, 2);
  snek_object_t *val = new_snek_integer(vm, 100);

  bool ok = snek_array_set(arr, 2, val);
  assert_false(ok);

  vm_free(vm);
  assert(boot_all_freed());
});

MunitTest snekobject_tests[] = {
    munit_test("/field_exists", test_field_exists),
    munit_test("/marked_is_false", test_marked_is_false),
    munit_test("/integer_constant", test_integer_constant),
    munit_test("/integer_obj", test_integer_obj),
    munit_test("/create_empty_array", test_create_empty_array),
    munit_test("/used_calloc", test_used_calloc),
    munit_test("/array_set_valid", test_array_set_valid),
    munit_test("/array_set_out_of_bounds", test_array_set_out_of_bounds),
    munit_null_test,
};
