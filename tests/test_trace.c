/**
 * @file test_trace.c
 * @brief Unit tests for garbage collector pointer graph tracing phase across
 * vectors, arrays, nested graphs, and unreachable cycles.
 */

#include "bootlib.h"
#include "munit.h"
#include "new.h"
#include "object.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Test pointer graph tracing through 3D vector object references.
 */
munit_case(RUN, test_trace_vector, {
  vm_new();
  frame_t *frame = vm_new_frame();

  object_t *x = new_integer(5);
  object_t *y = new_integer(5);
  object_t *z = new_integer(5);
  object_t *vector = new_vector3(x, y, z);

  // nothing is marked
  assert_false(x->is_marked);
  assert_false(y->is_marked);
  assert_false(z->is_marked);
  assert_false(vector->is_marked);

  // After referencing and marking, the
  // vector should be marked, but not the contents
  frame_reference_object(frame, vector);
  mark();
  assert_true(vector->is_marked);
  assert_false(x->is_marked);
  assert_false(y->is_marked);
  assert_false(z->is_marked);

  // After tracing, the contents should be marked
  trace();
  assert_true(vector->is_marked);
  assert_true(x->is_marked);
  assert_true(y->is_marked);
  assert_true(z->is_marked);

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test pointer graph tracing through array element object references.
 */
munit_case(SUBMIT, test_trace_array, {
  vm_new();
  frame_t *frame = vm_new_frame();

  object_t *devs = new_array(2);
  object_t *lane = new_string("Lane");
  object_t *teej = new_string("Teej");
  array_set(devs, 0, lane);
  array_set(devs, 1, teej);

  // nothing is marked
  assert_false(devs->is_marked);
  assert_false(lane->is_marked);
  assert_false(teej->is_marked);

  // After referencing and marking, the
  // array should be marked, but not the contents
  frame_reference_object(frame, devs);
  mark();
  assert_true(devs->is_marked);
  assert_false(lane->is_marked);
  assert_false(teej->is_marked);

  // After tracing, the contents should be marked
  trace();
  assert_true(devs->is_marked);
  assert_true(lane->is_marked);
  assert_true(teej->is_marked);

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test pointer graph tracing through deeply nested arrays of arrays.
 */
munit_case(SUBMIT, test_trace_nested, {
  vm_new();
  frame_t *frame = vm_new_frame();

  object_t *bootdevs = new_array(2);
  object_t *lane = new_string("Lane");
  object_t *hunter = new_string("Hunter");
  array_set(bootdevs, 0, lane);
  array_set(bootdevs, 1, hunter);

  object_t *terminaldevs = new_array(4);
  object_t *prime = new_string("Prime");
  object_t *teej = new_string("Teej");
  object_t *dax = new_string("Dax");
  object_t *adam = new_string("Adam");
  array_set(terminaldevs, 0, prime);
  array_set(terminaldevs, 1, teej);
  array_set(terminaldevs, 2, dax);
  array_set(terminaldevs, 3, adam);

  object_t *alldevs = new_array(2);
  array_set(alldevs, 0, bootdevs);
  array_set(alldevs, 1, terminaldevs);

  frame_reference_object(frame, alldevs);
  mark();
  trace();

  assert_true(bootdevs->is_marked);
  assert_true(lane->is_marked);
  assert_true(hunter->is_marked);
  assert_true(terminaldevs->is_marked);
  assert_true(prime->is_marked);
  assert_true(teej->is_marked);
  assert_true(dax->is_marked);
  assert_true(adam->is_marked);
  assert_true(alldevs->is_marked);

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test that trace_mark_object skips pushing objects that are already
 * marked.
 */
munit_case(SUBMIT, test_trace_mark_object_already_marked, {
  vm_new();
  vm_stack_t *gray_objects = stack_new(8);
  object_t *obj = new_integer(7);

  assert_not_null(gray_objects, "must allocate gray object stack");
  assert_not_null(obj, "must allocate object");

  obj->is_marked = true;
  trace_mark_object(gray_objects, obj);

  assert_size(gray_objects->count, ==, 0,
              "already-marked objects must not be pushed");

  stack_free(gray_objects);
  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test that unreachable self-referential cyclic structures remain
 * unmarked after tracing.
 */
munit_case(SUBMIT, test_trace_unreachable_cycle, {
  vm_new();
  object_t *unreachable = new_array(1);

  array_set(unreachable, 0, unreachable);

  mark();
  trace();

  assert_false(unreachable->is_marked);

  vm_free();
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
