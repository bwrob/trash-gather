/**
 * @file test_vm.c
 * @brief Unit tests for Virtual Machine lifecycle, mark-and-sweep garbage
 * collection passes, and allocation safety.
 */

#include "bootlib.h"
#include "munit.h"
#include "sneknew.h"
#include "snekobject.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Test basic garbage collection pass on a single stack frame.
 */
munit_case(RUN, test_simple, {
  vm_new();
  frame_t *f1 = vm_new_frame();

  snek_object_t *s = new_snek_string("I wish I knew how to read.");
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

  snek_object_t *s1 = new_snek_string("This string is going into frame 1");
  frame_reference_object(f1, s1);

  snek_object_t *s2 = new_snek_string("This string is going into frame 2");
  frame_reference_object(f2, s2);

  snek_object_t *s3 = new_snek_string("This string is going into frame 3");
  frame_reference_object(f3, s3);

  snek_object_t *i1 = new_snek_integer(69);
  snek_object_t *i2 = new_snek_integer(420);
  snek_object_t *i3 = new_snek_integer(1337);
  snek_object_t *v = new_snek_vector3(i1, i2, i3);
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
  new_snek_integer(5);
  new_snek_string("hello");
  vm_free();
  assert(boot_all_freed());
});

/**
 * @brief Test array object deallocation during vm_free.
 */
munit_case(RUN, test_array_freed, {
  vm_new();
  new_snek_array(3);
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
  snek_object_t *obj = new_snek_integer(5);
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
  snek_object_t *obj = new_snek_integer(42);
  frame_t *f = vm_new_frame();
  frame_reference_object(f, obj);
  mark();

  boot_set_fail_alloc_after(0);
  trace();

  vm_free();
  assert(boot_all_freed());
});

MunitTest vm_tests[] = {
    munit_test("/simple", test_simple),
    munit_test("/full", test_full),
    munit_test("/reference_object", test_reference_object),
    munit_test("/array_freed", test_array_freed),
    munit_test("/frames_are_freed", test_frames_are_freed),
    munit_test("/vm_new", test_vm_new),
    munit_test("/new_object", test_new_object),
    munit_test("/vm_alloc_failures", test_vm_alloc_failures),
    munit_null_test,
};
