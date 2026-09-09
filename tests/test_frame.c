/**
 * @file test_frame.c
 * @brief Unit tests for Virtual Machine stack frame creation and object
 * reference tracking.
 */

#include "bootlib.h"
#include "munit.h"
#include "new.h"
#include "object.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Test creating a new VM stack frame and verifying initial stack
 * allocation.
 */
munit_case(RUN, test_vm_new_frame, {
  vm_new();
  frame_t *frame = vm_new_frame();
  assert_ptr(frame->references, !=, NULL, "frame->references must be allocated");
  assert_int(frame->references->count, ==, 0, "references stack should start empty");
  assert(frame->references->capacity > 0); // references stack must have capacity > 0
  assert_ptr(frame->references->data, !=, NULL,
             "references stack backing list must be allocated");
  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test referencing a single object within a stack frame.
 */
munit_case(RUN, test_one_ref, {
  vm_new();
  frame_t *frame = vm_new_frame();

  object_t *lanes_wpm = new_integer(9);
  frame_reference_object(frame, lanes_wpm);

  assert_int(frame->references->count, ==, 1, "Only one reference");
  assert_ptr_equal(lanes_wpm, frame->references->data[0], "Refs lanes_wpm");

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test referencing multiple distinct objects within a stack frame.
 */
munit_case(SUBMIT, test_multi_ref, {
  vm_new();
  frame_t *frame = vm_new_frame();

  object_t *lanes_wpm = new_integer(9);
  object_t *teej_wpm = new_integer(160);
  frame_reference_object(frame, lanes_wpm);
  frame_reference_object(frame, teej_wpm);

  assert_int(frame->references->count, ==, 2, "Two references");
  assert_ptr_equal(lanes_wpm, frame->references->data[0], "Refs lanes_wpm");
  assert_ptr_equal(teej_wpm, frame->references->data[1], "Refs teej_wpm");

  vm_free();
  assert(boot_all_freed());
});

MunitTest frame_tests[] = {
    munit_test("/vm_new_frame", test_vm_new_frame),
    munit_test("/one_ref", test_one_ref),
    munit_test("/multi_ref", test_multi_ref),
    munit_null_test,
};
