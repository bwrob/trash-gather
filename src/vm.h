#pragma once

#include "object.h"
#include "stack.h"

typedef struct VirtualMachine
{
    // stack frames: vm_stack_t frame_t
    vm_stack_t *frames;

    // These are the rest of the objects: vm_stack_t object_t
    vm_stack_t *objects;
} vm_t;

typedef struct StackFrame
{
    vm_stack_t *references;
} frame_t;

void mark();
void trace();
void sweep();

void vm_collect_garbage();

void trace_blacken_object(
    vm_stack_t *gray_objects,
    object_t *ref
);
void trace_mark_object(
    vm_stack_t *gray_objects,
    object_t *ref
);

void vm_new(
    void
);
void vm_free();

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
