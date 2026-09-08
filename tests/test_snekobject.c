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

munit_case(RUN, test_array_set, {
  vm_t *vm = vm_new();
  snek_object_t *obj = new_snek_array(vm, 2);
  snek_object_t *first = new_snek_string(vm, "First");
  snek_object_t *second = new_snek_integer(vm, 3);

  assert(snek_array_set(obj, 0, first));
  assert(snek_array_set(obj, 1, second));

  assert_ptr(obj->data.v_array.elements[0], ==, first,
             "Should set the first element");
  assert_ptr(obj->data.v_array.elements[1], ==, second,
             "Should set the second element");

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(RUN, test_array_set_outside_bounds, {
  vm_t *vm = vm_new();
  snek_object_t *obj = new_snek_array(vm, 2);
  snek_object_t *outside = new_snek_string(vm, "First");

  assert(snek_array_set(obj, 1, outside));
  assert_false(snek_array_set(obj, 2, outside));
  assert_false(snek_array_set(obj, 100, outside));
  assert_ptr(obj->data.v_array.elements[1], ==, outside,
             "Should preserve existing elements");

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(SUBMIT, test_array_set_rejects_invalid_inputs, {
  vm_t *vm = vm_new();
  snek_object_t *array = new_snek_array(vm, 1);
  snek_object_t *value = new_snek_integer(vm, 3);
  snek_object_t *not_array = new_snek_integer(vm, 5);

  assert_false(snek_array_set(NULL, 0, value));
  assert_false(snek_array_set(array, 0, NULL));
  assert_false(snek_array_set(not_array, 0, value));

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(RUN, test_array_get, {
  vm_t *vm = vm_new();
  snek_object_t *obj = new_snek_array(vm, 2);
  snek_object_t *first = new_snek_string(vm, "First");
  snek_object_t *second = new_snek_integer(vm, 3);

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

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(RUN, test_array_get_empty_slot, {
  vm_t *vm = vm_new();
  snek_object_t *obj = new_snek_array(vm, 2);

  assert_null(snek_array_get(obj, 1), "Empty array slots should be NULL");

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(SUBMIT, test_array_get_outside_bounds, {
  vm_t *vm = vm_new();
  snek_object_t *obj = new_snek_array(vm, 1);
  snek_object_t *first = new_snek_string(vm, "First");
  assert(snek_array_set(obj, 0, first));

  assert_null(snek_array_get(obj, 1), "Should not access outside the array");

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(SUBMIT, test_array_get_rejects_invalid_inputs, {
  vm_t *vm = vm_new();
  snek_object_t *not_array = new_snek_integer(vm, 5);

  assert_null(snek_array_get(NULL, 0), "Should reject NULL input");
  assert_null(snek_array_get(not_array, 0), "Should reject non-array input");

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
    munit_test("/array_set", test_array_set),
    munit_test("/array_set_outside", test_array_set_outside_bounds),
    munit_test("/array_set_invalid", test_array_set_rejects_invalid_inputs),
    munit_test("/array_get", test_array_get),
    munit_test("/array_get_empty", test_array_get_empty_slot),
    munit_test("/array_get_outside", test_array_get_outside_bounds),
    munit_test("/array_get_invalid", test_array_get_rejects_invalid_inputs),
    munit_null_test,
};
