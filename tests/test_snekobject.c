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

MunitTest snekobject_tests[] = {
    munit_test("/field_exists", test_field_exists),
    munit_test("/marked_is_false", test_marked_is_false),
    munit_null_test,
};
