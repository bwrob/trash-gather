#include "vm.h"

#include "object.h"
#include "stack.h"

static vm_t *CURRENT_VM = NULL;

void vm_collect_garbage() {
  mark();
  trace();
  sweep();
}

vm_t *vm_get_current(void) {
  return CURRENT_VM;
}

void sweep() {
  /* Pass 1:
  Decref live children and free payloads for unmarked objects
  Notice that we cannot unmark live objects (obj->is_marked = false) until
  Pass 2. If Pass 1 unmarks objects on the fly, a later unmarked container
  visiting a live child might see is_marked == false and fail to decref it!
  */
  for (size_t i = 0; i < CURRENT_VM->objects->count; i++) {
    object_t *obj = CURRENT_VM->objects->data[i];
    if (obj == NULL || obj->is_marked) {
      continue;
    }
    object_decref_children(obj, true);
    object_free_payload(obj);
  }

  /*Pass 2:
  Free dead headers and unmark surviving objects
  */
  for (size_t i = 0; i < CURRENT_VM->objects->count; i++) {
    object_t *obj = CURRENT_VM->objects->data[i];
    if (obj == NULL) {
      continue;
    }
    if (obj->is_marked) {
      obj->is_marked = false;
      continue;
    }
    free(obj);
    CURRENT_VM->objects->data[i] = NULL;
  }

  // --- Compaction & Re-indexing ---
  stack_remove_nulls(CURRENT_VM->objects);
  for (size_t i = 0; i < CURRENT_VM->objects->count; i++) {
    object_t *obj = CURRENT_VM->objects->data[i];
    obj->tracker_id = i;
  }
}

void mark() {
  for (size_t i = 0; i < CURRENT_VM->frames->count; i++) {
    frame_t *frame = CURRENT_VM->frames->data[i];
    for (size_t j = 0; j < frame->references->count; j++) {
      void *obj_ = frame->references->data[j];
      if (obj_ == NULL) {
        continue;
      }
      object_t *obj = obj_;
      obj->is_marked = true;
    }
  }
}

void trace() {
  vm_stack_t *gray_objects = stack_new(8);
  if (gray_objects == NULL) {
    return;
  }

  // Get previously marked objects (which are the roots)
  for (size_t i = 0; i < CURRENT_VM->objects->count; i++) {
    void *obj_ = CURRENT_VM->objects->data[i];
    if (obj_ == NULL) {
      continue;
    }
    object_t *obj = obj_;
    if (obj->is_marked) {
      stack_push(gray_objects, obj);
    }
  }

  // Trace through the objects
  while (gray_objects->count > 0) {
    trace_blacken_object(gray_objects, stack_pop(gray_objects));
  }

  // Clean up after ourselves :)
  stack_free(gray_objects);
}

void trace_blacken_object(vm_stack_t *gray_objects, object_t *ref) {
  object_t *obj = ref;

  switch (obj->kind) {
  case INTEGER:
  case FLOAT:
  case STRING:
    break;
  case VECTOR3: {
    vector_t vec = obj->data.v_vector3;
    trace_mark_object(gray_objects, vec.x);
    trace_mark_object(gray_objects, vec.y);
    trace_mark_object(gray_objects, vec.z);
    break;
  }
  case ARRAY: {
    for (size_t i = 0; i < obj->data.v_array.size; i++) {
      trace_mark_object(gray_objects, obj->data.v_array.elements[i]);
    }
    break;
  }
  }
}

void trace_mark_object(vm_stack_t *gray_objects, object_t *obj) {
  if (obj == NULL || obj->is_marked) {
    return;
  }

  stack_push(gray_objects, obj);
  obj->is_marked = true;
}

void frame_reference_object(frame_t *frame, object_t *obj) {
  stack_push(frame->references, obj);
  refcount_inc(obj);
}

void vm_new(void) {
  vm_t *vm = malloc(sizeof(vm_t));
  if (vm == NULL) {
    return;
  }

  vm->frames = stack_new(8);
  vm->objects = stack_new(8);
  if (vm->frames == NULL || vm->objects == NULL) {
    if (vm->frames != NULL)
      stack_free(vm->frames);
    if (vm->objects != NULL)
      stack_free(vm->objects);
    free(vm);
    return;
  }
  CURRENT_VM = vm;
}

void vm_free() {
  // Free the stack frames, an!d then their stack container
  for (size_t i = 0; i < CURRENT_VM->frames->count; i++) {
    frame_free(CURRENT_VM->frames->data[i]);
  }
  stack_free(CURRENT_VM->frames);

  // Free buffers
  for (size_t i = 0; i < CURRENT_VM->objects->count; i++) {
    if (CURRENT_VM->objects->data[i] != NULL) {
      object_free_payload(CURRENT_VM->objects->data[i]);
    }
  }
  // Free headers
  for (size_t i = 0; i < CURRENT_VM->objects->count; i++) {
    if (CURRENT_VM->objects->data[i] != NULL) {
      free(CURRENT_VM->objects->data[i]);
    }
  }

  stack_free(CURRENT_VM->objects);
  free(CURRENT_VM);
}

void vm_frame_push(frame_t *frame) {
  stack_push(CURRENT_VM->frames, frame);
}

frame_t *vm_frame_pop() {
  return stack_pop(CURRENT_VM->frames);
}

frame_t *vm_new_frame() {
  frame_t *frame = malloc(sizeof(frame_t));
  frame->references = stack_new(8);

  vm_frame_push(frame);
  return frame;
}

void frame_free(frame_t *frame) {
  for (size_t i = 0; i < frame->references->count; i++) {
    if (frame->references->data[i] == NULL) {
      continue;
    }
    refcount_dec(frame->references->data[i]);
  }
  stack_free(frame->references);
  free(frame);
}

void vm_track_object(object_t *obj) {
  stack_push(CURRENT_VM->objects, obj);
  obj->tracker_id = (CURRENT_VM->objects->count) - 1;
}

void vm_untrack_object(object_t *obj) {
  if (obj->tracker_id < CURRENT_VM->objects->count &&
      CURRENT_VM->objects->data[obj->tracker_id] == obj) {
    CURRENT_VM->objects->data[obj->tracker_id] = NULL;
  }
}
