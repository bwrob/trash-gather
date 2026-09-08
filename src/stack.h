#pragma once

#include <stddef.h>
#include <stdlib.h>

typedef struct Stack {
  size_t count;
  size_t capacity;
  void **data;
} snek_stack_t;

#define stack_t snek_stack_t

snek_stack_t *stack_new(size_t capacity);

void stack_push(snek_stack_t *stack, void *obj);
void *stack_pop(snek_stack_t *stack);

void stack_free(snek_stack_t *stack);
void stack_remove_nulls(snek_stack_t *stack);

