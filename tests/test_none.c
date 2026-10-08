/**
 * @file test_none.c
 * @brief Unit tests for the immortal None singleton object, its properties,
 * arithmetic immutability, container usage, and reference count immunity.
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
 * @brief Test that none returns a valid singleton with identical pointer across
 * repeated calls and performs zero allocations.
 */
munit_case(
    RUN,
    test_none_singleton_identity,
    {
        vm_new();
        size_t allocs_before = boot_total_alloc_count();
        object_t *none1 = new_none();
        assert_not_null(none1);
        assert_int(none1->kind, ==, NONE, "must be NONE kind");
        assert_size(
            boot_total_alloc_count(), ==, allocs_before,
            "none must perform zero heap allocations"
        );

        object_t *none2 = new_none();
        assert_ptr_equal(none1, none2);
        assert_size(
            boot_total_alloc_count(), ==, allocs_before,
            "repeated none must perform zero heap allocations"
        );

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that none returns NULL safely when no VM is active.
 */
munit_case(
    RUN,
    test_none_without_vm,
    {
        object_t *none = new_none();
        assert_null(none);
    }
);

/**
 * @brief Test None object kind, len, and payload free safety.
 */
munit_case(
    RUN,
    test_none_properties,
    {
        vm_new();
        object_t *none = new_none();
        assert_not_null(none);
        assert_int(none->kind, ==, NONE, "kind must be NONE");
        assert_int64(object_len(none), ==, -1);

        object_free_payload(none);
        object_free_payload(NULL);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test list_set and list_get mutations with None objects.
 */
munit_case(
    RUN,
    test_none_in_list_mutation,
    {
        vm_new();
        object_t *lst = list_new(3);
        object_t *none = new_none();

        assert_true(list_set(lst, 0, none));
        assert_true(list_set(lst, 1, none));
        assert_ptr_equal(list_get(lst, 0), none);
        assert_ptr_equal(list_get(lst, 1), none);
        assert_null(list_get(lst, 2));

        object_t *replacement = integer_new(123);
        assert_true(list_set(lst, 0, replacement));
        assert_ptr_equal(list_get(lst, 0), replacement);
        assert_ptr_equal(list_get(lst, 1), none);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that arithmetic addition with None safely returns NULL.
 */
munit_case(
    RUN,
    test_none_add_operations,
    {
        vm_new();
        object_t *none = new_none();
        object_t *num = integer_new(42);
        object_t *empty_t = tuple_new_0();

        assert_null(object_add(none, none));
        assert_null(object_add(none, num));
        assert_null(object_add(num, none));
        assert_null(object_add(none, empty_t));
        assert_null(object_add(empty_t, none));

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that None reference count is immune to increments and decrements.
 */
munit_case(
    RUN,
    test_none_refcount_immunity,
    {
        vm_new();
        object_t *none = new_none();
        assert_not_null(none);
        assert_size(none->refcount, ==, OBJECT_IMMORTAL_REFCOUNT);

        object_refcount_inc(none);
        assert_size(none->refcount, ==, OBJECT_IMMORTAL_REFCOUNT);

        object_refcount_dec(none);
        assert_size(none->refcount, ==, OBJECT_IMMORTAL_REFCOUNT);
        assert(!boot_is_freed(none));

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that releasing a list holding None via object_refcount_dec cleanly
 * reclaims the list without touching None.
 */
munit_case(
    RUN,
    test_container_holding_none_refcount_parity,
    {
        vm_new();
        object_t *none = new_none();
        object_t *lst = list_new(2);

        list_set(lst, 0, none);
        list_set(lst, 1, none);

        assert_size(none->refcount, ==, OBJECT_IMMORTAL_REFCOUNT);

        // Reclaiming the container via object_refcount_dec (testing reference count
        // parity)
        object_refcount_dec(lst);

        assert(!boot_is_freed(none));

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test multi-level nested containers holding None and empty tuple reclaimed via
 * refcount.
 */
munit_case(
    RUN,
    test_deep_nested_immortals_refcount_parity,
    {
        vm_new();
        object_t *none = new_none();
        object_t *t0 = tuple_new_0();

        object_t *l_inner = list_new(2);
        list_set(l_inner, 0, none);
        list_set(l_inner, 1, t0);

        object_t *t_mid = tuple_new_1(l_inner);
        object_refcount_dec(l_inner);

        object_t *l_outer = list_new(1);
        list_set(l_outer, 0, t_mid);
        object_refcount_dec(t_mid);

        // Reclaim entire hierarchy by dropping outer list
        object_refcount_dec(l_outer);

        assert(!boot_is_freed(none));
        assert(!boot_is_freed(t0));

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

MunitTest none_tests[] = {
    munit_test("/singleton_identity", test_none_singleton_identity),
    munit_test("/without_vm", test_none_without_vm),
    munit_test("/properties", test_none_properties),
    munit_test("/in_list_mutation", test_none_in_list_mutation),
    munit_test("/add_operations", test_none_add_operations),
    munit_test("/refcount_immunity", test_none_refcount_immunity),
    munit_test(
        "/container_holding_none_refcount_parity",
        test_container_holding_none_refcount_parity
    ),
    munit_test(
        "/deep_nested_immortals_refcount_parity",
        test_deep_nested_immortals_refcount_parity
    ),
    munit_null_test,
};
