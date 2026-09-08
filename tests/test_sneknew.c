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

MunitTest sneknew_tests[] = {
    munit_test("/integer_positive", test_positive_integer),
    munit_test("/integer_zero", test_zero_integer),
    munit_test("/integer_negative", test_negative_integer),
    munit_null_test,
};
