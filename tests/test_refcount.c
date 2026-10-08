/**
 * @file test_refcount.c
 * @brief Unit tests for core reference counting mechanics, increments, decrements,
 * automatic deallocation, pointer safety, and cyclic limitations.
 */

#include "bootlib.h"
#include "list.h"
#include "munit.h"
#include "object.h"
#include "tuple.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Helper to clean up a VM in unit tests where objects are freed directly
 * by reference counting decrements.
 */
static void vm_cleanup_after_refcount(
    void
)
{
    vm_t *vm = vm_get_current();
    if (vm == NULL)
    {
        return;
    }
    if (vm->objects != NULL)
    {
        vm->objects->count = 0;
    }
    vm_free();
}

/**
 * @brief Test that new integer objects initialize with refcount 1.
 */
munit_case(
    RUN,
    test_int_has_refcount,
    {
        vm_new();
        object_t *obj = integer_new(10);
        assert_int(obj->refcount, ==, 1, "Refcount should be 1 on creation");

        object_refcount_dec(obj);
        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test incrementing object reference count.
 */
munit_case(
    RUN,
    test_inc_refcount,
    {
        vm_new();
        object_t *obj = float_new(4.20f);
        assert_int(obj->refcount, ==, 1, "Refcount should be 1 on creation");

        object_refcount_inc(obj);
        assert_int(obj->refcount, ==, 2, "Refcount should be incremented");

        object_refcount_dec(obj);
        object_refcount_dec(obj);
        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test decrementing object reference count without freeing.
 */
munit_case(
    RUN,
    test_dec_refcount,
    {
        vm_new();
        object_t *obj = float_new(4.20f);

        object_refcount_inc(obj);
        assert_int(obj->refcount, ==, 2, "Refcount should be incremented");

        object_refcount_dec(obj);
        assert_int(obj->refcount, ==, 1, "Refcount should be decremented");
        assert(!boot_is_freed(obj));

        object_refcount_dec(obj);
        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that refcount reaching 0 automatically frees the object.
 */
munit_case(
    RUN,
    test_refcount_free_is_called,
    {
        vm_new();
        object_t *obj = float_new(4.20f);

        object_refcount_inc(obj);
        assert_int(obj->refcount, ==, 2, "Refcount should be incremented");

        object_refcount_dec(obj);
        assert_int(obj->refcount, ==, 1, "Refcount should be decremented");

        object_refcount_dec(obj);
        assert(boot_is_freed(obj));

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that heap-allocated string buffers are freed when refcount drops
 * to 0.
 */
munit_case(
    RUN,
    test_allocated_string_is_freed,
    {
        vm_new();
        object_t *obj = string_new("Hello @wagslane!");

        object_refcount_inc(obj);
        assert_int(obj->refcount, ==, 2, "Refcount should be incremented");

        object_refcount_dec(obj);
        assert_int(obj->refcount, ==, 1, "Refcount should be decremented");
        assert_string_equal(obj->data.v_string, "Hello @wagslane!", "references str");

        object_refcount_dec(obj);
        assert(boot_is_freed(obj));

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial test: NULL pointer safety for refcount operations.
 */
munit_case(
    RUN,
    test_refcount_null_safety,
    {
        // Must safely no-op without crashing
        object_refcount_inc(NULL);
        object_refcount_dec(NULL);
        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial test: circular reference cycle cannot be freed by RC
 * alone.
 */
munit_case(
    RUN,
    test_cycle_refcount_limitation,
    {
        vm_new();
        object_t *arr_a = list_new(1);
        object_t *arr_b = list_new(1);

        // arr_a -> arr_b and arr_b -> arr_a
        list_set(arr_a, 0, arr_b);
        list_set(arr_b, 0, arr_a);

        assert_int(arr_a->refcount, ==, 2, "arr_a referenced by caller and arr_b");
        assert_int(arr_b->refcount, ==, 2, "arr_b referenced by caller and arr_a");

        // Drop external references
        object_refcount_dec(arr_a);
        object_refcount_dec(arr_b);

        // Both still have refcount == 1 due to the cycle!
        assert_int(arr_a->refcount, ==, 1, "arr_a trapped in cycle");
        assert_int(arr_b->refcount, ==, 1, "arr_b trapped in cycle");
        assert(!boot_is_freed(arr_a));
        assert(!boot_is_freed(arr_b));

        // Manually break the cycle to clean up memory in this test
        arr_a->data.v_list.elements[0] = NULL;
        object_refcount_dec(arr_b); // this cascades and frees arr_a and arr_b
        assert(boot_is_freed(arr_a));
        assert(boot_is_freed(arr_b));

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial test: verify localized container lifecycle and refcount
 * reclamation using bootlib checkpoints while keeping the active VM intact.
 */
munit_case(
    RUN,
    test_container_scoped_checkpoint_leak_freedom,
    {
        vm_new();
        object_t *none = new_none();
        object_t *t0 = tuple_new_0();

        // Establish checkpoint: VM and singletons are already alive
        boot_checkpoint_t cp = boot_checkpoint();

        // Allocate local hierarchy of mortal objects and containers
        object_t *val1 = integer_new(42);
        object_t *val2 = string_new("checkpoint_test");
        object_t *lst = list_new(3);
        list_set(lst, 0, val1);
        list_set(lst, 1, val2);
        list_set(lst, 2, none);

        object_t *tup = tuple_new_2(lst, t0);

        // Caller releases their owned references
        object_refcount_dec(val1);
        object_refcount_dec(val2);
        object_refcount_dec(lst);

        // Scoped live allocations exist since checkpoint
        assert_size(boot_checkpoint_leak_count(cp), >, 0);
        assert_size(boot_checkpoint_alloc_size(cp), >, 0);

        // Dropping root container frees all child mortal objects
        object_refcount_dec(tup);

        // All allocations made since the checkpoint must be 100% freed
        assert_true(boot_checkpoint_all_freed(cp));
        assert_size(boot_checkpoint_leak_count(cp), ==, 0);
        assert_size(boot_checkpoint_alloc_size(cp), ==, 0);

        // The VM and singletons remain completely healthy and active
        assert_not_null(vm_get_current());
        assert_ptr_equal(new_none(), none);
        assert_ptr_equal(tuple_new_0(), t0);

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

MunitTest refcount_tests[] = {
    munit_test("/int_has_refcount", test_int_has_refcount),
    munit_test("/inc_refcount", test_inc_refcount),
    munit_test("/dec_refcount", test_dec_refcount),
    munit_test("/refcount_free", test_refcount_free_is_called),
    munit_test("/string_freed", test_allocated_string_is_freed),
    munit_test(
        "/scoped_checkpoint_leak_freedom",
        test_container_scoped_checkpoint_leak_freedom
    ),
    munit_test("/null_safety", test_refcount_null_safety),
    munit_test("/cycle_limitation", test_cycle_refcount_limitation),
    munit_null_test,
};
