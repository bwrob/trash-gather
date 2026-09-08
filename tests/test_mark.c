/**
 * @file test_mark.c
 * @brief Unit tests for garbage collector root marking phase across single and
 * multiple stack frames.
 */

#include "bootlib.h"
#include "munit.h"
#include "sneknew.h"
#include "snekobject.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Test root marking behavior for objects referenced within a single
 * stack frame.
 */
munit_case(RUN, test_single_frame, {
  vm_t *vm = vm_new();
  frame_t *frame = vm_new_frame(vm);

  snek_object_t *teej_skill = new_snek_integer(vm, 420);
  snek_object_t *lane_skill = new_snek_string(vm, "issues");

  mark(vm);
  // should not be marked because not in frame
  assert_false(teej_skill->is_marked);
  assert_false(lane_skill->is_marked);

  frame_reference_object(frame, teej_skill);
  frame_reference_object(frame, lane_skill);

  // after adding and marking, should be marked
  mark(vm);
  assert_true(teej_skill->is_marked);
  assert_true(lane_skill->is_marked);

  vm_free(vm);
  assert(boot_all_freed());
});

/**
 * @brief Test root marking behavior across multiple stack frames.
 */
munit_case(SUBMIT, test_multi_frame, {
  vm_t *vm = vm_new();
  frame_t *frame = vm_new_frame(vm);
  frame_t *frame2 = vm_new_frame(vm);

  snek_object_t *teej_skill = new_snek_integer(vm, 420);
  snek_object_t *lane_skill = new_snek_string(vm, "issues");
  snek_object_t *prime_skill = new_snek_string(vm, "infinite");

  frame_reference_object(frame, teej_skill);
  frame_reference_object(frame, lane_skill);
  frame_reference_object(frame2, prime_skill);
  mark(vm);

  assert_true(teej_skill->is_marked);
  assert_true(lane_skill->is_marked);
  assert_true(prime_skill->is_marked);
  vm_free(vm);
  assert(boot_all_freed());
});

MunitTest mark_tests[] = {
    munit_test("/single_frame", test_single_frame),
    munit_test("/multi_frame", test_multi_frame),
    munit_null_test,
};
