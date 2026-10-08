#pragma once

#include "vm/stack.h"
#include "vm_objects/object.h"

typedef struct Immortals
{
    object_t *none;
    object_t *empty_tuple;
} immortals_t;

typedef struct VirtualMachine
{
    vm_stack_t *frames;
    vm_stack_t *objects;
    immortals_t immortals;
} vm_t;

typedef struct StackFrame
{
    vm_stack_t *references;
} frame_t;

void vm_mark(
    void
);
void vm_trace(
    void
);
void vm_sweep(
    void
);

void vm_collect_garbage(
    void
);

void trace_blacken_object(
    vm_stack_t *gray_objects,
    object_t *obj
);
void trace_mark_object(
    vm_stack_t *gray_objects,
    object_t *obj
);

void vm_new(
    void
);
void vm_free(
    void
);

void vm_track_object(
    object_t *obj
);
void vm_untrack_object(
    object_t *obj
);

frame_t *vm_new_frame();
void vm_frame_push(
    frame_t *frame
);
frame_t *vm_frame_pop();

void frame_free(
    frame_t *frame
);

// Marks the object as referenced in the current stack frame.
void frame_reference_object(
    frame_t *frame,
    object_t *obj
);
vm_t *vm_get_current(
    void
);
object_t *vm_get_empty_tuple(
    void
);
object_t *vm_get_none(
    void
);
