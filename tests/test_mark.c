/**
 * @file test_mark.c
 * @brief Unit tests for garbage collector root marking phase across single and
 * multiple stack frames.
 */

#include "bootlib.h"
#include "munit.h"
#include "new.h"
#include "object.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Test root marking behavior for objects referenced within a single
 * stack frame.
 */
munit_case(
    RUN,
    test_single_frame,
    {
        vm_new();
        frame_t *frame = vm_new_frame();

        object_t *teej_skill = new_integer(420);
        object_t *lane_skill = new_string("issues");

        mark();
        // should not be marked because not in frame
        assert_false(teej_skill->is_marked);
        assert_false(lane_skill->is_marked);

        frame_reference_object(frame, teej_skill);
        frame_reference_object(frame, lane_skill);

        // after adding and marking, should be marked
        mark();
        assert_true(teej_skill->is_marked);
        assert_true(lane_skill->is_marked);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test root marking behavior across multiple stack frames.
 */
munit_case(
    SUBMIT,
    test_multi_frame,
    {
        vm_new();
        frame_t *frame = vm_new_frame();
        frame_t *frame2 = vm_new_frame();

        object_t *teej_skill = new_integer(420);
        object_t *lane_skill = new_string("issues");
        object_t *prime_skill = new_string("infinite");

        frame_reference_object(frame, teej_skill);
        frame_reference_object(frame, lane_skill);
        frame_reference_object(frame2, prime_skill);
        mark();

        assert_true(teej_skill->is_marked);
        assert_true(lane_skill->is_marked);
        assert_true(prime_skill->is_marked);
        vm_free();
        assert(boot_all_freed());
    }
);

MunitTest mark_tests[] = {
    munit_test("/single_frame", test_single_frame),
    munit_test("/multi_frame", test_multi_frame),
    munit_null_test,
};
