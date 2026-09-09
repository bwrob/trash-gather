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
static void vm_cleanup_after_refcount(void) {
  vm_t *vm = vm_get_current();
  if (vm == NULL) {
    return;
  }
  if (vm->objects != NULL) {
    vm->objects->count = 0;
  }
  vm_free();
}

/**
 * @brief Test that new integer objects initialize with refcount 1.
 */
munit_case(RUN, test_int_has_refcount, {
  vm_new();
  object_t *obj = new_integer(10);
  assert_int(obj->refcount, ==, 1, "Refcount should be 1 on creation");

  refcount_dec(obj);
  vm_cleanup_after_refcount();
  assert(boot_all_freed());
});

/**
 * @brief Test incrementing object reference count.
 */
munit_case(RUN, test_inc_refcount, {
  vm_new();
  object_t *obj = new_float(4.20f);
  assert_int(obj->refcount, ==, 1, "Refcount should be 1 on creation");

  refcount_inc(obj);
  assert_int(obj->refcount, ==, 2, "Refcount should be incremented");

  refcount_dec(obj);
  refcount_dec(obj);
  vm_cleanup_after_refcount();
  assert(boot_all_freed());
});

/**
 * @brief Test decrementing object reference count without freeing.
 */
munit_case(RUN, test_dec_refcount, {
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
});

/**
 * @brief Test that refcount reaching 0 automatically frees the object.
 */
munit_case(RUN, test_refcount_free_is_called, {
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
});

/**
 * @brief Test that heap-allocated string buffers are freed when refcount drops
 * to 0.
 */
munit_case(RUN, test_allocated_string_is_freed, {
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
});

/**
 * @brief Test that assigning to an list increments the new element refcount.
 */
munit_case(RUN, test_list_set, {
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
});

/**
 * @brief Test that overwriting an list element decrements the old element's
 * refcount.
 */
munit_case(SUBMIT, test_list_free, {
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
});

/**
 * @brief Test vector reference counting across dimensions and nested free.
 */
munit_case(RUN, test_vector3_refcounting, {
  vm_new();
  object_t *foo = new_integer(1);
  object_t *bar = new_integer(2);
  object_t *baz = new_integer(3);

  object_t *vec = new_vector3(foo, bar, baz);
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
});

/**
 * @brief Adversarial test: NULL pointer safety for refcount operations.
 */
munit_case(RUN, test_refcount_null_safety, {
  // Must safely no-op without crashing
  refcount_inc(NULL);
  refcount_dec(NULL);
  assert(boot_all_freed());
});

/**
 * @brief Adversarial test: circular reference cycle cannot be freed by RC
 * alone.
 */
munit_case(RUN, test_cycle_refcount_limitation, {
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
});

MunitTest refcount_tests[] = {
    munit_test("/int_has_refcount", test_int_has_refcount),
    munit_test("/inc_refcount", test_inc_refcount),
    munit_test("/dec_refcount", test_dec_refcount),
    munit_test("/refcount_free", test_refcount_free_is_called),
    munit_test("/string_freed", test_allocated_string_is_freed),
    munit_test("/list_set", test_list_set),
    munit_test("/list_free", test_list_free),
    munit_test("/vector3_refcounting", test_vector3_refcounting),
    munit_test("/null_safety", test_refcount_null_safety),
    munit_test("/cycle_limitation", test_cycle_refcount_limitation),
    munit_null_test,
};
