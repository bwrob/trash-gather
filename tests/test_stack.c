#include "bootlib.h"
#include "munit.h"
#include "stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static void scary_double_push(stack_t *s) {
  stack_push(s, (void *)(uintptr_t)1337);
  int *p = malloc(sizeof(int));
  *p = 1024;
  stack_push(s, p);
}

munit_case(RUN, create_stack_small, {
  stack_t *s = stack_new(3);
  assert_int(s->capacity, ==, 3, "Sets capacity to 3");
  assert_int(s->count, ==, 0, "No elements in the stack yet");
  assert_ptr_not_null(s->data, "Allocates the stack data");

  stack_free(s);
  assert(boot_all_freed());
});

munit_case(SUBMIT, create_stack_large, {
  stack_t *s = stack_new(100);
  assert_int(s->capacity, ==, 100, "Sets capacity to 100");
  assert_int(s->count, ==, 0, "No elements in the stack yet");
  assert_ptr_not_null(s->data, "Allocates the stack data");

  stack_free(s);
  assert(boot_all_freed());
});

munit_case(SUBMIT, create_stack_allocation_size, {
  size_t capacity = 5;
  stack_t *s = stack_new(capacity);
  assert_int(s->capacity, ==, capacity, "Sets capacity to 5");
  assert_int(s->count, ==, 0, "No elements in the stack yet");
  assert_ptr_not_null(s->data, "Allocates the stack data");
  assert_size(boot_alloc_size(), ==, sizeof(stack_t) + capacity * sizeof(void *),
              "Allocates memory for one stack and the stack data");

  stack_free(s);
  assert(boot_all_freed());
});

munit_case(RUN, push_stack, {
  stack_t *s = stack_new(2);
  assert_ptr_not_null(s, "Must allocate a new stack");

  assert_int(s->capacity, ==, 2, "Sets capacity to 2");
  assert_int(s->count, ==, 0, "No elements in the stack yet");
  assert_ptr_not_null(s->data, "Allocates the stack data");

  int a = 1;

  stack_push(s, &a);
  stack_push(s, &a);

  assert_int(s->capacity, ==, 2, "Sets capacity to 2");
  assert_int(s->count, ==, 2, "2 elements in the stack");
  assert_ptr_equal(s->data[0], &a, "element inserted into stack");

  stack_free(s);
  assert(boot_all_freed());
});

munit_case(RUN, push_double_capacity, {
  stack_t *s = stack_new(2);
  assert_ptr_not_null(s, "Must allocate a new stack");

  assert_int(s->capacity, ==, 2, "Sets capacity to 2");
  assert_int(s->count, ==, 0, "No elements in the stack yet");
  assert_ptr_not_null(s->data, "Allocates the stack data");

  int a = 1;

  stack_push(s, &a);
  stack_push(s, &a);

  assert_int(s->capacity, ==, 2, "Sets capacity to 2");
  assert_int(s->count, ==, 2, "2 elements in the stack");

  stack_push(s, &a);
  assert_int(s->capacity, ==, 4, "Capacity is doubled");
  assert_int(s->count, ==, 3, "3 elements in the stack");

  assert_size(boot_last_realloc_size(), ==, 4 * sizeof(void *),
              "realloc requested correct size");

  assert_int_equal(boot_realloc_count(), 1, "Must reallocate memory for stack");

  stack_free(s);
  assert(boot_all_freed());
});

munit_case(SUBMIT, push_multiple_values, {
  stack_t *s = stack_new(2);
  assert_ptr_not_null(s, "Must allocate a new stack");

  int one = 1;
  int two = 2;
  int three = 3;

  stack_push(s, &one);
  stack_push(s, &two);
  stack_push(s, &three);

  assert_int(s->capacity, ==, 4, "Capacity is doubled");
  assert_int(s->count, ==, 3, "3 elements in the stack");
  assert_ptr_equal(s->data[0], &one, "first element is preserved");
  assert_ptr_equal(s->data[1], &two, "second element is preserved");
  assert_ptr_equal(s->data[2], &three, "third element is inserted");

  stack_free(s);
  assert(boot_all_freed());
});

munit_case(RUN, pop_stack, {
  stack_t *s = stack_new(2);
  assert_ptr_not_null(s, "Must allocate a new stack");

  assert_int(s->capacity, ==, 2, "Sets capacity to 2");
  assert_int(s->count, ==, 0, "No elements in the stack yet");
  assert_ptr_not_null(s->data, "Allocates the stack data");

  int one = 1;
  int two = 2;
  int three = 3;

  stack_push(s, &one);
  stack_push(s, &two);

  assert_int(s->capacity, ==, 2, "Sets capacity to 2");
  assert_int(s->count, ==, 2, "2 elements in the stack");

  stack_push(s, &three);
  assert_int(s->capacity, ==, 4, "Capacity is doubled");
  assert_int(s->count, ==, 3, "3 elements in the stack");

  int *popped = stack_pop(s);
  assert_int(*popped, ==, three, "Should pop the last element");

  popped = stack_pop(s);
  assert_int(*popped, ==, two, "Should pop the last element");

  popped = stack_pop(s);
  assert_int(*popped, ==, one, "Should pop the only remaining element");

  popped = stack_pop(s);
  assert_null(popped, "No remaining elements");

  stack_free(s);
  assert(boot_all_freed());
});

munit_case(SUBMIT, pop_stack_empty, {
  stack_t *s = stack_new(2);
  assert_ptr_not_null(s, "Must allocate a new stack");

  assert_int(s->capacity, ==, 2, "Sets capacity to 2");
  assert_int(s->count, ==, 0, "No elements in the stack yet");
  assert_ptr_not_null(s->data, "Allocates the stack data");

  int *popped = stack_pop(s);
  assert_null(popped, "Should return null when popping an empty stack");

  stack_free(s);
  assert(boot_all_freed());
});

munit_case(RUN, heterogenous_stack, {
  stack_t *s = stack_new(2);
  assert_ptr_not_null(s, "Must allocate a new stack");

  scary_double_push(s);
  assert_int(s->count, ==, 2, "Should have two items in the stack");

  int value = (int)(uintptr_t)s->data[0];
  assert_int(value, ==, 1337, "Zero item should be 1337");

  int *pointer = s->data[1];
  assert_int(*pointer, ==, 1024, "Top item should be 1024");

  free(pointer);
  stack_free(s);
  assert(boot_all_freed());
});

MunitTest stack_tests[] = {
    munit_test("/create_small", create_stack_small),
    munit_test("/create_large", create_stack_large),
    munit_test("/allocation_size", create_stack_allocation_size),
    munit_test("/push", push_stack),
    munit_test("/push_double_capacity", push_double_capacity),
    munit_test("/push_multiple_values", push_multiple_values),
    munit_test("/pop", pop_stack),
    munit_test("/pop_empty", pop_stack_empty),
    munit_test("/heterogenous", heterogenous_stack),
    munit_null_test,
};
