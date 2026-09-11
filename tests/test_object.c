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
        object_t *res = add(a, b);

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
        object_t *res1 = add(a, b);
        object_t *res2 = add(b, a);

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
        object_t *res = add(a, b);

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
        object_t *res = add(a, b);

        assert_not_null(res);
        assert_int(res->kind, ==, STRING);
        assert_string_equal(res->data.v_string, "Hello World!");

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test component-wise addition of 3D vector objects.
 */
munit_case(
    RUN,
    test_add_vectors,
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

        object_t *res = add(v1, v2);

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

        object_t *res = add(arr1, arr2);

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

        assert_null(add(NULL, i));
        assert_null(add(i, NULL));
        assert_null(add(i, s));
        assert_null(add(f, s));
        assert_null(add(s, i));
        assert_null(add(v, i));
        assert_null(add(a, i));

        object_t invalid_obj = {.kind = (object_kind_t)999};
        assert_null(add(&invalid_obj, i));

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
    munit_test("/add_vectors", test_add_vectors),
    munit_test("/add_lists", test_add_lists),
    munit_test("/add_invalid_mismatched", test_add_invalid_mismatched),
    munit_null_test,
};
