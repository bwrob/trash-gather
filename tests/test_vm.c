/**
 * @file test_vm.c
 * @brief Unit tests for Virtual Machine lifecycle, mark-and-sweep garbage
 * collection passes, and allocation safety.
 */

#include "bootlib.h"
#include "munit.h"
#include "new.h"
#include "object.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Test basic garbage collection pass on a single stack frame.
 */
munit_case(RUN, test_simple, {
  vm_new();
  frame_t *f1 = vm_new_frame();

  object_t *s = new_string("I wish I knew how to read.");
  frame_reference_object(f1, s);
  vm_collect_garbage();
  // nothing should be collected because
  // we haven't freed the frame
  assert(!boot_is_freed(s));

  frame_free(vm_frame_pop());
  vm_collect_garbage();
  assert_true(boot_is_freed(s));

  vm_free();
  assert_true(boot_all_freed());
});

/**
 * @brief Test full mark-and-sweep garbage collection across multiple stack
 * frames and nested objects.
 */
munit_case(SUBMIT, test_full, {
  vm_new();
  vm_t *vm = vm_get_current();
  frame_t *f1 = vm_new_frame();
  frame_t *f2 = vm_new_frame();
  frame_t *f3 = vm_new_frame();

  object_t *s1 = new_string("This string is going into frame 1");
  frame_reference_object(f1, s1);

  object_t *s2 = new_string("This string is going into frame 2");
  frame_reference_object(f2, s2);

  object_t *s3 = new_string("This string is going into frame 3");
  frame_reference_object(f3, s3);

  object_t *i1 = new_integer(69);
  object_t *i2 = new_integer(420);
  object_t *i3 = new_integer(1337);
  object_t *v = new_vector3(i1, i2, i3);
  frame_reference_object(f2, v);
  frame_reference_object(f3, v);

  assert_int(vm->objects->count, ==, 7,
             "Correct number of objects in the VM before GC");

  // only free the top frame (f3)
  frame_free(vm_frame_pop());
  vm_collect_garbage();
  assert_true(boot_is_freed(s3));
  assert_false(boot_is_freed(s1));
  assert_false(boot_is_freed(s2));

  // VM pass should free the string, but not the vector
  // because its final frame hasn't been freed
  frame_free(vm_frame_pop());
  frame_free(vm_frame_pop());
  vm_collect_garbage();
  assert_true(boot_is_freed(s1));
  assert_true(boot_is_freed(s2));
  assert_true(boot_is_freed(s3));
  assert_true(boot_is_freed(v));
  assert_true(boot_is_freed(i1));
  assert_true(boot_is_freed(i2));
  assert_true(boot_is_freed(i3));

  assert_int(vm->objects->count, ==, 0, "No live objects remaining");

  vm_free();
  assert_true(boot_all_freed());
});

/**
 * @brief Test automatic cleanup of unreferenced objects when vm_free is called.
 */
munit_case(RUN, test_reference_object, {
  vm_new();
  new_integer(5);
  new_string("hello");
  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test list object deallocation during vm_free.
 */
munit_case(RUN, test_list_freed, {
  vm_new();
  new_list(3);
  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test stack frame deallocation during vm_free.
 */
munit_case(SUBMIT, test_frames_are_freed, {
  vm_new();
  vm_new_frame();
  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test virtual machine initialization and internal stack/object pool
 * allocation.
 */
munit_case(RUN, test_vm_new, {
  vm_new();
  vm_t *vm = vm_get_current();
  assert_ptr_not_null(vm->frames, "frames must not be NULL");
  assert_ptr_not_null(vm->objects, "objects must not be NULL");
  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test object tracking registration upon creation in VM pool.
 */
munit_case(RUN, test_new_object, {
  vm_new();
  vm_t *vm = vm_get_current();
  object_t *obj = new_integer(5);
  assert_int(obj->kind, ==, INTEGER, "kind must be INTEGER");
  assert_ptr_equal(vm->objects->data[0], obj, "object must be tracked");
  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test virtual machine allocation failure simulation.
 */
munit_case(RUN, test_vm_alloc_failures, {
  for (int i = 0; i <= 4; i++) {
    boot_set_fail_alloc_after(i);
    vm_new();
    assert_null(vm_get_current());
  }

  vm_new();
  object_t *obj = new_integer(42);
  frame_t *f = vm_new_frame();
  frame_reference_object(f, obj);
  mark();

  boot_set_fail_alloc_after(0);
  trace();

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Adversarial test: GC reclaims isolated circular reference cycle.
 */
munit_case(RUN, test_gc_reclaims_unreachable_cycle, {
  vm_new();
  vm_t *vm = vm_get_current();
  frame_t *f = vm_new_frame();

  object_t *arr_a = new_list(1);
  object_t *arr_b = new_list(1);
  frame_reference_object(f, arr_a);
  frame_reference_object(f, arr_b);

  // Form cycle: A -> B and B -> A
  list_set(arr_a, 0, arr_b);
  list_set(arr_b, 0, arr_a);

  // Pop and free the only frame referencing the cycle
  frame_free(vm_frame_pop());

  // Both objects have refcount == 1 due to the mutual cycle,
  // but are completely unreachable from any frame.
  assert_false(boot_is_freed(arr_a));
  assert_false(boot_is_freed(arr_b));

  vm_collect_garbage();

  // Cycle must be reclaimed by the hybrid collector
  assert_true(boot_is_freed(arr_a));
  assert_true(boot_is_freed(arr_b));
  assert_size(vm->objects->count, ==, 0);

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Adversarial test: GC reclaims self-referencing object cycle.
 */
munit_case(RUN, test_gc_reclaims_self_referencing_cycle, {
  vm_new();
  vm_t *vm = vm_get_current();
  frame_t *f = vm_new_frame();

  object_t *self_arr = new_list(1);
  frame_reference_object(f, self_arr);
  list_set(self_arr, 0, self_arr);

  frame_free(vm_frame_pop());
  assert_false(boot_is_freed(self_arr));

  vm_collect_garbage();

  assert_true(boot_is_freed(self_arr));
  assert_size(vm->objects->count, ==, 0);

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Adversarial test: dead cycle referencing a live rooted object.
 * Verifies that the dead cycle is collected while the live object survives with
 * properly decremented reference count.
 */
munit_case(RUN, test_gc_dead_cycle_pointing_to_live_object, {
  vm_new();
  frame_t *live_frame = vm_new_frame();
  object_t *live_str = new_string("survivor");
  frame_reference_object(live_frame, live_str);

  frame_t *dead_frame = vm_new_frame();
  object_t *a = new_list(2);
  object_t *b = new_list(1);
  frame_reference_object(dead_frame, a);
  frame_reference_object(dead_frame, b);

  list_set(a, 0, b);
  list_set(b, 0, a);
  list_set(a, 1, live_str); // dead container holds reference to live object

  // Pop and destroy the dead frame
  frame_free(vm_frame_pop());

  vm_collect_garbage();

  assert_true(boot_is_freed(a));
  assert_true(boot_is_freed(b));
  assert_false(boot_is_freed(live_str));
  // Live string lost the dead container's reference, so refcount is back to 2
  assert_size(live_str->refcount, ==, 2);

  frame_free(vm_frame_pop());
  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Adversarial test: unreachable list containing NULL slots collected safely.
 */
munit_case(RUN, test_gc_list_with_null_slots, {
  vm_new();
  frame_t *f = vm_new_frame();
  object_t *arr = new_list(5);
  frame_reference_object(f, arr);

  // Set only slots 0 and 3; slots 1, 2, 4 remain NULL
  object_t *val0 = new_integer(100);
  object_t *val3 = new_integer(300);
  list_set(arr, 0, val0);
  list_set(arr, 3, val3);

  frame_free(vm_frame_pop());
  vm_collect_garbage();

  assert_true(boot_is_freed(arr));
  assert_true(boot_is_freed(val0));
  assert_true(boot_is_freed(val3));

  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Adversarial test: multi-node cycle mesh (triangle cycle plus tail).
 */
munit_case(RUN, test_gc_cycle_mesh_with_tail, {
  vm_new();
  frame_t *f = vm_new_frame();

  object_t *n1 = new_list(1);
  object_t *n2 = new_list(1);
  object_t *n3 = new_list(2);
  object_t *tail = new_integer(999);

  frame_reference_object(f, n1);
  frame_reference_object(f, n2);
  frame_reference_object(f, n3);
  frame_reference_object(f, tail);

  // n1 -> n2 -> n3 -> n1 (triangle cycle)
  list_set(n1, 0, n2);
  list_set(n2, 0, n3);
  list_set(n3, 0, n1);
  // n3 also references tail
  list_set(n3, 1, tail);

  frame_free(vm_frame_pop());
  vm_collect_garbage();

  assert_true(boot_is_freed(n1));
  assert_true(boot_is_freed(n2));
  assert_true(boot_is_freed(n3));
  assert_true(boot_is_freed(tail));

  vm_free();
  assert(boot_all_freed());
});

MunitTest vm_tests[] = {
    munit_test("/simple", test_simple),
    munit_test("/full", test_full),
    munit_test("/reference_object", test_reference_object),
    munit_test("/list_freed", test_list_freed),
    munit_test("/frames_are_freed", test_frames_are_freed),
    munit_test("/vm_new", test_vm_new),
    munit_test("/new_object", test_new_object),
    munit_test("/vm_alloc_failures", test_vm_alloc_failures),
    munit_test("/gc_reclaims_unreachable_cycle", test_gc_reclaims_unreachable_cycle),
    munit_test("/gc_reclaims_self_referencing_cycle",
               test_gc_reclaims_self_referencing_cycle),
    munit_test("/gc_dead_cycle_pointing_to_live_object",
               test_gc_dead_cycle_pointing_to_live_object),
    munit_test("/gc_list_with_null_slots", test_gc_list_with_null_slots),
    munit_test("/gc_cycle_mesh_with_tail", test_gc_cycle_mesh_with_tail),
    munit_null_test,
};
