/**
 * @file test_template.c
 * @brief Template for adversarial unit tests in trash-gather using µnit and bootlib.
 */

#include "bootlib.h"
#include "munit.h"
#include "new.h"
#include "object.h"
#include "vm.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Adversarial probe: NULL safety and parameter rejection.
 */
munit_case(
    RUN,
    test_null_safety_guard,
    {
        vm_new();

        // Pass NULL to operations; verify graceful failure without segfaults
        object_t *result = new_tuple(NULL, 1);
        assert_null(result);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial probe: Lifecycle reference parity (avoiding vm_free() masking).
 */
munit_case(
    RUN,
    test_reference_parity_and_lifecycle,
    {
        vm_new();

        object_t *elem = new_integer(42);
        assert_not_null(elem);
        assert_size(elem->refcount, ==, 1);

        // Container takes ownership/reference of child
        object_t *container = new_tuple_1(elem);
        assert_not_null(container);
        assert_size(elem->refcount, ==, 2);

        // Release container via pure reference counting
        refcount_dec(container);
        vm_cleanup_after_refcount();

        // Child element must drop back to 1
        assert_size(elem->refcount, ==, 1);

        // Release child
        refcount_dec(elem);
        vm_cleanup_after_refcount();

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial probe: Mid-loop failure unwinding (K-of-N rollback).
 */
munit_case(
    RUN,
    test_mid_loop_allocation_failure_rollback,
    {
        vm_new();

        // Simulate allocation failure during each allocation phase
        for (size_t fail_after = 0; fail_after <= 5; fail_after++)
        {
            boot_set_fail_alloc_after(fail_after);
            object_t *obj = new_list(10);
            (void)obj;
        }

        // Disable fault injection and assert zero leaked heap blocks
        boot_set_fail_alloc_after(SIZE_MAX);
        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Suite test array exported to test_runner.c.
 */
MunitTest example_tests[] = {
    munit_test(test_null_safety_guard),
    munit_test(test_reference_parity_and_lifecycle),
    munit_test(test_mid_loop_allocation_failure_rollback),
    munit_null_test,
};
