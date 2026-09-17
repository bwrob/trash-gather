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
munit_case(
    RUN,
    test_simple,
    {
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
    }
);

/**
 * @brief Test full mark-and-sweep garbage collection across multiple stack
 * frames and nested objects.
 */
munit_case(
    SUBMIT,
    test_full,
    {
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
        object_t *v = new_tuple_3(i1, i2, i3);
        frame_reference_object(f2, v);
        frame_reference_object(f3, v);

        assert_int(
            vm->objects->count, ==, 7, "Correct number of objects in the VM before GC"
        );

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
    }
);

/**
 * @brief Test automatic cleanup of unreferenced objects when vm_free is called.
 */
munit_case(
    RUN,
    test_reference_object,
    {
        vm_new();
        new_integer(5);
        new_string("hello");
        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test list object deallocation during vm_free.
 */
munit_case(
    RUN,
    test_list_freed,
    {
        vm_new();
        new_list(3);
        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test stack frame deallocation during vm_free.
 */
munit_case(
    SUBMIT,
    test_frames_are_freed,
    {
        vm_new();
        vm_new_frame();
        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test virtual machine initialization and internal stack/object pool
 * allocation.
 */
munit_case(
    RUN,
    test_vm_new,
    {
        vm_new();
        vm_t *vm = vm_get_current();
        assert_ptr_not_null(vm->frames, "frames must not be NULL");
        assert_ptr_not_null(vm->objects, "objects must not be NULL");
        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test object tracking registration upon creation in VM pool.
 */
munit_case(
    RUN,
    test_new_object,
    {
        vm_new();
        vm_t *vm = vm_get_current();
        object_t *obj = new_integer(5);
        assert_int(obj->kind, ==, INTEGER, "kind must be INTEGER");
        assert_ptr_equal(vm->objects->data[0], obj, "object must be tracked");
        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test virtual machine allocation failure simulation.
 */
munit_case(
    RUN,
    test_vm_alloc_failures,
    {
        for (int i = 0; i <= 7; i++)
        {
            boot_set_fail_alloc_after(i);
            vm_new();
            assert_null(vm_get_current());
            assert_true(boot_fail_alloc_triggered());
        }

        vm_new();
        object_t *obj = new_integer(42);
        frame_t *f = vm_new_frame();
        frame_reference_object(f, obj);
        mark();

        boot_set_fail_alloc_after(0);
        trace();
        assert_true(boot_fail_alloc_triggered());

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial test: GC reclaims isolated circular reference cycle.
 */
munit_case(
    RUN,
    test_gc_reclaims_unreachable_cycle,
    {
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
    }
);

/**
 * @brief Adversarial test: GC reclaims self-referencing object cycle.
 */
munit_case(
    RUN,
    test_gc_reclaims_self_referencing_cycle,
    {
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
    }
);

/**
 * @brief Adversarial test: dead cycle referencing a live rooted object.
 * Verifies that the dead cycle is collected while the live object survives with
 * properly decremented reference count.
 */
munit_case(
    RUN,
    test_gc_dead_cycle_pointing_to_live_object,
    {
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
    }
);

/**
 * @brief Adversarial test: unreachable list containing NULL slots collected safely.
 */
munit_case(
    RUN,
    test_gc_list_with_null_slots,
    {
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
    }
);

/**
 * @brief Adversarial test: multi-node cycle mesh (triangle cycle plus tail).
 */
munit_case(
    RUN,
    test_gc_cycle_mesh_with_tail,
    {
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
    }
);

/**
 * @brief Adversarial test: GC reclaims a circular reference cycle between a list and a
 * tuple.
 */
munit_case(
    RUN,
    test_gc_cycle_with_tuple,
    {
        vm_new();
        frame_t *f = vm_new_frame();

        object_t *list = new_list(1);
        object_t *tup = new_tuple_1(list);
        list_set(list, 0, tup);

        frame_reference_object(f, list);
        frame_reference_object(f, tup);

        // Pop frame, making the mutual cycle completely unreachable
        frame_free(vm_frame_pop());

        // Both objects have non-zero refcounts due to the mutual cycle,
        // but neither is reachable from any root frame.
        assert_false(boot_is_freed(list));
        assert_false(boot_is_freed(tup));

        vm_collect_garbage();

        assert_true(boot_is_freed(list));
        assert_true(boot_is_freed(tup));

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial test: verify mark, trace, sweep, and frame_free handle
 * NULL slots in frame and object lists.
 */
munit_case(
    RUN,
    test_vm_null_slots_in_frames_and_objects,
    {
        vm_new();
        frame_t *f = vm_new_frame();

        // Push NULL into frame references
        stack_push(f->references, NULL);

        // Push NULL into VM objects list
        stack_push(vm_get_current()->objects, NULL);

        // mark() encounters NULL in frame references
        mark();

        // trace() encounters NULL in objects list
        trace();

        // sweep() encounters NULL in objects list during pass 2
        sweep();

        // Remove the NULL from objects list so clean teardown succeeds
        stack_pop(vm_get_current()->objects);

        // frame_free() encounters NULL in frame references
        frame_free(vm_frame_pop());

        // vm_untrack_object safely handles NULL
        vm_untrack_object(NULL);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that None singleton survives unrooted garbage collection passes.
 */
munit_case(
    RUN,
    test_none_survives_gc_sweep,
    {
        vm_new();
        object_t *none1 = new_none();
        assert_not_null(none1);

        // Run garbage collection when None has no frame references
        vm_collect_garbage();

        assert(!boot_is_freed(none1));
        object_t *none2 = new_none();
        assert_ptr_equal(none1, none2);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that empty tuple singleton survives unrooted garbage collection passes.
 */
munit_case(
    RUN,
    test_empty_tuple_survives_gc_sweep,
    {
        vm_new();
        object_t *t1 = new_tuple_0();
        assert_not_null(t1);

        // Run garbage collection when empty tuple has no frame references
        vm_collect_garbage();

        assert(!boot_is_freed(t1));
        object_t *t2 = new_tuple_0();
        assert_ptr_equal(t1, t2);

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that a reference cycle referencing None is reclaimed while None survives.
 */
munit_case(
    RUN,
    test_none_in_cycle_reclaimed,
    {
        vm_new();
        object_t *none = new_none();

        // Construct cyclic mesh A <-> B where A also holds None
        object_t *list_a = new_list(2);
        object_t *list_b = new_list(1);

        list_set(list_a, 0, list_b);
        list_set(list_a, 1, none);
        list_set(list_b, 0, list_a);

        // Cycle is unrooted (not in any frame)
        vm_collect_garbage();

        assert_true(boot_is_freed(list_a));
        assert_true(boot_is_freed(list_b));
        assert(!boot_is_freed(none));

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test consecutive VM instances cleanly allocate and destroy immortals without
 * leaks.
 */
munit_case(
    RUN,
    test_sequential_vm_lifecycles_with_immortals,
    {
        for (int round = 0; round < 3; round++)
        {
            vm_new();
            object_t *none = new_none();
            object_t *t0 = new_tuple_0();
            assert_not_null(none);
            assert_not_null(t0);

            vm_free();
            assert(boot_all_freed());
        }
    }
);

/**
 * @brief Test referencing None and empty tuple across multiple stack frames and popping
 * them.
 */
munit_case(
    RUN,
    test_frame_stack_immortal_churn,
    {
        vm_new();
        object_t *none = new_none();
        object_t *t0 = new_tuple_0();

        const size_t num_frames = 5;
        for (size_t i = 0; i < num_frames; i++)
        {
            frame_t *frame = vm_new_frame();
            frame_reference_object(frame, none);
            frame_reference_object(frame, t0);
        }

        // Run GC while referenced on frames
        vm_collect_garbage();
        assert(!boot_is_freed(none));
        assert(!boot_is_freed(t0));

        // Pop and free all frames
        for (size_t i = 0; i < num_frames; i++)
        {
            frame_t *frame = vm_frame_pop();
            frame_free(frame);
        }

        // Run GC after all frames are popped
        vm_collect_garbage();
        assert(!boot_is_freed(none));
        assert(!boot_is_freed(t0));

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that VM operations safely no-op or return NULL when CURRENT_VM is NULL.
 */
munit_case(
    RUN,
    test_vm_operations_without_active_vm,
    {
        assert_null(vm_get_current());

        // Calling GC and VM routines when no VM is active must safely return without
        // crashing
        vm_free();
        mark();
        trace();
        sweep();
        vm_collect_garbage();

        vm_frame_push(NULL);
        assert_null(vm_frame_pop());

        vm_track_object(NULL);
        vm_untrack_object(NULL);

        assert_null(vm_get_empty_tuple());
        assert_null(vm_get_none());

        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial test: Dead cycle mesh containing immortals (None and ()) pointing
 * to a live external tail. Verifies cycle collection, live tail survival, refcount
 * adjustment, and immortal stability.
 */
munit_case(
    RUN,
    test_gc_mesh_cycle_with_immortals,
    {
        vm_new();
        object_t *none = new_none();
        object_t *t0 = new_tuple_0();

        // Node A: holds B and None
        object_t *node_a = new_list(2);
        // Node B: holds C and ()
        object_t *node_b = new_list(2);
        // Node C: holds A, None, and live_tail
        object_t *node_c = new_list(3);

        object_t *live_tail = new_integer(999);

        // Frame roots only the live_tail
        frame_t *frame = vm_new_frame();
        frame_reference_object(frame, live_tail);

        list_set(node_a, 0, node_b);
        list_set(node_a, 1, none);

        list_set(node_b, 0, node_c);
        list_set(node_b, 1, t0);

        list_set(node_c, 0, node_a);
        list_set(node_c, 1, none);
        list_set(node_c, 2, live_tail);

        // Drop caller's local reference so live_tail is held only by frame and node_c
        refcount_dec(live_tail);
        assert_int(live_tail->refcount, ==, 2);

        // Run GC: cycle A-B-C is unrooted and must be collected
        vm_collect_garbage();

        assert_true(boot_is_freed(node_a));
        assert_true(boot_is_freed(node_b));
        assert_true(boot_is_freed(node_c));

        // live_tail must survive and refcount should drop from 2 to 1
        assert_false(boot_is_freed(live_tail));
        assert_int(live_tail->refcount, ==, 1);

        // Immortals must survive untouched
        assert_false(boot_is_freed(none));
        assert_false(boot_is_freed(t0));
        assert_size(none->refcount, ==, OBJECT_IMMORTAL_REFCOUNT);
        assert_size(t0->refcount, ==, OBJECT_IMMORTAL_REFCOUNT);

        // Now pop the frame rooting live_tail
        frame_free(vm_frame_pop());

        // Second GC sweep reclaims live_tail
        vm_collect_garbage();
        assert_true(boot_is_freed(live_tail));

        assert_false(boot_is_freed(none));
        assert_false(boot_is_freed(t0));

        vm_free();
        assert(boot_all_freed());
    }
);

/**
 * @brief Adversarial test: VM initialization under persistent out-of-memory failure.
 */
munit_case(
    RUN,
    test_vm_persistent_alloc_failure,
    {
        boot_set_fail_alloc_repeat(0, -1);
        vm_new();
        assert_null(vm_get_current());
        assert_true(boot_fail_alloc_triggered());
        boot_reset_fail_alloc();

        assert(boot_all_freed());
    }
);

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
    munit_test(
        "/gc_reclaims_self_referencing_cycle",
        test_gc_reclaims_self_referencing_cycle
    ),
    munit_test(
        "/gc_dead_cycle_pointing_to_live_object",
        test_gc_dead_cycle_pointing_to_live_object
    ),
    munit_test("/gc_list_with_null_slots", test_gc_list_with_null_slots),
    munit_test("/gc_cycle_mesh_with_tail", test_gc_cycle_mesh_with_tail),
    munit_test("/gc_cycle_with_tuple", test_gc_cycle_with_tuple),
    munit_test(
        "/null_slots_in_frames_and_objects",
        test_vm_null_slots_in_frames_and_objects
    ),
    munit_test("/none_survives_gc_sweep", test_none_survives_gc_sweep),
    munit_test("/empty_tuple_survives_gc_sweep", test_empty_tuple_survives_gc_sweep),
    munit_test("/none_in_cycle_reclaimed", test_none_in_cycle_reclaimed),
    munit_test(
        "/sequential_vm_lifecycles_with_immortals",
        test_sequential_vm_lifecycles_with_immortals
    ),
    munit_test("/frame_stack_immortal_churn", test_frame_stack_immortal_churn),
    munit_test("/operations_without_active_vm", test_vm_operations_without_active_vm),
    munit_test("/gc_mesh_cycle_with_immortals", test_gc_mesh_cycle_with_immortals),
    munit_test("/persistent_alloc_failure", test_vm_persistent_alloc_failure),
    munit_null_test,
};
