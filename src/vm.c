#include "vm.h"

#include "new.h"
#include "object.h"
#include "stack.h"

static vm_t *CURRENT_VM = NULL;

void vm_collect_garbage()
{
    mark();
    trace();
    sweep();
}

vm_t *vm_get_current(
    void
)
{
    return CURRENT_VM;
}

void sweep()
{
    vm_t *vm = vm_get_current();
    if (vm == NULL)
    {
        return;
    }

    /* Pass 1:
    Decref live children and free payloads for unmarked objects
    Notice that we cannot unmark live objects (obj->is_marked = false) until
    Pass 2. If Pass 1 unmarks objects on the fly, a later unmarked container
    visiting a live child might see is_marked == false and fail to decref it!
    */
    for (size_t i = 0; i < vm->objects->count; i++)
    {
        object_t *obj = vm->objects->data[i];
        if (obj == NULL || obj->is_marked)
        {
            continue;
        }
        object_decref_children(obj, true);
        object_free_payload(obj);
    }

    /*Pass 2:
    Free dead headers and unmark surviving objects
    */
    for (size_t i = 0; i < vm->objects->count; i++)
    {
        object_t *obj = vm->objects->data[i];
        if (obj == NULL)
        {
            continue;
        }
        if (obj->is_marked)
        {
            obj->is_marked = false;
            continue;
        }
        free(obj);
        vm->objects->data[i] = NULL;
    }

    // --- Compaction & Re-indexing ---
    stack_remove_nulls(vm->objects);
    for (size_t i = 0; i < vm->objects->count; i++)
    {
        object_t *obj = vm->objects->data[i];
        obj->tracker_id = i;
    }
}

void mark()
{
    vm_t *vm = vm_get_current();
    if (vm == NULL)
    {
        return;
    }

    for (size_t i = 0; i < vm->frames->count; i++)
    {
        frame_t *frame = vm->frames->data[i];
        for (size_t j = 0; j < frame->references->count; j++)
        {
            void *obj_ = frame->references->data[j];
            if (obj_ == NULL || object_is_immortal(obj_))
            {
                continue;
            }
            object_t *obj = obj_;
            obj->is_marked = true;
        }
    }
}

void trace()
{
    vm_t *vm = vm_get_current();
    if (vm == NULL)
    {
        return;
    }

    vm_stack_t *gray_objects = stack_new(8);
    if (gray_objects == NULL)
    {
        return;
    }

    // Get previously marked objects (which are the roots)
    for (size_t i = 0; i < vm->objects->count; i++)
    {
        void *obj_ = vm->objects->data[i];
        if (obj_ == NULL)
        {
            continue;
        }
        object_t *obj = obj_;
        if (obj->is_marked)
        {
            stack_push(gray_objects, obj);
        }
    }

    // Trace through the objects
    while (gray_objects->count > 0)
    {
        trace_blacken_object(gray_objects, stack_pop(gray_objects));
    }

    // Clean up after ourselves :)
    stack_free(gray_objects);
}

void trace_blacken_object(
    vm_stack_t *gray_objects,
    object_t *obj
)
{
    if (obj == NULL)
    {
        return;
    }

    switch (obj->kind)
    {
        case INTEGER:
        case FLOAT:
        case STRING:
        case NONE:
            break;
        case TUPLE:
        {
            for (size_t i = 0; i < obj->data.v_tuple->size; i++)
            {
                trace_mark_object(gray_objects, obj->data.v_tuple->elements[i]);
            }
            break;
        }
        case LIST:
        {
            for (size_t i = 0; i < obj->data.v_list.size; i++)
            {
                trace_mark_object(gray_objects, obj->data.v_list.elements[i]);
            }
            break;
        }
    }
}

void trace_mark_object(
    vm_stack_t *gray_objects,
    object_t *obj
)
{
    if (obj == NULL || obj->is_marked || object_is_immortal(obj))
    {
        return;
    }

    stack_push(gray_objects, obj);
    obj->is_marked = true;
}

void frame_reference_object(
    frame_t *frame,
    object_t *obj
)
{
    stack_push(frame->references, obj);
    refcount_inc(obj);
}

void _immortals_free(
    immortals_t *imm
)
{
    free(imm->none);
    object_free_payload(imm->empty_tuple);
    free(imm->empty_tuple);
}

void vm_new(
    void
)
{
    vm_t *vm = malloc(sizeof(vm_t));
    if (vm == NULL)
    {
        return;
    }

    vm->frames = stack_new(8);
    vm->objects = stack_new(8);
    if (vm->frames == NULL || vm->objects == NULL)
    {
        if (vm->frames != NULL)
            stack_free(vm->frames);
        if (vm->objects != NULL)
            stack_free(vm->objects);
        free(vm);
        return;
    }

    immortals_t *imm = &vm->immortals;
    imm->none = create_none_singleton();
    imm->empty_tuple = create_empty_tuple_singleton();
    if (imm->none == NULL || imm->empty_tuple == NULL)
    {
        _immortals_free(imm);
        stack_free(vm->frames);
        stack_free(vm->objects);
        free(vm);
        return;
    }

    CURRENT_VM = vm;
}

void vm_free()
{
    vm_t *vm = vm_get_current();
    if (vm == NULL)
    {
        return;
    }
    // Free the stack frames, an!d then their stack container
    for (size_t i = 0; i < vm->frames->count; i++)
    {
        frame_free(vm->frames->data[i]);
    }
    stack_free(vm->frames);

    // Free buffers
    for (size_t i = 0; i < vm->objects->count; i++)
    {
        if (vm->objects->data[i] != NULL)
        {
            object_free_payload(vm->objects->data[i]);
        }
    }
    // Free headers
    for (size_t i = 0; i < vm->objects->count; i++)
    {
        if (vm->objects->data[i] != NULL)
        {
            free(vm->objects->data[i]);
        }
    }
    stack_free(vm->objects);

    _immortals_free(&vm->immortals);

    free(vm);
    CURRENT_VM = NULL;
}

void vm_frame_push(
    frame_t *frame
)
{
    vm_t *vm = vm_get_current();
    if (vm == NULL)
    {
        return;
    }
    stack_push(vm->frames, frame);
}

frame_t *vm_frame_pop()
{
    vm_t *vm = vm_get_current();
    if (vm == NULL)
    {
        return NULL;
    }
    return stack_pop(vm->frames);
}

frame_t *vm_new_frame()
{
    frame_t *frame = malloc(sizeof(frame_t));
    frame->references = stack_new(8);

    vm_frame_push(frame);
    return frame;
}

void frame_free(
    frame_t *frame
)
{
    for (size_t i = 0; i < frame->references->count; i++)
    {
        if (frame->references->data[i] == NULL)
        {
            continue;
        }
        refcount_dec(frame->references->data[i]);
    }
    stack_free(frame->references);
    free(frame);
}

void vm_track_object(
    object_t *obj
)
{
    vm_t *vm = vm_get_current();
    if (vm == NULL)
    {
        return;
    }
    stack_push(vm->objects, obj);
    obj->tracker_id = vm->objects->count - 1;
}

void vm_untrack_object(
    object_t *obj
)
{
    vm_t *vm = vm_get_current();
    if (vm == NULL)
    {
        return;
    }
    if (obj == NULL)
    {
        return;
    }
    if (obj->tracker_id < vm->objects->count &&
        vm->objects->data[obj->tracker_id] == obj)
    {
        vm->objects->data[obj->tracker_id] = NULL;
    }
}
object_t *vm_get_empty_tuple()
{
    vm_t *vm = vm_get_current();
    if (vm == NULL)
    {
        return NULL;
    }
    return vm->immortals.empty_tuple;
};
object_t *vm_get_none()
{
    vm_t *vm = vm_get_current();
    if (vm == NULL)
    {
        return NULL;
    }
    return vm->immortals.none;
};
