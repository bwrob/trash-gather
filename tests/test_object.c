/**
 * @file test_object.c
 * @brief Unit tests for base object properties, scalar primitives, arithmetic
 * operators, allocation failure injection, and polymorphic sequence length protocol.
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
 * @brief Test existence of marking flags on allocated objects.
 */
munit_case(
    RUN,
    test_field_exists,
    {
        vm_new();
        object_t *lane_courses = integer_new(20);
        object_t *teej_courses = integer_new(1);
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
        object_t *lane_courses = integer_new(20);
        object_t *teej_courses = integer_new(1);
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
 * @brief Test allocating positive integer objects.
 */
munit_case(
    RUN,
    test_positive_integer,
    {
        vm_new();
        object_t *int_object = integer_new(42);
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
        object_t *int_object = integer_new(0);

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
        object_t *int_object = integer_new(-5);

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
        object_t *float_object = float_new(3.14f);

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
        object_t *string_object = string_new("Hello ");

        assert_int(string_object->kind, ==, STRING, "must be STRING type");
        assert_string_equal(
            string_object->data.v_string, "Hello ", "must copy string content"
        );

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
        object_t *a = integer_new(10);
        object_t *b = integer_new(20);
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
        object_t *a = integer_new(5);
        object_t *b = float_new(2.5f);
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
        object_t *a = float_new(1.5f);
        object_t *b = float_new(2.5f);
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
        object_t *a = string_new("Hello ");
        object_t *b = string_new("World!");
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
            object_t *a = string_new("Hello ");
            object_t *b = string_new("World!");

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
 * @brief Test addition rejection for invalid or mismatched object types.
 */
munit_case(
    RUN,
    test_add_invalid_mismatched,
    {
        vm_new();
        object_t *i = integer_new(1);
        object_t *f = float_new(1.0f);
        object_t *s = string_new("hi");
        object_t *v = tuple_new_3(i, i, i);
        object_t *a = list_new(1);

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
        object_t *i = integer_new(42);
        object_t *f = float_new(3.14f);
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
        object_t *s = string_new("");
        object_t *l = list_new(0);
        object_t *t = tuple_new_0();

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
        object_t *s1 = string_new("a");
        object_t *s2 = string_new("hello");
        object_t *s3 = string_new("The quick brown fox jumps over the lazy dog.");

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
 * @brief Adversarial test: verify object_len resolves immediately without recursion
 * on self-referencing and mutually cyclic containers.
 */
munit_case(
    RUN,
    test_object_len_cyclic_containers,
    {
        vm_new();

        // 1. Self-referencing cycle: list pointing to itself
        object_t *self_list = list_new(1);
        list_set(self_list, 0, self_list);
        assert_int64(object_len(self_list), ==, 1);

        // 2. Mutual cycle: list A <-> list B
        object_t *list_a = list_new(2);
        object_t *list_b = list_new(3);
        list_set(list_a, 0, list_b);
        list_set(list_b, 0, list_a);

        assert_int64(object_len(list_a), ==, 2);
        assert_int64(object_len(list_b), ==, 3);

        // Discard local roots so cycles are unreferenced from outside
        object_refcount_dec(self_list);
        object_refcount_dec(list_a);
        object_refcount_dec(list_b);

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
        object_t *s = string_new("deep");
        object_t *l = list_new(1);
        list_set(l, 0, s);
        object_t *inner_tuple = tuple_new_1(l);
        object_t *outer_tuple = tuple_new_2(inner_tuple, s);

        assert_int64(object_len(outer_tuple), ==, 2);
        assert_int64(object_len(inner_tuple), ==, 1);
        assert_int64(object_len(l), ==, 1);
        assert_int64(object_len(s), ==, 4);

        // 2. Large list sequence
        const size_t large_size = 2000;
        object_t *large_list = list_new(large_size);
        assert_int64(object_len(large_list), ==, (int64_t)large_size);

        // 3. Large tuple sequence
        const size_t tuple_size = 50;
        object_t *items[50];
        for (size_t idx = 0; idx < tuple_size; idx++)
        {
            items[idx] = s;
        }
        object_t *large_tuple = tuple_new(items, tuple_size);
        assert_int64(object_len(large_tuple), ==, (int64_t)tuple_size);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test object_is_immortal predicate on NULL, mortal objects, and immortals.
 */
munit_case(
    RUN,
    test_object_is_immortal_predicate,
    {
        assert_false(object_is_immortal(NULL));

        vm_new();
        object_t *i = integer_new(10);
        object_t *f = float_new(2.5f);
        object_t *s = string_new("mortal");
        object_t *l = list_new(1);
        object_t *none = none_get();
        object_t *empty_t = tuple_new_0();

        assert_false(object_is_immortal(i));
        assert_false(object_is_immortal(f));
        assert_false(object_is_immortal(s));
        assert_false(object_is_immortal(l));

        assert_true(object_is_immortal(none));
        assert_true(object_is_immortal(empty_t));

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
        assert_null(integer_new(1));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(0);
        assert_null(float_new(1.0f));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(0);
        assert_null(string_new("test"));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(1);
        assert_null(string_new("test"));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(0);
        assert_null(list_new(5));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(1);
        assert_null(list_new(5));
        assert_true(boot_fail_alloc_triggered());

        object_t *x = integer_new(1);
        object_t *y = integer_new(2);
        object_t *z = integer_new(3);

        boot_set_fail_alloc_after(0);
        assert_null(create_empty_tuple_singleton());
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(1);
        assert_null(create_empty_tuple_singleton());
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(0);
        assert_null(none_create());
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(0);
        assert_null(tuple_new_1(x));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(1);
        assert_null(tuple_new_1(x));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(0);
        assert_null(tuple_new_2(x, y));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(1);
        assert_null(tuple_new_2(x, y));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(0);
        assert_null(tuple_new_3(x, y, z));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(1);
        assert_null(tuple_new_3(x, y, z));
        assert_true(boot_fail_alloc_triggered());

        object_t *items[3];
        items[0] = x;
        items[1] = y;
        items[2] = z;
        boot_set_fail_alloc_after(0);
        assert_null(tuple_new(items, 3));
        assert_true(boot_fail_alloc_triggered());

        boot_set_fail_alloc_after(1);
        assert_null(tuple_new(items, 3));
        assert_true(boot_fail_alloc_triggered());

        // Persistent OOM failure simulation: every allocation fails
        boot_set_fail_alloc_repeat(0, -1);
        assert_null(integer_new(100));
        assert_null(float_new(2.0f));
        assert_null(string_new("oom"));
        assert_null(list_new(10));
        assert_null(tuple_new_1(x));
        assert_size(boot_fail_alloc_injected_count(), >=, 5);
        boot_reset_fail_alloc();

        vm_free();
        assert(boot_all_freed());
    }
);

MunitTest object_tests[] = {
    munit_test("/field_exists", test_field_exists),
    munit_test("/marked_is_false", test_marked_is_false),
    munit_test("/integer_constant", test_integer_constant),
    munit_test("/integer_obj", test_integer_obj),
    munit_test("/integer_positive", test_positive_integer),
    munit_test("/integer_zero", test_zero_integer),
    munit_test("/integer_negative", test_negative_integer),
    munit_test("/float_object", test_float_object),
    munit_test("/string_object", test_string_object),
    munit_test("/add_integers", test_add_integers),
    munit_test("/add_integer_and_float", test_add_integer_and_float),
    munit_test("/add_floats", test_add_floats),
    munit_test("/add_strings", test_add_strings),
    munit_test("/add_strings_alloc_failure", test_add_strings_alloc_failure),
    munit_test("/add_invalid_mismatched", test_add_invalid_mismatched),
    munit_test("/len_null", test_object_len_null),
    munit_test("/len_non_sequence", test_object_len_non_sequence),
    munit_test("/len_empty_containers", test_object_len_empty_containers),
    munit_test("/len_strings", test_object_len_strings),
    munit_test("/len_cyclic_containers", test_object_len_cyclic_containers),
    munit_test("/len_deep_and_large", test_object_len_deep_and_large_sequences),
    munit_test("/object_is_immortal_predicate", test_object_is_immortal_predicate),
    munit_test("/alloc_failures", test_alloc_failures),
    munit_null_test,
};
