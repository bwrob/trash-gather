/**
 * @file test_tuple.c
 * @brief Unit tests for tuple object construction, indexing, addition, and memory
 * lifecycles.
 */

#include "bootlib.h"
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
 * @brief Test allocating an empty tuple (0 elements).
 */
munit_case(
    RUN,
    test_tuple_0_empty,
    {
        vm_new();
        object_t *tuple = tuple_new_0();

        assert_not_null(tuple);
        assert_int(tuple->kind, ==, TUPLE);
        assert_not_null(tuple->data.v_tuple);
        assert_size(tuple->data.v_tuple->size, ==, 0);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test allocating a 1-element tuple.
 */
munit_case(
    RUN,
    test_tuple_1_object,
    {
        vm_new();
        object_t *x = integer_new(42);
        object_t *tuple = tuple_new_1(x);

        assert_not_null(tuple);
        assert_int(tuple->kind, ==, TUPLE);
        assert_size(tuple->data.v_tuple->size, ==, 1);
        assert_ptr_equal(tuple->data.v_tuple->elements[0], x);
        assert_int(x->refcount, ==, 2);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test allocating a 2-element tuple.
 */
munit_case(
    RUN,
    test_tuple_2_object,
    {
        vm_new();
        object_t *x = integer_new(10);
        object_t *y = integer_new(20);
        object_t *tuple = tuple_new_2(x, y);

        assert_not_null(tuple);
        assert_int(tuple->kind, ==, TUPLE);
        assert_size(tuple->data.v_tuple->size, ==, 2);
        assert_ptr_equal(tuple->data.v_tuple->elements[0], x);
        assert_ptr_equal(tuple->data.v_tuple->elements[1], y);
        assert_int(x->refcount, ==, 2);
        assert_int(y->refcount, ==, 2);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test allocating a 3-element tuple.
 */
munit_case(
    RUN,
    test_tuple_3_object,
    {
        vm_new();
        object_t *x = integer_new(1);
        object_t *y = integer_new(2);
        object_t *z = integer_new(3);
        object_t *tuple = tuple_new_3(x, y, z);

        assert_not_null(tuple);
        assert_int(tuple->kind, ==, TUPLE);
        assert_size(tuple->data.v_tuple->size, ==, 3);
        assert_ptr_equal(tuple->data.v_tuple->elements[0], x);
        assert_ptr_equal(tuple->data.v_tuple->elements[1], y);
        assert_ptr_equal(tuple->data.v_tuple->elements[2], z);
        assert_int(x->refcount, ==, 2);
        assert_int(y->refcount, ==, 2);
        assert_int(z->refcount, ==, 2);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test tuple allocation rejection on NULL inputs.
 */
munit_case(
    RUN,
    test_tuple_null_rejection,
    {
        vm_new();
        object_t *val = integer_new(1);

        assert_null(tuple_new_1(NULL));
        assert_null(tuple_new_2(NULL, val));
        assert_null(tuple_new_2(val, NULL));
        assert_null(tuple_new_2(NULL, NULL));
        assert_null(tuple_new_3(NULL, val, val));
        assert_null(tuple_new_3(val, NULL, val));
        assert_null(tuple_new_3(val, val, NULL));
        assert_null(tuple_new_3(NULL, NULL, NULL));
        assert_null(tuple_new(NULL, 1));
        assert_null(tuple_new(NULL, 5));

        object_t *arr_with_null[2];
        arr_with_null[0] = val;
        arr_with_null[1] = NULL;
        assert_null(tuple_new(arr_with_null, 2));

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test allocating a 0-element tuple via tuple_new.
 */
munit_case(
    RUN,
    test_tuple_0_from_array,
    {
        vm_new();
        object_t *tuple = tuple_new(NULL, 0);

        assert_not_null(tuple);
        assert_int(tuple->kind, ==, TUPLE);
        assert_size(tuple->data.v_tuple->size, ==, 0);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test allocating an arbitrary length tuple from an object array.
 */
munit_case(
    RUN,
    test_tuple_arbitrary_array,
    {
        vm_new();
        object_t *items[5];
        for (int i = 0; i < 5; i++)
        {
            items[i] = integer_new(i * 10);
        }

        object_t *tuple = tuple_new(items, 5);
        assert_not_null(tuple);
        assert_int(tuple->kind, ==, TUPLE);
        assert_size(tuple->data.v_tuple->size, ==, 5);

        for (int i = 0; i < 5; i++)
        {
            assert_ptr_equal(tuple->data.v_tuple->elements[i], items[i]);
            assert_int(items[i]->refcount, ==, 2);
        }

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test allocating a large tuple (100 elements) via tuple_new.
 */
munit_case(
    RUN,
    test_tuple_large_n,
    {
        vm_new();
        const size_t n = 100;
        object_t *items[100];
        for (size_t i = 0; i < n; i++)
        {
            items[i] = integer_new((int)i);
        }

        object_t *tuple = tuple_new(items, n);
        assert_not_null(tuple);
        assert_size(tuple->data.v_tuple->size, ==, n);

        for (size_t i = 0; i < n; i++)
        {
            assert_ptr_equal(tuple->data.v_tuple->elements[i], items[i]);
            assert_int(items[i]->refcount, ==, 2);
        }

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that tuple_new_0 and tuple_new(..., 0) return the singleton empty tuple.
 */
munit_case(
    RUN,
    test_tuple_0_singleton_identity,
    {
        vm_new();
        size_t allocs_before = boot_total_alloc_count();
        object_t *t1 = tuple_new_0();
        assert_not_null(t1);
        assert_int(t1->kind, ==, TUPLE, "empty tuple must have TUPLE kind");
        assert_int(t1->data.v_tuple->size, ==, 0, "empty tuple must have size 0");
        assert_size(
            boot_total_alloc_count(), ==, allocs_before,
            "tuple_new_0 must perform zero heap allocations"
        );

        object_t *t2 = tuple_new_0();
        assert_ptr_equal(t1, t2);
        assert_size(
            boot_total_alloc_count(), ==, allocs_before,
            "repeated tuple_new_0 must perform zero heap allocations"
        );

        object_t *t3 = tuple_new(NULL, 0);
        assert_ptr_equal(t1, t3);
        assert_size(
            boot_total_alloc_count(), ==, allocs_before,
            "tuple_new(NULL, 0) must perform zero heap allocations"
        );

        object_t *dummy[] = {NULL};
        object_t *t4 = tuple_new(dummy, 0);
        assert_ptr_equal(t1, t4);
        assert_size(
            boot_total_alloc_count(), ==, allocs_before,
            "tuple_new(dummy, 0) must perform zero heap allocations"
        );

        object_t *items[] = {t1};
        object_t *t5 = tuple_new(items, 0);
        assert_ptr_equal(t1, t5);
        assert_size(
            boot_total_alloc_count(), ==, allocs_before,
            "tuple_new(items, 0) must perform zero heap allocations"
        );

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test elementwise addition of coordinate tuples.
 */
munit_case(
    RUN,
    test_tuple_add,
    {
        vm_new();
        object_t *x1 = integer_new(1);
        object_t *y1 = integer_new(2);
        object_t *z1 = integer_new(3);
        object_t *v1 = tuple_new_3(x1, y1, z1);

        object_t *x2 = integer_new(4);
        object_t *y2 = integer_new(5);
        object_t *z2 = integer_new(6);
        object_t *v2 = tuple_new_3(x2, y2, z2);

        object_t *res = object_add(v1, v2);

        assert_not_null(res);
        assert_int(res->kind, ==, TUPLE);
        assert_int(res->data.v_tuple->size, ==, 3);
        assert_int(res->data.v_tuple->elements[0]->data.v_int, ==, 5);
        assert_int(res->data.v_tuple->elements[1]->data.v_int, ==, 7);
        assert_int(res->data.v_tuple->elements[2]->data.v_int, ==, 9);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test tuple addition fails cleanly when lengths differ.
 */
munit_case(
    RUN,
    test_tuple_add_size_mismatch,
    {
        vm_new();
        object_t *i1 = integer_new(1);
        object_t *i2 = integer_new(2);
        object_t *i3 = integer_new(3);

        object_t *t2 = tuple_new_2(i1, i2);
        object_t *t3 = tuple_new_3(i1, i2, i3);

        object_t *res = object_add(t2, t3);
        assert_null(res);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test empty tuple addition returns empty tuple.
 */
munit_case(
    RUN,
    test_tuple_add_empty,
    {
        vm_new();
        object_t *t0_a = tuple_new_0();
        object_t *t0_b = tuple_new_0();

        object_t *res = object_add(t0_a, t0_b);
        assert_not_null(res);
        assert_int(res->kind, ==, TUPLE);
        assert_int(res->data.v_tuple->size, ==, 0);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test tuple addition memory failure rollback clean unwinding.
 */
munit_case(
    RUN,
    test_tuple_add_alloc_failure,
    {
        for (int fail_idx = 0; fail_idx < 10; fail_idx++)
        {
            vm_new();
            // Pre-expand objects stack so stack_push realloc isn't triggered
            vm_get_current()->objects->data =
                realloc(vm_get_current()->objects->data, 64 * sizeof(void *));
            vm_get_current()->objects->capacity = 64;

            object_t *t1 = tuple_new_2(integer_new(1), integer_new(2));
            object_t *t2 = tuple_new_2(integer_new(3), integer_new(4));

            boot_set_fail_alloc_after(fail_idx);
            object_t *res = object_add(t1, t2);
            if (res != NULL)
            {
                assert_int(res->kind, ==, TUPLE);
            }
            boot_set_fail_alloc_after(-1);

            vm_free();
            assert(boot_all_freed());
        }
    }
);

/**
 * @brief Test that adding empty tuple to non-empty tuple returns NULL due to size
 * mismatch.
 */
munit_case(
    RUN,
    test_tuple_add_empty_with_nonempty,
    {
        vm_new();
        object_t *t0 = tuple_new_0();
        object_t *elem = integer_new(42);
        object_t *t1 = tuple_new_1(elem);

        assert_null(object_add(t0, t1));
        assert_null(object_add(t1, t0));

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test object_len polymorphic dispatch for tuples.
 */
munit_case(
    RUN,
    test_object_len_tuples,
    {
        vm_new();
        object_t *i = integer_new(1);
        object_t *f = float_new(2.0f);
        object_t *s = string_new("three");

        object_t *t0 = tuple_new_0();
        object_t *t1 = tuple_new_1(i);
        object_t *t2 = tuple_new_2(i, f);
        object_t *t3 = tuple_new_3(i, f, s);

        assert_int64(object_len(t0), ==, 0);
        assert_int64(object_len(t1), ==, 1);
        assert_int64(object_len(t2), ==, 2);
        assert_int64(object_len(t3), ==, 3);

        object_t *items[6];
        items[0] = i;
        items[1] = f;
        items[2] = s;
        items[3] = i;
        items[4] = f;
        items[5] = s;
        object_t *t6 = tuple_new(items, 6);
        assert_int64(object_len(t6), ==, 6);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test vector reference counting across dimensions and nested free.
 */
munit_case(
    RUN,
    test_vector3_refcounting,
    {
        vm_new();
        object_t *foo = integer_new(1);
        object_t *bar = integer_new(2);
        object_t *baz = integer_new(3);

        object_t *vec = tuple_new_3(foo, bar, baz);
        assert_int(foo->refcount, ==, 2, "foo is now referenced by vec");
        assert_int(bar->refcount, ==, 2, "bar is now referenced by vec");
        assert_int(baz->refcount, ==, 2, "baz is now referenced by vec");

        // Drop outer references
        object_refcount_dec(foo);
        assert(!boot_is_freed(foo));

        // Dropping vec drops inner references and frees foo
        object_refcount_dec(vec);
        assert(boot_is_freed(foo));

        // bar and baz still have outer references
        assert(!boot_is_freed(bar));
        assert(!boot_is_freed(baz));

        object_refcount_dec(bar);
        object_refcount_dec(baz);

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test cascading decref through deeply nested tuple hierarchies.
 */
munit_case(
    RUN,
    test_tuple_nested_refcounting,
    {
        vm_new();
        object_t *leaf1 = integer_new(10);
        object_t *leaf2 = string_new("hello");
        object_t *inner = tuple_new_2(leaf1, leaf2);
        object_t *outer = tuple_new_1(inner);

        assert_int(leaf1->refcount, ==, 2);
        assert_int(leaf2->refcount, ==, 2);
        assert_int(inner->refcount, ==, 2);
        assert_int(outer->refcount, ==, 1);

        // Drop external direct references to inner objects
        object_refcount_dec(leaf1);
        object_refcount_dec(leaf2);
        object_refcount_dec(inner);

        // All inner objects are kept alive solely through the outer tuple
        assert(!boot_is_freed(leaf1));
        assert(!boot_is_freed(leaf2));
        assert(!boot_is_freed(inner));

        // Dropping outer tuple cascades and frees inner tuple and leaves
        object_refcount_dec(outer);
        assert(boot_is_freed(outer));
        assert(boot_is_freed(inner));
        assert(boot_is_freed(leaf1));
        assert(boot_is_freed(leaf2));

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that elements created during tuple addition have refcount 1 and
 * are properly freed when the resulting tuple is decref'd.
 */
munit_case(
    RUN,
    test_tuple_add_refcount_lifecycle,
    {
        vm_new();
        object_t *i1 = integer_new(1);
        object_t *i2 = integer_new(2);
        object_t *t1 = tuple_new_2(i1, i2);

        object_t *i3 = integer_new(10);
        object_t *i4 = integer_new(20);
        object_t *t2 = tuple_new_2(i3, i4);

        object_t *res = object_add(t1, t2);
        assert_not_null(res);
        assert_int(res->kind, ==, TUPLE);
        assert_size(res->data.v_tuple->size, ==, 2);

        // Child elements in res should have refcount == 1 (owned exclusively by res)
        assert_int(
            res->data.v_tuple->elements[0]->refcount, ==, 1,
            "Result element 0 should have refcount 1"
        );
        assert_int(
            res->data.v_tuple->elements[1]->refcount, ==, 1,
            "Result element 1 should have refcount 1"
        );

        // Release input tuples and their elements
        object_refcount_dec(i1);
        object_refcount_dec(i2);
        object_refcount_dec(t1);

        object_refcount_dec(i3);
        object_refcount_dec(i4);
        object_refcount_dec(t2);

        // Now release the result tuple: should cascade-free res and both child elements
        object_refcount_dec(res);

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test rollback clean release when tuple addition element addition fails.
 */
munit_case(
    RUN,
    test_tuple_add_component_failure_rollback,
    {
        vm_new();
        object_t *i1 = integer_new(1);
        object_t *s1 = string_new("cannot_add_to_int");
        object_t *t1 = tuple_new_2(i1, s1);

        object_t *i2 = integer_new(2);
        object_t *i3 = integer_new(3);
        object_t *t2 = tuple_new_2(i2, i3);

        // Element 0 (1 + 2) succeeds; Element 1 ("cannot_add_to_int" + 3) fails
        object_t *res = object_add(t1, t2);
        assert_null(res);

        // Release input tuples
        object_refcount_dec(i1);
        object_refcount_dec(s1);
        object_refcount_dec(t1);

        object_refcount_dec(i2);
        object_refcount_dec(i3);
        object_refcount_dec(t2);

        // Any intermediate element allocated for index 0 must have been freed
        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test rollback in nested tuple addition when sub-addition fails.
 */
munit_case(
    RUN,
    test_tuple_add_nested_failure_rollback,
    {
        vm_new();
        // Inner tuple 1: (1, 2)
        object_t *i1 = integer_new(1);
        object_t *i2 = integer_new(2);
        object_t *inner1 = tuple_new_2(i1, i2);

        // Inner tuple 2: (3, "unsupported")
        object_t *i3 = integer_new(3);
        object_t *s1 = string_new("unsupported");
        object_t *inner2 = tuple_new_2(i3, s1);

        // Outer tuple A: ((1, 2), (3, "unsupported"))
        object_t *outer_a = tuple_new_2(inner1, inner2);

        // Inner tuple 3: (10, 20)
        object_t *i4 = integer_new(10);
        object_t *i5 = integer_new(20);
        object_t *inner3 = tuple_new_2(i4, i5);

        // Inner tuple 4: (30, 40)
        object_t *i6 = integer_new(30);
        object_t *i7 = integer_new(40);
        object_t *inner4 = tuple_new_2(i6, i7);

        // Outer tuple B: ((10, 20), (30, 40))
        object_t *outer_b = tuple_new_2(inner3, inner4);

        // Outer addition:
        // index 0: add((1, 2), (10, 20)) -> succeeds! (allocates new inner tuple (11,
        // 22)) index 1: add((3, "unsupported"), (30, 40)) -> fails on component 1!
        object_t *res = object_add(outer_a, outer_b);
        assert_null(res);

        // Decrement inputs
        object_refcount_dec(i1);
        object_refcount_dec(i2);
        object_refcount_dec(inner1);
        object_refcount_dec(i3);
        object_refcount_dec(s1);
        object_refcount_dec(inner2);
        object_refcount_dec(outer_a);

        object_refcount_dec(i4);
        object_refcount_dec(i5);
        object_refcount_dec(inner3);
        object_refcount_dec(i6);
        object_refcount_dec(i7);
        object_refcount_dec(inner4);
        object_refcount_dec(outer_b);

        // All intermediate objects (including the successfully allocated (11, 22)
        // tuple) must have been completely freed by the failure rollback!
        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test empty tuple singleton refcount immunity.
 */
munit_case(
    RUN,
    test_empty_tuple_refcount_immunity,
    {
        vm_new();
        object_t *t0 = tuple_new_0();
        assert_not_null(t0);
        assert_true(object_is_immortal(t0));
        assert_size(t0->refcount, ==, OBJECT_IMMORTAL_REFCOUNT);

        object_refcount_inc(t0);
        assert_size(t0->refcount, ==, OBJECT_IMMORTAL_REFCOUNT);
        object_refcount_dec(t0);
        assert_size(t0->refcount, ==, OBJECT_IMMORTAL_REFCOUNT);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test tuple holding None singleton refcount lifecycle.
 */
munit_case(
    RUN,
    test_tuple_holding_none_refcount_lifecycle,
    {
        vm_new();
        object_t *none = none_get();
        object_t *t = tuple_new_1(none);

        assert_ptr_equal(t->data.v_tuple->elements[0], none);
        assert_size(none->refcount, ==, OBJECT_IMMORTAL_REFCOUNT);

        object_refcount_dec(t);
        assert(!boot_is_freed(none));

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test immortals in nested tuples refcount parity.
 */
munit_case(
    RUN,
    test_immortals_in_nested_tuples_refcount_parity,
    {
        vm_new();
        object_t *none = none_get();
        object_t *t0 = tuple_new_0();

        object_t *t_inner = tuple_new_2(none, t0);
        object_t *t_outer = tuple_new_2(t_inner, none);

        assert_size(none->refcount, ==, OBJECT_IMMORTAL_REFCOUNT);
        assert_size(t0->refcount, ==, OBJECT_IMMORTAL_REFCOUNT);

        object_refcount_dec(t_inner);
        object_refcount_dec(t_outer);

        assert(!boot_is_freed(none));
        assert(!boot_is_freed(t0));

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test tuple addition mid-failure with None singleton.
 */
munit_case(
    RUN,
    test_tuple_add_mid_failure_with_none,
    {
        vm_new();
        object_t *none = none_get();
        object_t *i1 = integer_new(10);
        object_t *t1 = tuple_new_2(i1, none);

        object_t *i2 = integer_new(20);
        object_t *t2 = tuple_new_2(i2, none);

        object_t *res = object_add(t1, t2);
        assert_null(res);

        assert_size(none->refcount, ==, OBJECT_IMMORTAL_REFCOUNT);
        assert_int(i1->refcount, ==, 2);
        assert_int(i2->refcount, ==, 2);

        object_refcount_dec(t1);
        object_refcount_dec(t2);

        assert_int(i1->refcount, ==, 1);
        assert_int(i2->refcount, ==, 1);
        assert_size(none->refcount, ==, OBJECT_IMMORTAL_REFCOUNT);

        vm_free();
        assert(boot_all_freed());
    }
);

MunitTest tuple_tests[] = {
    munit_test("/tuple_0_empty", test_tuple_0_empty),
    munit_test("/tuple_0_from_array", test_tuple_0_from_array),
    munit_test("/tuple_1_object", test_tuple_1_object),
    munit_test("/tuple_2_object", test_tuple_2_object),
    munit_test("/tuple_3_object", test_tuple_3_object),
    munit_test("/tuple_null_rejection", test_tuple_null_rejection),
    munit_test("/tuple_arbitrary_array", test_tuple_arbitrary_array),
    munit_test("/tuple_large_n", test_tuple_large_n),
    munit_test("/tuple_0_singleton_identity", test_tuple_0_singleton_identity),
    munit_test("/tuple_add", test_tuple_add),
    munit_test("/tuple_add_size_mismatch", test_tuple_add_size_mismatch),
    munit_test("/tuple_add_empty", test_tuple_add_empty),
    munit_test("/tuple_add_alloc_failure", test_tuple_add_alloc_failure),
    munit_test("/tuple_add_empty_with_nonempty", test_tuple_add_empty_with_nonempty),
    munit_test("/len_tuples", test_object_len_tuples),
    munit_test("/vector3_refcounting", test_vector3_refcounting),
    munit_test("/tuple_nested_refcounting", test_tuple_nested_refcounting),
    munit_test("/tuple_add_refcount_lifecycle", test_tuple_add_refcount_lifecycle),
    munit_test(
        "/tuple_add_component_failure_rollback",
        test_tuple_add_component_failure_rollback
    ),
    munit_test(
        "/tuple_add_nested_failure_rollback",
        test_tuple_add_nested_failure_rollback
    ),
    munit_test("/empty_tuple_refcount_immunity", test_empty_tuple_refcount_immunity),
    munit_test(
        "/tuple_holding_none_refcount_lifecycle",
        test_tuple_holding_none_refcount_lifecycle
    ),
    munit_test(
        "/immortals_in_nested_tuples_refcount_parity",
        test_immortals_in_nested_tuples_refcount_parity
    ),
    munit_test(
        "/tuple_add_mid_failure_with_none",
        test_tuple_add_mid_failure_with_none
    ),
    munit_null_test,
};
