/**
 * @file test_list.c
 * @brief Unit tests for list object creation, bounds checking, element indexing,
 * dynamic concatenation, allocation failures, and reference counting lifecycles.
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
 * @brief Test allocating non-empty list objects.
 */
munit_case(
    RUN,
    test_list_object,
    {
        vm_new();
        object_t *arr = list_new(5);

        assert_int(arr->kind, ==, LIST, "must be LIST type");
        assert_size(arr->data.v_list.size, ==, 5, "size must be 5");
        assert_ptr_not_null(
            arr->data.v_list.elements, "elements list must be allocated"
        );
        assert_ptr_null(
            arr->data.v_list.elements[0], "elements must be initialized to NULL"
        );

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test allocating zero-sized empty list objects.
 */
munit_case(
    RUN,
    test_list_empty,
    {
        vm_new();
        object_t *arr = list_new(0);

        assert_int(arr->kind, ==, LIST, "must be LIST type");
        assert_size(arr->data.v_list.size, ==, 0, "size must be 0");

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test list object creation with specified element capacity.
 */
munit_case(
    RUN,
    test_create_empty_list,
    {
        vm_new();
        object_t *obj = list_new(2);

        assert_int(obj->kind, ==, LIST, "Must set type to LIST");
        assert_int(obj->data.v_list.size, ==, 2, "Must set size to 2");

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test zero-initialization of allocated list slots.
 */
munit_case(
    SUBMIT,
    test_used_calloc,
    {
        vm_new();
        object_t *obj = list_new(2);

        assert_ptr_null(obj->data.v_list.elements[0], "Should use calloc");
        assert_ptr_null(obj->data.v_list.elements[1], "Should use calloc");

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test setting elements within valid list index bounds.
 */
munit_case(
    RUN,
    test_list_set,
    {
        vm_new();
        object_t *obj = list_new(2);
        object_t *first = string_new("First");
        object_t *second = integer_new(3);

        assert(list_set(obj, 0, first));
        assert(list_set(obj, 1, second));

        assert_ptr(
            obj->data.v_list.elements[0], ==, first, "Should set the first element"
        );
        assert_ptr(
            obj->data.v_list.elements[1], ==, second, "Should set the second element"
        );

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test list set rejection for out-of-bounds indices.
 */
munit_case(
    RUN,
    test_list_set_outside_bounds,
    {
        vm_new();
        object_t *obj = list_new(2);
        object_t *outside = string_new("First");

        assert(list_set(obj, 1, outside));
        assert_false(list_set(obj, 2, outside));
        assert_false(list_set(obj, 100, outside));
        assert_ptr(
            obj->data.v_list.elements[1], ==, outside,
            "Should preserve existing elements"
        );

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test list set error handling for NULL pointers or non-list inputs.
 */
munit_case(
    SUBMIT,
    test_list_set_rejects_invalid_inputs,
    {
        vm_new();
        object_t *list = list_new(1);
        object_t *value = integer_new(3);
        object_t *not_list = integer_new(5);

        assert_false(list_set(NULL, 0, value));
        assert_false(list_set(list, 0, NULL));
        assert_false(list_set(not_list, 0, value));

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test retrieving elements from populated list slots.
 */
munit_case(
    RUN,
    test_list_get,
    {
        vm_new();
        object_t *obj = list_new(2);
        object_t *first = string_new("First");
        object_t *second = integer_new(3);

        assert(list_set(obj, 0, first));
        assert(list_set(obj, 1, second));

        object_t *retrieved_first = list_get(obj, 0);
        assert_not_null(retrieved_first, "Should find the first object");
        assert_int(retrieved_first->kind, ==, STRING, "Should be a string");
        assert_ptr(first, ==, retrieved_first, "Should be the same object");

        object_t *retrieved_second = list_get(obj, 1);
        assert_not_null(retrieved_second, "Should find the second object");
        assert_int(retrieved_second->kind, ==, INTEGER, "Should be an integer");
        assert_ptr(second, ==, retrieved_second, "Should be the same object");

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test retrieving elements from uninitialized empty list slots.
 */
munit_case(
    RUN,
    test_list_get_empty_slot,
    {
        vm_new();
        object_t *obj = list_new(2);

        assert_null(list_get(obj, 1), "Empty list slots should be NULL");

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test list get rejection for out-of-bounds indices.
 */
munit_case(
    SUBMIT,
    test_list_get_outside_bounds,
    {
        vm_new();
        object_t *obj = list_new(1);
        object_t *first = string_new("First");
        assert(list_set(obj, 0, first));

        assert_null(list_get(obj, 1), "Should not access outside the list");

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test list get rejection for NULL or non-list inputs.
 */
munit_case(
    SUBMIT,
    test_list_get_rejects_invalid_inputs,
    {
        vm_new();
        object_t *not_list = integer_new(5);

        assert_null(list_get(NULL, 0), "Should reject NULL input");
        assert_null(list_get(not_list, 0), "Should reject non-list input");

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test list concatenation via addition operator.
 */
munit_case(
    RUN,
    test_list_add,
    {
        vm_new();
        object_t *arr1 = list_new(2);
        object_t *elem1 = integer_new(10);
        object_t *elem2 = integer_new(20);
        list_set(arr1, 0, elem1);
        list_set(arr1, 1, elem2);

        object_t *arr2 = list_new(1);
        object_t *elem3 = integer_new(30);
        list_set(arr2, 0, elem3);

        object_t *res = object_add(arr1, arr2);

        assert_not_null(res);
        assert_int(res->kind, ==, LIST);
        assert_size(res->data.v_list.size, ==, 3);
        assert_int(list_get(res, 0)->data.v_int, ==, 10);
        assert_int(list_get(res, 1)->data.v_int, ==, 20);
        assert_int(list_get(res, 2)->data.v_int, ==, 30);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial test: list concatenation under heap allocation failures.
 */
munit_case(
    RUN,
    test_list_add_alloc_failure,
    {
        for (int i = 0; i < 4; i++)
        {
            vm_new();
            object_t *arr1 = list_new(2);
            object_t *arr2 = list_new(2);

            boot_set_fail_alloc_after(i);
            object_t *res = object_add(arr1, arr2);
            if (res != NULL)
            {
                assert_int(res->kind, ==, LIST);
            }
            boot_set_fail_alloc_after(-1);

            vm_free();
            assert(boot_all_freed());
        }
    }
);

/**
 * @brief Adversarial test: list concatenation preserving sparse NULL slots.
 */
munit_case(
    RUN,
    test_list_add_sparse_nulls,
    {
        vm_new();
        object_t *arr1 = list_new(2);
        object_t *elem1 = integer_new(10);
        list_set(arr1, 0, elem1);

        object_t *arr2 = list_new(2);
        object_t *elem2 = integer_new(20);
        list_set(arr2, 1, elem2);

        object_t *res = object_add(arr1, arr2);
        assert_not_null(res);
        assert_int(res->kind, ==, LIST);
        assert_size(res->data.v_list.size, ==, 4);

        assert_ptr_equal(list_get(res, 0), elem1);
        assert_null(list_get(res, 1));
        assert_null(list_get(res, 2));
        assert_ptr_equal(list_get(res, 3), elem2);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test object_len resolution on lists with populated and sparse elements.
 */
munit_case(
    RUN,
    test_object_len_lists,
    {
        vm_new();
        object_t *l1 = list_new(1);
        object_t *l5 = list_new(5);
        object_t *elem = integer_new(100);

        assert_int64(object_len(l1), ==, 1);
        assert_int64(object_len(l5), ==, 5);

        list_set(l5, 0, elem);
        list_set(l5, 4, elem);
        assert_int64(object_len(l5), ==, 5);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that assigning to a list increments the new element refcount.
 */
munit_case(
    RUN,
    test_list_set_refcount,
    {
        vm_new();
        object_t *foo = integer_new(1);
        object_t *list = list_new(1);

        list_set(list, 0, foo);
        assert_int(foo->refcount, ==, 2, "foo is now referenced by list");
        assert(!boot_is_freed(foo));

        object_refcount_dec(foo);
        object_refcount_dec(list);

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that overwriting a list element decrements the old element's refcount.
 */
munit_case(
    SUBMIT,
    test_list_free,
    {
        vm_new();
        object_t *foo = integer_new(1);
        object_t *bar = integer_new(2);
        object_t *baz = integer_new(3);

        object_t *list = list_new(2);
        list_set(list, 0, foo);
        list_set(list, 1, bar);
        assert_int(foo->refcount, ==, 2, "foo is now referenced by list");
        assert_int(bar->refcount, ==, 2, "bar is now referenced by list");
        assert_int(baz->refcount, ==, 1, "baz is not yet referenced by list");

        // foo is still referenced in the list, so it should not be freed.
        object_refcount_dec(foo);
        assert(!boot_is_freed(foo));

        // Overwrite index 0 (foo) with baz. foo refcount hits 0 and is freed.
        list_set(list, 0, baz);
        assert(boot_is_freed(foo));

        object_refcount_dec(bar);
        object_refcount_dec(baz);
        object_refcount_dec(list);

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that list concatenation properly manages reference counts of
 * elements shared across lists.
 */
munit_case(
    RUN,
    test_list_add_refcount_lifecycle,
    {
        vm_new();
        object_t *elem1 = integer_new(10);
        object_t *elem2 = string_new("hello");
        object_t *elem3 = float_new(3.14f);

        object_t *list_a = list_new(2);
        list_set(list_a, 0, elem1);
        list_set(list_a, 1, elem2);

        object_t *list_b = list_new(1);
        list_set(list_b, 0, elem3);

        object_t *res = object_add(list_a, list_b);
        assert_not_null(res);
        assert_size(res->data.v_list.size, ==, 3);

        // Each element has 3 references: caller, source list, concatenated list
        assert_int(elem1->refcount, ==, 3);
        assert_int(elem2->refcount, ==, 3);
        assert_int(elem3->refcount, ==, 3);

        // Drop caller references
        object_refcount_dec(elem1);
        object_refcount_dec(elem2);
        object_refcount_dec(elem3);

        // Drop source lists
        object_refcount_dec(list_a);
        object_refcount_dec(list_b);

        // Elements alive solely via res (refcount == 1)
        assert_int(elem1->refcount, ==, 1);
        assert_int(elem2->refcount, ==, 1);
        assert_int(elem3->refcount, ==, 1);

        // Dropping res frees res and all 3 elements
        object_refcount_dec(res);

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial test: list concatenation with lists holding immortals (None and
 * ()), verifying reference count parity and clean reclamation under object_refcount_dec
 * without touching singletons.
 */
munit_case(
    RUN,
    test_list_add_with_immortals_refcount_parity,
    {
        vm_new();
        object_t *none = new_none();
        object_t *t0 = tuple_new_0();
        object_t *elem1 = integer_new(10);
        object_t *elem2 = integer_new(20);

        object_t *list_a = list_new(2);
        list_set(list_a, 0, none);
        list_set(list_a, 1, elem1);

        object_t *list_b = list_new(2);
        list_set(list_b, 0, t0);
        list_set(list_b, 1, elem2);

        object_t *res = object_add(list_a, list_b);
        assert_not_null(res);
        assert_size(res->data.v_list.size, ==, 4);

        assert_ptr_equal(list_get(res, 0), none);
        assert_ptr_equal(list_get(res, 1), elem1);
        assert_ptr_equal(list_get(res, 2), t0);
        assert_ptr_equal(list_get(res, 3), elem2);

        // Mortal elements have 3 references (local var, source list, res)
        assert_int(elem1->refcount, ==, 3);
        assert_int(elem2->refcount, ==, 3);
        assert_size(none->refcount, ==, OBJECT_IMMORTAL_REFCOUNT);
        assert_size(t0->refcount, ==, OBJECT_IMMORTAL_REFCOUNT);

        // Drop local references
        object_refcount_dec(elem1);
        object_refcount_dec(elem2);

        // Drop source lists
        object_refcount_dec(list_a);
        object_refcount_dec(list_b);

        // Mortal elements alive solely via res (refcount == 1)
        assert_int(elem1->refcount, ==, 1);
        assert_int(elem2->refcount, ==, 1);

        // Drop concatenated list
        object_refcount_dec(res);

        assert(!boot_is_freed(none));
        assert(!boot_is_freed(t0));

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

MunitTest list_tests[] = {
    munit_test("/list_object", test_list_object),
    munit_test("/list_empty", test_list_empty),
    munit_test("/create_empty_list", test_create_empty_list),
    munit_test("/used_calloc", test_used_calloc),
    munit_test("/list_set", test_list_set),
    munit_test("/list_set_outside", test_list_set_outside_bounds),
    munit_test("/list_set_invalid", test_list_set_rejects_invalid_inputs),
    munit_test("/list_get", test_list_get),
    munit_test("/list_get_empty", test_list_get_empty_slot),
    munit_test("/list_get_outside", test_list_get_outside_bounds),
    munit_test("/list_get_invalid", test_list_get_rejects_invalid_inputs),
    munit_test("/list_add", test_list_add),
    munit_test("/list_add_alloc_failure", test_list_add_alloc_failure),
    munit_test("/list_add_sparse_nulls", test_list_add_sparse_nulls),
    munit_test("/len_lists", test_object_len_lists),
    munit_test("/list_set_refcount", test_list_set_refcount),
    munit_test("/list_free", test_list_free),
    munit_test("/list_add_refcount_lifecycle", test_list_add_refcount_lifecycle),
    munit_test(
        "/list_add_with_immortals_refcount_parity",
        test_list_add_with_immortals_refcount_parity
    ),
    munit_null_test,
};
