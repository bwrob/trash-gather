#include "bootlib.h"
#include "munit.h"
#include "sneknew.h"
#include "snekobject.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

munit_case(RUN, test_trace_vector, {
  vm_t *vm = vm_new();
  frame_t *frame = vm_new_frame(vm);

  snek_object_t *x = new_snek_integer(vm, 5);
  snek_object_t *y = new_snek_integer(vm, 5);
  snek_object_t *z = new_snek_integer(vm, 5);
  snek_object_t *vector = new_snek_vector3(vm, x, y, z);

  // nothing is marked
  assert_false(x->is_marked);
  assert_false(y->is_marked);
  assert_false(z->is_marked);
  assert_false(vector->is_marked);

  // After referencing and marking, the
  // vector should be marked, but not the contents
  frame_reference_object(frame, vector);
  mark(vm);
  assert_true(vector->is_marked);
  assert_false(x->is_marked);
  assert_false(y->is_marked);
  assert_false(z->is_marked);

  // After tracing, the contents should be marked
  trace(vm);
  assert_true(vector->is_marked);
  assert_true(x->is_marked);
  assert_true(y->is_marked);
  assert_true(z->is_marked);

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(SUBMIT, test_trace_array, {
  vm_t *vm = vm_new();
  frame_t *frame = vm_new_frame(vm);

  snek_object_t *devs = new_snek_array(vm, 2);
  snek_object_t *lane = new_snek_string(vm, "Lane");
  snek_object_t *teej = new_snek_string(vm, "Teej");
  snek_array_set(devs, 0, lane);
  snek_array_set(devs, 1, teej);

  // nothing is marked
  assert_false(devs->is_marked);
  assert_false(lane->is_marked);
  assert_false(teej->is_marked);

  // After referencing and marking, the
  // array should be marked, but not the contents
  frame_reference_object(frame, devs);
  mark(vm);
  assert_true(devs->is_marked);
  assert_false(lane->is_marked);
  assert_false(teej->is_marked);

  // After tracing, the contents should be marked
  trace(vm);
  assert_true(devs->is_marked);
  assert_true(lane->is_marked);
  assert_true(teej->is_marked);

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(SUBMIT, test_trace_nested, {
  vm_t *vm = vm_new();
  frame_t *frame = vm_new_frame(vm);

  snek_object_t *bootdevs = new_snek_array(vm, 2);
  snek_object_t *lane = new_snek_string(vm, "Lane");
  snek_object_t *hunter = new_snek_string(vm, "Hunter");
  snek_array_set(bootdevs, 0, lane);
  snek_array_set(bootdevs, 1, hunter);

  snek_object_t *terminaldevs = new_snek_array(vm, 4);
  snek_object_t *prime = new_snek_string(vm, "Prime");
  snek_object_t *teej = new_snek_string(vm, "Teej");
  snek_object_t *dax = new_snek_string(vm, "Dax");
  snek_object_t *adam = new_snek_string(vm, "Adam");
  snek_array_set(terminaldevs, 0, prime);
  snek_array_set(terminaldevs, 1, teej);
  snek_array_set(terminaldevs, 2, dax);
  snek_array_set(terminaldevs, 3, adam);

  snek_object_t *alldevs = new_snek_array(vm, 2);
  snek_array_set(alldevs, 0, bootdevs);
  snek_array_set(alldevs, 1, terminaldevs);

  frame_reference_object(frame, alldevs);
  mark(vm);
  trace(vm);

  assert_true(bootdevs->is_marked);
  assert_true(lane->is_marked);
  assert_true(hunter->is_marked);
  assert_true(terminaldevs->is_marked);
  assert_true(prime->is_marked);
  assert_true(teej->is_marked);
  assert_true(dax->is_marked);
  assert_true(adam->is_marked);
  assert_true(alldevs->is_marked);

  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(SUBMIT, test_trace_mark_object_already_marked, {
  vm_t *vm = vm_new();
  stack_t *gray_objects = stack_new(8);
  snek_object_t *obj = new_snek_integer(vm, 7);

  assert_not_null(gray_objects, "must allocate gray object stack");
  assert_not_null(obj, "must allocate object");

  obj->is_marked = true;
  trace_mark_object(gray_objects, obj);

  assert_size(gray_objects->count, ==, 0,
              "already-marked objects must not be pushed");

  stack_free(gray_objects);
  vm_free(vm);
  assert(boot_all_freed());
});

munit_case(SUBMIT, test_trace_unreachable_cycle, {
  vm_t *vm = vm_new();
  snek_object_t *unreachable = new_snek_array(vm, 1);

  snek_array_set(unreachable, 0, unreachable);

  mark(vm);
  trace(vm);

  assert_false(unreachable->is_marked);

  vm_free(vm);
  assert(boot_all_freed());
});

MunitTest trace_tests[] = {
    munit_test("/vector", test_trace_vector),
    munit_test("/array", test_trace_array),
    munit_test("/nested", test_trace_nested),
    munit_test("/mark_already_marked", test_trace_mark_object_already_marked),
    munit_test("/unreachable_cycle", test_trace_unreachable_cycle),
    munit_null_test,
};
