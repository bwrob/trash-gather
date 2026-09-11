/**
 * @file test_refcount.c
 * @brief Unit tests for reference counting increments, decrements, list/vector
 * ownership, and lifecycle management.
 */

#include "bootlib.h"
#include "munit.h"
#include "new.h"
#include "object.h"
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
        object_t *obj = new_integer(10);
        assert_int(obj->refcount, ==, 1, "Refcount should be 1 on creation");

        refcount_dec(obj);
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
        object_t *obj = new_float(4.20f);
        assert_int(obj->refcount, ==, 1, "Refcount should be 1 on creation");

        refcount_inc(obj);
        assert_int(obj->refcount, ==, 2, "Refcount should be incremented");

        refcount_dec(obj);
        refcount_dec(obj);
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
        object_t *obj = new_float(4.20f);

        refcount_inc(obj);
        assert_int(obj->refcount, ==, 2, "Refcount should be incremented");

        refcount_dec(obj);
        assert_int(obj->refcount, ==, 1, "Refcount should be decremented");
        assert(!boot_is_freed(obj));

        refcount_dec(obj);
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
        object_t *obj = new_float(4.20f);

        refcount_inc(obj);
        assert_int(obj->refcount, ==, 2, "Refcount should be incremented");

        refcount_dec(obj);
        assert_int(obj->refcount, ==, 1, "Refcount should be decremented");

        refcount_dec(obj);
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
        object_t *obj = new_string("Hello @wagslane!");

        refcount_inc(obj);
        assert_int(obj->refcount, ==, 2, "Refcount should be incremented");

        refcount_dec(obj);
        assert_int(obj->refcount, ==, 1, "Refcount should be decremented");
        assert_string_equal(obj->data.v_string, "Hello @wagslane!", "references str");

        refcount_dec(obj);
        assert(boot_is_freed(obj));

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that assigning to an list increments the new element refcount.
 */
munit_case(
    RUN,
    test_list_set,
    {
        vm_new();
        object_t *foo = new_integer(1);
        object_t *list = new_list(1);

        list_set(list, 0, foo);
        assert_int(foo->refcount, ==, 2, "foo is now referenced by list");
        assert(!boot_is_freed(foo));

        refcount_dec(foo);
        refcount_dec(list);

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that overwriting an list element decrements the old element's
 * refcount.
 */
munit_case(
    SUBMIT,
    test_list_free,
    {
        vm_new();
        object_t *foo = new_integer(1);
        object_t *bar = new_integer(2);
        object_t *baz = new_integer(3);

        object_t *list = new_list(2);
        list_set(list, 0, foo);
        list_set(list, 1, bar);
        assert_int(foo->refcount, ==, 2, "foo is now referenced by list");
        assert_int(bar->refcount, ==, 2, "bar is now referenced by list");
        assert_int(baz->refcount, ==, 1, "baz is not yet referenced by list");

        // foo is still referenced in the list, so it should not be freed.
        refcount_dec(foo);
        assert(!boot_is_freed(foo));

        // Overwrite index 0 (foo) with baz. foo refcount hits 0 and is freed.
        list_set(list, 0, baz);
        assert(boot_is_freed(foo));

        refcount_dec(bar);
        refcount_dec(baz);
        refcount_dec(list);

        vm_cleanup_after_refcount();
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
        object_t *foo = new_integer(1);
        object_t *bar = new_integer(2);
        object_t *baz = new_integer(3);

        object_t *vec = new_tuple_3(foo, bar, baz);
        assert_int(foo->refcount, ==, 2, "foo is now referenced by vec");
        assert_int(bar->refcount, ==, 2, "bar is now referenced by vec");
        assert_int(baz->refcount, ==, 2, "baz is now referenced by vec");

        // Drop outer references
        refcount_dec(foo);
        assert(!boot_is_freed(foo));

        // Dropping vec drops inner references and frees foo
        refcount_dec(vec);
        assert(boot_is_freed(foo));

        // bar and baz still have outer references
        assert(!boot_is_freed(bar));
        assert(!boot_is_freed(baz));

        refcount_dec(bar);
        refcount_dec(baz);

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
        object_t *leaf1 = new_integer(10);
        object_t *leaf2 = new_string("hello");
        object_t *inner = new_tuple_2(leaf1, leaf2);
        object_t *outer = new_tuple_1(inner);

        assert_int(leaf1->refcount, ==, 2);
        assert_int(leaf2->refcount, ==, 2);
        assert_int(inner->refcount, ==, 2);
        assert_int(outer->refcount, ==, 1);

        // Drop external direct references to inner objects
        refcount_dec(leaf1);
        refcount_dec(leaf2);
        refcount_dec(inner);

        // All inner objects are kept alive solely through the outer tuple
        assert(!boot_is_freed(leaf1));
        assert(!boot_is_freed(leaf2));
        assert(!boot_is_freed(inner));

        // Dropping outer tuple cascades and frees inner tuple and leaves
        refcount_dec(outer);
        assert(boot_is_freed(outer));
        assert(boot_is_freed(inner));
        assert(boot_is_freed(leaf1));
        assert(boot_is_freed(leaf2));

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
        refcount_inc(NULL);
        refcount_dec(NULL);
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
        object_t *arr_a = new_list(1);
        object_t *arr_b = new_list(1);

        // arr_a -> arr_b and arr_b -> arr_a
        list_set(arr_a, 0, arr_b);
        list_set(arr_b, 0, arr_a);

        assert_int(arr_a->refcount, ==, 2, "arr_a referenced by caller and arr_b");
        assert_int(arr_b->refcount, ==, 2, "arr_b referenced by caller and arr_a");

        // Drop external references
        refcount_dec(arr_a);
        refcount_dec(arr_b);

        // Both still have refcount == 1 due to the cycle!
        assert_int(arr_a->refcount, ==, 1, "arr_a trapped in cycle");
        assert_int(arr_b->refcount, ==, 1, "arr_b trapped in cycle");
        assert(!boot_is_freed(arr_a));
        assert(!boot_is_freed(arr_b));

        // Manually break the cycle to clean up memory in this test
        arr_a->data.v_list.elements[0] = NULL;
        refcount_dec(arr_b); // this cascades and frees arr_a and arr_b
        assert(boot_is_freed(arr_a));
        assert(boot_is_freed(arr_b));

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
        object_t *i1 = new_integer(1);
        object_t *i2 = new_integer(2);
        object_t *t1 = new_tuple_2(i1, i2);

        object_t *i3 = new_integer(10);
        object_t *i4 = new_integer(20);
        object_t *t2 = new_tuple_2(i3, i4);

        object_t *res = add(t1, t2);
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
        refcount_dec(i1);
        refcount_dec(i2);
        refcount_dec(t1);

        refcount_dec(i3);
        refcount_dec(i4);
        refcount_dec(t2);

        // Now release the result tuple: should cascade-free res and both child elements
        refcount_dec(res);

        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial test: when tuple addition fails midway (e.g. element type
 * mismatch), partially created elements must be rolled back without leaking.
 */
munit_case(
    RUN,
    test_tuple_add_component_failure_rollback,
    {
        vm_new();
        object_t *i1 = new_integer(1);
        object_t *s1 = new_string("cannot_add_to_int");
        object_t *t1 = new_tuple_2(i1, s1);

        object_t *i2 = new_integer(2);
        object_t *i3 = new_integer(3);
        object_t *t2 = new_tuple_2(i2, i3);

        // Element 0 (1 + 2) succeeds; Element 1 ("cannot_add_to_int" + 3) fails
        object_t *res = add(t1, t2);
        assert_null(res);

        // Release input tuples
        refcount_dec(i1);
        refcount_dec(s1);
        refcount_dec(t1);

        refcount_dec(i2);
        refcount_dec(i3);
        refcount_dec(t2);

        // Any intermediate element allocated for index 0 must have been freed
        vm_cleanup_after_refcount();
        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial test: deeply nested tuple addition where a sub-component
 * fails, verifying that previously created sub-tuples are fully rolled back.
 */
munit_case(
    RUN,
    test_tuple_add_nested_failure_rollback,
    {
        vm_new();
        // Inner tuple 1: (1, 2)
        object_t *i1 = new_integer(1);
        object_t *i2 = new_integer(2);
        object_t *inner1 = new_tuple_2(i1, i2);

        // Inner tuple 2: (3, "unsupported")
        object_t *i3 = new_integer(3);
        object_t *s1 = new_string("unsupported");
        object_t *inner2 = new_tuple_2(i3, s1);

        // Outer tuple A: ((1, 2), (3, "unsupported"))
        object_t *outer_a = new_tuple_2(inner1, inner2);

        // Inner tuple 3: (10, 20)
        object_t *i4 = new_integer(10);
        object_t *i5 = new_integer(20);
        object_t *inner3 = new_tuple_2(i4, i5);

        // Inner tuple 4: (30, 40)
        object_t *i6 = new_integer(30);
        object_t *i7 = new_integer(40);
        object_t *inner4 = new_tuple_2(i6, i7);

        // Outer tuple B: ((10, 20), (30, 40))
        object_t *outer_b = new_tuple_2(inner3, inner4);

        // Outer addition:
        // index 0: add((1, 2), (10, 20)) -> succeeds! (allocates new inner tuple (11,
        // 22)) index 1: add((3, "unsupported"), (30, 40)) -> fails on component 1!
        object_t *res = add(outer_a, outer_b);
        assert_null(res);

        // Decrement inputs
        refcount_dec(i1);
        refcount_dec(i2);
        refcount_dec(inner1);
        refcount_dec(i3);
        refcount_dec(s1);
        refcount_dec(inner2);
        refcount_dec(outer_a);

        refcount_dec(i4);
        refcount_dec(i5);
        refcount_dec(inner3);
        refcount_dec(i6);
        refcount_dec(i7);
        refcount_dec(inner4);
        refcount_dec(outer_b);

        // All intermediate objects (including the successfully allocated (11, 22)
        // tuple) must have been completely freed by the failure rollback!
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
        object_t *elem1 = new_integer(10);
        object_t *elem2 = new_string("hello");
        object_t *elem3 = new_float(3.14f);

        object_t *list_a = new_list(2);
        list_set(list_a, 0, elem1);
        list_set(list_a, 1, elem2);

        object_t *list_b = new_list(1);
        list_set(list_b, 0, elem3);

        object_t *res = add(list_a, list_b);
        assert_not_null(res);
        assert_size(res->data.v_list.size, ==, 3);

        // Each element has 3 references: caller, source list, concatenated list
        assert_int(elem1->refcount, ==, 3);
        assert_int(elem2->refcount, ==, 3);
        assert_int(elem3->refcount, ==, 3);

        // Drop caller references
        refcount_dec(elem1);
        refcount_dec(elem2);
        refcount_dec(elem3);

        // Drop source lists
        refcount_dec(list_a);
        refcount_dec(list_b);

        // Elements alive solely via res (refcount == 1)
        assert_int(elem1->refcount, ==, 1);
        assert_int(elem2->refcount, ==, 1);
        assert_int(elem3->refcount, ==, 1);

        // Dropping res frees res and all 3 elements
        refcount_dec(res);

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
    munit_test("/list_set", test_list_set),
    munit_test("/list_free", test_list_free),
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
    munit_test("/list_add_refcount_lifecycle", test_list_add_refcount_lifecycle),
    munit_test("/null_safety", test_refcount_null_safety),
    munit_test("/cycle_limitation", test_cycle_refcount_limitation),
    munit_null_test,
};
