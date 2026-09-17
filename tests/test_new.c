/**
 * @file test_new.c
 * @brief Unit tests for object allocation constructors (integers, floats,
 * strings, vectors, lists) and failure injection.
 */

#include "bootlib.h"
#include "munit.h"
#include "new.h"
#include "object.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Test allocating positive integer objects.
 */
munit_case(
    RUN,
    test_positive_integer,
    {
        vm_new();
        object_t *int_object = new_integer(42);
        assert_int(int_object->data.v_int, ==, 42, "must allow positive numbers");

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test allocating zero integer objects.
 */
munit_case(
    RUN,
    test_zero_integer,
    {
        vm_new();
        object_t *int_object = new_integer(0);

        assert_int(int_object->kind, ==, INTEGER, "must be INTEGER type");
        assert_int(int_object->data.v_int, ==, 0, "must equal zero");

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test allocating negative integer objects.
 */
munit_case(
    SUBMIT,
    test_negative_integer,
    {
        vm_new();
        object_t *int_object = new_integer(-5);

        assert_int(int_object->kind, ==, INTEGER, "must be INTEGER type");
        assert_int(int_object->data.v_int, ==, -5, "must allow negative numbers");

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test allocating floating-point objects.
 */
munit_case(
    RUN,
    test_float_object,
    {
        vm_new();
        object_t *float_object = new_float(3.14f);

        assert_int(float_object->kind, ==, FLOAT, "must be FLOAT type");
        assert_double_equal((double)float_object->data.v_float, 3.14, 2);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test allocating copied string objects.
 */
munit_case(
    RUN,
    test_string_object,
    {
        vm_new();
        object_t *string_object = new_string("Hello ");

        assert_int(string_object->kind, ==, STRING, "must be STRING type");
        assert_string_equal(
            string_object->data.v_string, "Hello ", "must copy string content"
        );

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test allocating an empty tuple (0 elements).
 */
munit_case(
    RUN,
    test_tuple_0_empty,
    {
        vm_new();
        object_t *tuple = new_tuple_0();

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
        object_t *x = new_integer(42);
        object_t *tuple = new_tuple_1(x);

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
        object_t *x = new_integer(10);
        object_t *y = new_integer(20);
        object_t *tuple = new_tuple_2(x, y);

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
        object_t *x = new_integer(1);
        object_t *y = new_integer(2);
        object_t *z = new_integer(3);
        object_t *tuple = new_tuple_3(x, y, z);

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
        object_t *val = new_integer(1);

        assert_null(new_tuple_1(NULL));
        assert_null(new_tuple_2(NULL, val));
        assert_null(new_tuple_2(val, NULL));
        assert_null(new_tuple_2(NULL, NULL));
        assert_null(new_tuple_3(NULL, val, val));
        assert_null(new_tuple_3(val, NULL, val));
        assert_null(new_tuple_3(val, val, NULL));
        assert_null(new_tuple_3(NULL, NULL, NULL));
        assert_null(new_tuple(NULL, 1));
        assert_null(new_tuple(NULL, 5));

        object_t *arr_with_null[2];
        arr_with_null[0] = val;
        arr_with_null[1] = NULL;
        assert_null(new_tuple(arr_with_null, 2));

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test allocating a 0-element tuple via new_tuple.
 */
munit_case(
    RUN,
    test_tuple_0_from_array,
    {
        vm_new();
        object_t *tuple = new_tuple(NULL, 0);

        assert_not_null(tuple);
        assert_int(tuple->kind, ==, TUPLE);
        assert_size(tuple->data.v_tuple->size, ==, 0);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test allocating an arbitrary-length tuple from an array.
 */
munit_case(
    RUN,
    test_tuple_arbitrary_array,
    {
        vm_new();
        object_t *items[5];
        for (int i = 0; i < 5; i++)
        {
            items[i] = new_integer(i * 10);
        }

        object_t *tuple = new_tuple(items, 5);
        assert_not_null(tuple);
        assert_int(tuple->kind, ==, TUPLE);
        assert_size(tuple->data.v_tuple->size, ==, 5);

        for (size_t i = 0; i < 5; i++)
        {
            assert_ptr_equal(tuple->data.v_tuple->elements[i], items[i]);
            assert_int(items[i]->refcount, ==, 2);
        }

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test allocating a large tuple (100 elements) via new_tuple.
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
            items[i] = new_integer((int)i);
        }

        object_t *tuple = new_tuple(items, n);
        assert_not_null(tuple);
        assert_size(tuple->data.v_tuple->size, ==, n);

        for (size_t i = 0; i < n; i++)
        {
            assert_ptr_equal(tuple->data.v_tuple->elements[i], items[i]);
            assert_int(items[i]->data.v_int, ==, (int)i);
        }

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test allocating non-empty list objects.
 */
munit_case(
    RUN,
    test_list_object,
    {
        vm_new();
        object_t *arr = new_list(5);

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
        object_t *arr = new_list(0);

        assert_int(arr->kind, ==, LIST, "must be LIST type");
        assert_size(arr->data.v_list.size, ==, 0, "size must be 0");

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test memory allocation failure simulation across object constructors.
 */
munit_case(
    RUN,
    test_alloc_failures,
    {
        vm_new();

        boot_set_fail_alloc_after(0);
        assert_null(new_integer(1));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(0);
        assert_null(new_float(1.0f));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(0);
        assert_null(new_string("test"));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(1);
        assert_null(new_string("test"));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(0);
        assert_null(new_list(5));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(1);
        assert_null(new_list(5));
        assert_true(boot_fail_alloc_triggered());

        object_t *x = new_integer(1);
        object_t *y = new_integer(2);
        object_t *z = new_integer(3);

        boot_set_fail_alloc_after(0);
        assert_null(create_empty_tuple_singleton());
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(1);
        assert_null(create_empty_tuple_singleton());
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(0);
        assert_null(create_none_singleton());
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(0);
        assert_null(new_tuple_1(x));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(1);
        assert_null(new_tuple_1(x));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(0);
        assert_null(new_tuple_2(x, y));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(1);
        assert_null(new_tuple_2(x, y));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(0);
        assert_null(new_tuple_3(x, y, z));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(1);
        assert_null(new_tuple_3(x, y, z));
        assert_true(boot_fail_alloc_triggered());

        object_t *items[3];
        items[0] = x;
        items[1] = y;
        items[2] = z;
        boot_set_fail_alloc_after(0);
        assert_null(new_tuple(items, 3));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(1);
        assert_null(new_tuple(items, 3));
        assert_true(boot_fail_alloc_triggered());

        // Persistent OOM failure simulation: every allocation fails
        boot_set_fail_alloc_repeat(0, -1);
        assert_null(new_integer(100));
        assert_null(new_float(2.0f));
        assert_null(new_string("oom"));
        assert_null(new_list(10));
        assert_null(new_tuple_1(x));
        assert_size(boot_fail_alloc_injected_count(), >=, 5);
        boot_reset_fail_alloc();

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that new_none returns a valid singleton with identical pointer across
 * repeated calls.
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
            "new_none must perform zero heap allocations"
        );

        object_t *none2 = new_none();
        assert_ptr_equal(none1, none2);
        assert_size(
            boot_total_alloc_count(), ==, allocs_before,
            "repeated new_none must perform zero heap allocations"
        );

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that new_none and new_tuple_0 return NULL safely when no VM is active.
 */
munit_case(
    RUN,
    test_none_without_vm,
    {
        object_t *none = new_none();
        assert_null(none);

        object_t *t0 = new_tuple_0();
        assert_null(t0);
    }
);

/**
 * @brief Test that new_tuple_0 and new_tuple(NULL, 0) return the singleton empty tuple.
 */
munit_case(
    RUN,
    test_tuple_0_singleton_identity,
    {
        vm_new();
        size_t allocs_before = boot_total_alloc_count();
        object_t *t1 = new_tuple_0();
        assert_not_null(t1);
        assert_int(t1->kind, ==, TUPLE, "empty tuple must have TUPLE kind");
        assert_int(t1->data.v_tuple->size, ==, 0, "empty tuple must have size 0");
        assert_size(
            boot_total_alloc_count(), ==, allocs_before,
            "new_tuple_0 must perform zero heap allocations"
        );

        object_t *t2 = new_tuple_0();
        assert_ptr_equal(t1, t2);
        assert_size(
            boot_total_alloc_count(), ==, allocs_before,
            "repeated new_tuple_0 must perform zero heap allocations"
        );

        object_t *t3 = new_tuple(NULL, 0);
        assert_ptr_equal(t1, t3);
        assert_size(
            boot_total_alloc_count(), ==, allocs_before,
            "new_tuple(NULL, 0) must perform zero heap allocations"
        );

        object_t *dummy[] = {NULL};
        assert_null(new_tuple(dummy, 0));

        vm_free();
        assert(boot_all_freed());
    }
);

MunitTest new_tests[] = {
    munit_test("/integer_positive", test_positive_integer),
    munit_test("/integer_zero", test_zero_integer),
    munit_test("/integer_negative", test_negative_integer),
    munit_test("/float_object", test_float_object),
    munit_test("/string_object", test_string_object),
    munit_test("/tuple_0_empty", test_tuple_0_empty),
    munit_test("/tuple_0_from_array", test_tuple_0_from_array),
    munit_test("/tuple_1_object", test_tuple_1_object),
    munit_test("/tuple_2_object", test_tuple_2_object),
    munit_test("/tuple_3_object", test_tuple_3_object),
    munit_test("/tuple_null_rejection", test_tuple_null_rejection),
    munit_test("/tuple_arbitrary_array", test_tuple_arbitrary_array),
    munit_test("/tuple_large_n", test_tuple_large_n),
    munit_test("/tuple_0_singleton_identity", test_tuple_0_singleton_identity),
    munit_test("/list_object", test_list_object),
    munit_test("/list_empty", test_list_empty),
    munit_test("/none_singleton_identity", test_none_singleton_identity),
    munit_test("/none_without_vm", test_none_without_vm),
    munit_test("/alloc_failures", test_alloc_failures),
    munit_null_test,
};
