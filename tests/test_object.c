/**
 * @file test_object.c
 * @brief Unit tests for object field properties, list bounds/mutation, and
 * polymorphic addition operations.
 */

#include "bootlib.h"
#include "munit.h"
#include "new.h"
#include "object.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Test existence of marking flags on allocated objects.
 */
munit_case(
    RUN,
    test_field_exists,
    {
        vm_new();
        object_t *lane_courses = new_integer(20);
        object_t *teej_courses = new_integer(1);
        (void)lane_courses->is_marked;
        (void)teej_courses->is_marked;
        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that newly allocated objects are unmarked by default.
 */
munit_case(
    SUBMIT,
    test_marked_is_false,
    {
        vm_new();
        object_t *lane_courses = new_integer(20);
        object_t *teej_courses = new_integer(1);
        assert_false(lane_courses->is_marked);
        assert_false(teej_courses->is_marked);
        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test integer type enum constant definition.
 */
munit_case(
    RUN,
    test_integer_constant,
    { assert_int(INTEGER, ==, 0, "INTEGER is defined as 0"); }
);

/**
 * @brief Test raw integer object structure initialization.
 */
munit_case(
    RUN,
    test_integer_obj,
    {
        object_t *obj = malloc(sizeof(object_t));
        obj->kind = INTEGER;
        obj->data.v_int = 0;
        assert_int(obj->kind, ==, INTEGER, "must be INTEGER type");
        assert_int(obj->data.v_int, ==, 0, "must equal zero");

        free(obj);
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
        object_t *obj = new_list(2);

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
        object_t *obj = new_list(2);

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
        object_t *obj = new_list(2);
        object_t *first = new_string("First");
        object_t *second = new_integer(3);

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
        object_t *obj = new_list(2);
        object_t *outside = new_string("First");

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
        object_t *list = new_list(1);
        object_t *value = new_integer(3);
        object_t *not_list = new_integer(5);

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
        object_t *obj = new_list(2);
        object_t *first = new_string("First");
        object_t *second = new_integer(3);

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
        object_t *obj = new_list(2);

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
        object_t *obj = new_list(1);
        object_t *first = new_string("First");
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
        object_t *not_list = new_integer(5);

        assert_null(list_get(NULL, 0), "Should reject NULL input");
        assert_null(list_get(not_list, 0), "Should reject non-list input");

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test adding integer objects together.
 */
munit_case(
    RUN,
    test_add_integers,
    {
        vm_new();
        object_t *a = new_integer(10);
        object_t *b = new_integer(20);
        object_t *res = object_add(a, b);

        assert_not_null(res);
        assert_int(res->kind, ==, INTEGER);
        assert_int(res->data.v_int, ==, 30);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test polymorphic addition promoting integer and float operands to
 * float.
 */
munit_case(
    RUN,
    test_add_integer_and_float,
    {
        vm_new();
        object_t *a = new_integer(5);
        object_t *b = new_float(2.5f);
        object_t *res1 = object_add(a, b);
        object_t *res2 = object_add(b, a);

        assert_not_null(res1);
        assert_int(res1->kind, ==, FLOAT);
        assert_float(res1->data.v_float, ==, 7.5f);

        assert_not_null(res2);
        assert_int(res2->kind, ==, FLOAT);
        assert_float(res2->data.v_float, ==, 7.5f);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test adding float objects together.
 */
munit_case(
    RUN,
    test_add_floats,
    {
        vm_new();
        object_t *a = new_float(1.5f);
        object_t *b = new_float(2.5f);
        object_t *res = object_add(a, b);

        assert_not_null(res);
        assert_int(res->kind, ==, FLOAT);
        assert_float(res->data.v_float, ==, 4.0f);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test concatenating string objects together.
 */
munit_case(
    RUN,
    test_add_strings,
    {
        vm_new();
        object_t *a = new_string("Hello ");
        object_t *b = new_string("World!");
        object_t *res = object_add(a, b);

        assert_not_null(res);
        assert_int(res->kind, ==, STRING);
        assert_string_equal(res->data.v_string, "Hello World!");

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial test: string concatenation under heap allocation failures.
 */
munit_case(
    RUN,
    test_add_strings_alloc_failure,
    {
        for (int i = 0; i < 3; i++)
        {
            vm_new();
            object_t *a = new_string("Hello ");
            object_t *b = new_string("World!");

            boot_set_fail_alloc_after(i);
            object_t *res = object_add(a, b);
            if (res != NULL)
            {
                assert_int(res->kind, ==, STRING);
                assert_string_equal(res->data.v_string, "Hello World!");
            }
            boot_set_fail_alloc_after(-1);

            vm_free();
            assert(boot_all_freed());
        }
    }
);

/**
 * @brief Test component-wise addition of tuple objects.
 */
munit_case(
    RUN,
    test_add_tuples,
    {
        vm_new();
        object_t *x1 = new_integer(1);
        object_t *y1 = new_integer(2);
        object_t *z1 = new_integer(3);
        object_t *v1 = new_tuple_3(x1, y1, z1);

        object_t *x2 = new_integer(4);
        object_t *y2 = new_integer(5);
        object_t *z2 = new_integer(6);
        object_t *v2 = new_tuple_3(x2, y2, z2);

        object_t *res = object_add(v1, v2);

        assert_not_null(res);
        assert_int(res->kind, ==, TUPLE);
        assert_size(res->data.v_tuple->size, ==, 3);
        assert_int(res->data.v_tuple->elements[0]->data.v_int, ==, 5);
        assert_int(res->data.v_tuple->elements[1]->data.v_int, ==, 7);
        assert_int(res->data.v_tuple->elements[2]->data.v_int, ==, 9);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial test: adding tuples of differing lengths must be rejected.
 */
munit_case(
    RUN,
    test_add_tuples_size_mismatch,
    {
        vm_new();
        object_t *i1 = new_integer(1);
        object_t *i2 = new_integer(2);
        object_t *i3 = new_integer(3);

        object_t *t2 = new_tuple_2(i1, i2);
        object_t *t3 = new_tuple_3(i1, i2, i3);
        object_t *t0 = new_tuple_0();

        assert_null(object_add(t2, t3));
        assert_null(object_add(t3, t2));
        assert_null(object_add(t0, t2));
        assert_null(object_add(t2, t0));

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test adding two empty tuples produces an empty tuple.
 */
munit_case(
    RUN,
    test_add_tuples_empty,
    {
        vm_new();
        object_t *t0_a = new_tuple_0();
        object_t *t0_b = new_tuple_0();

        object_t *res = object_add(t0_a, t0_b);
        assert_not_null(res);
        assert_int(res->kind, ==, TUPLE);
        assert_size(res->data.v_tuple->size, ==, 0);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial test: heap failure injection during tuple addition.
 */
munit_case(
    RUN,
    test_add_tuples_alloc_failure,
    {
        for (int fail_idx = 0; fail_idx < 10; fail_idx++)
        {
            vm_new();
            // Pre-expand objects stack so stack_push realloc isn't triggered
            vm_get_current()->objects->data =
                realloc(vm_get_current()->objects->data, 64 * sizeof(void *));
            vm_get_current()->objects->capacity = 64;

            object_t *t1 = new_tuple_2(new_integer(1), new_integer(2));
            object_t *t2 = new_tuple_2(new_integer(3), new_integer(4));

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
 * @brief Test list concatenation via addition operator.
 */
munit_case(
    RUN,
    test_add_lists,
    {
        vm_new();
        object_t *arr1 = new_list(2);
        object_t *elem1 = new_integer(10);
        object_t *elem2 = new_integer(20);
        list_set(arr1, 0, elem1);
        list_set(arr1, 1, elem2);

        object_t *arr2 = new_list(1);
        object_t *elem3 = new_integer(30);
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
    test_add_lists_alloc_failure,
    {
        for (int i = 0; i < 4; i++)
        {
            vm_new();
            object_t *arr1 = new_list(2);
            object_t *arr2 = new_list(2);

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
    test_add_lists_sparse_nulls,
    {
        vm_new();
        object_t *arr1 = new_list(2);
        object_t *elem1 = new_integer(10);
        list_set(arr1, 0, elem1);

        object_t *arr2 = new_list(2);
        object_t *elem2 = new_integer(20);
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
 * @brief Test addition rejection for invalid or mismatched object types.
 */
munit_case(
    RUN,
    test_add_invalid_mismatched,
    {
        vm_new();
        object_t *i = new_integer(1);
        object_t *f = new_float(1.0f);
        object_t *s = new_string("hi");
        object_t *v = new_tuple_3(i, i, i);
        object_t *a = new_list(1);

        assert_null(object_add(NULL, i));
        assert_null(object_add(i, NULL));
        assert_null(object_add(i, s));
        assert_null(object_add(f, s));
        assert_null(object_add(s, i));
        assert_null(object_add(v, i));
        assert_null(object_add(a, i));

        object_t invalid_obj = {.kind = (object_kind_t)999};
        assert_null(object_add(&invalid_obj, i));

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that object_len returns -2 when passed a NULL pointer.
 */
munit_case(
    RUN,
    test_object_len_null,
    { assert_int64(object_len(NULL), ==, -2); }
);

/**
 * @brief Test that object_len returns -1 for non-sequence types, -3 for invalid kinds,
 * and preserves refcounts.
 */
munit_case(
    RUN,
    test_object_len_non_sequence,
    {
        vm_new();
        object_t *i = new_integer(42);
        object_t *f = new_float(3.14f);
        object_t invalid_obj = {.kind = (object_kind_t)999};

        assert_int64(object_len(i), ==, -1);
        assert_int64(object_len(f), ==, -1);
        assert_int64(object_len(&invalid_obj), ==, -3);

        assert_size(i->refcount, ==, 1);
        assert_size(f->refcount, ==, 1);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that object_len returns 0 for empty sequence containers.
 */
munit_case(
    RUN,
    test_object_len_empty_containers,
    {
        vm_new();
        object_t *s = new_string("");
        object_t *l = new_list(0);
        object_t *t = new_tuple_0();

        assert_int64(object_len(s), ==, 0);
        assert_int64(object_len(l), ==, 0);
        assert_int64(object_len(t), ==, 0);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test object_len resolution across populated strings of various lengths and
 * const preservation.
 */
munit_case(
    RUN,
    test_object_len_strings,
    {
        vm_new();
        object_t *s1 = new_string("a");
        object_t *s2 = new_string("hello");
        object_t *s3 = new_string("The quick brown fox jumps over the lazy dog.");

        assert_int64(object_len(s1), ==, 1);
        assert_int64(object_len(s2), ==, 5);
        assert_int64(object_len(s3), ==, 44);

        const object_t *const_s = s2;
        assert_int64(object_len(const_s), ==, 5);

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
        object_t *l1 = new_list(1);
        object_t *l5 = new_list(5);
        object_t *elem = new_integer(100);

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
 * @brief Test object_len resolution across variable-length tuples and element refcount
 * preservation.
 */
munit_case(
    RUN,
    test_object_len_tuples,
    {
        vm_new();
        object_t *i = new_integer(10);
        object_t *f = new_float(2.5f);
        object_t *s = new_string("data");

        object_t *t1 = new_tuple_1(i);
        object_t *t2 = new_tuple_2(i, f);
        object_t *t3 = new_tuple_3(i, f, s);

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
        object_t *t6 = new_tuple(items, 6);
        assert_int64(object_len(t6), ==, 6);
        assert_size(t6->refcount, ==, 1);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial test: verify object_len resolves immediately without recursion
 * on self-referencing and mutually cyclic containers.
 */
munit_case(
    RUN,
    test_object_len_cyclic_containers,
    {
        vm_new();

        // 1. Self-referencing cycle: list pointing to itself
        object_t *self_list = new_list(1);
        list_set(self_list, 0, self_list);
        assert_int64(object_len(self_list), ==, 1);

        // 2. Mutual cycle: list A <-> list B
        object_t *list_a = new_list(2);
        object_t *list_b = new_list(3);
        list_set(list_a, 0, list_b);
        list_set(list_b, 0, list_a);

        assert_int64(object_len(list_a), ==, 2);
        assert_int64(object_len(list_b), ==, 3);

        // Discard local roots so cycles are unreferenced from outside
        refcount_dec(self_list);
        refcount_dec(list_a);
        refcount_dec(list_b);

        // Run cycle collector to sweep cyclic garbage
        vm_collect_garbage();
        assert_true(boot_is_freed(self_list));
        assert_true(boot_is_freed(list_a));
        assert_true(boot_is_freed(list_b));

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial test: verify object_len handles deeply nested container
 * hierarchies and large sequences without stack overflow or boundary corruption.
 */
munit_case(
    RUN,
    test_object_len_deep_and_large_sequences,
    {
        vm_new();

        // 1. Deeply nested hierarchy: Tuple -> Tuple -> List -> String
        object_t *s = new_string("deep");
        object_t *l = new_list(1);
        list_set(l, 0, s);
        object_t *inner_tuple = new_tuple_1(l);
        object_t *outer_tuple = new_tuple_2(inner_tuple, s);

        assert_int64(object_len(outer_tuple), ==, 2);
        assert_int64(object_len(inner_tuple), ==, 1);
        assert_int64(object_len(l), ==, 1);
        assert_int64(object_len(s), ==, 4);

        // 2. Large list sequence
        const size_t large_size = 2000;
        object_t *large_list = new_list(large_size);
        assert_int64(object_len(large_list), ==, (int64_t)large_size);

        // 3. Large tuple sequence
        const size_t tuple_size = 50;
        object_t *items[50];
        for (size_t idx = 0; idx < tuple_size; idx++)
        {
            items[idx] = s;
        }
        object_t *large_tuple = new_tuple(items, tuple_size);
        assert_int64(object_len(large_tuple), ==, (int64_t)tuple_size);

        vm_free();
        assert(boot_all_freed());
    }
);

MunitTest object_tests[] = {
    munit_test("/field_exists", test_field_exists),
    munit_test("/marked_is_false", test_marked_is_false),
    munit_test("/integer_constant", test_integer_constant),
    munit_test("/integer_obj", test_integer_obj),
    munit_test("/create_empty_list", test_create_empty_list),
    munit_test("/used_calloc", test_used_calloc),
    munit_test("/list_set", test_list_set),
    munit_test("/list_set_outside", test_list_set_outside_bounds),
    munit_test("/list_set_invalid", test_list_set_rejects_invalid_inputs),
    munit_test("/list_get", test_list_get),
    munit_test("/list_get_empty", test_list_get_empty_slot),
    munit_test("/list_get_outside", test_list_get_outside_bounds),
    munit_test("/list_get_invalid", test_list_get_rejects_invalid_inputs),
    munit_test("/add_integers", test_add_integers),
    munit_test("/add_integer_and_float", test_add_integer_and_float),
    munit_test("/add_floats", test_add_floats),
    munit_test("/add_strings", test_add_strings),
    munit_test("/add_strings_alloc_failure", test_add_strings_alloc_failure),
    munit_test("/add_tuples", test_add_tuples),
    munit_test("/add_tuples_size_mismatch", test_add_tuples_size_mismatch),
    munit_test("/add_tuples_empty", test_add_tuples_empty),
    munit_test("/add_tuples_alloc_failure", test_add_tuples_alloc_failure),
    munit_test("/add_lists", test_add_lists),
    munit_test("/add_lists_alloc_failure", test_add_lists_alloc_failure),
    munit_test("/add_lists_sparse_nulls", test_add_lists_sparse_nulls),
    munit_test("/add_invalid_mismatched", test_add_invalid_mismatched),
    munit_test("/len_null", test_object_len_null),
    munit_test("/len_non_sequence", test_object_len_non_sequence),
    munit_test("/len_empty_containers", test_object_len_empty_containers),
    munit_test("/len_strings", test_object_len_strings),
    munit_test("/len_lists", test_object_len_lists),
    munit_test("/len_tuples", test_object_len_tuples),
    munit_test("/len_cyclic_containers", test_object_len_cyclic_containers),
    munit_test("/len_deep_and_large", test_object_len_deep_and_large_sequences),
    munit_null_test,
};
