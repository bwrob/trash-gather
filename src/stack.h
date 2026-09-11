#pragma once

#include <stddef.h>
#include <stdlib.h>

typedef struct Stack
{
    size_t count;
    size_t capacity;
    void **data;
} vm_stack_t;

#define vm_stack_t vm_stack_t

vm_stack_t *stack_new(
    size_t capacity
);

void stack_push(
    vm_stack_t *stack,
    void *obj
);
void *stack_pop(
    vm_stack_t *stack
);

void stack_free(
    vm_stack_t *stack
);
void stack_remove_nulls(
    vm_stack_t *stack
);
