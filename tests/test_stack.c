#include "bootlib.h"
#include "munit.h"
#include "stack.h"
#include <stdio.h>
#include <stdlib.h>

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

MunitTest stack_tests[] = {
    munit_test("/create_small", create_stack_small),
    munit_test("/create_large", create_stack_large),
    munit_test("/allocation_size", create_stack_allocation_size),
    munit_null_test,
};
