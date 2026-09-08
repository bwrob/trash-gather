#include "bootlib.h"
#include "sneknew.h"
#include "snekobject.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  printf("Initializing Virtual Machine...\n");

  vm_t *vm = vm_new();
  frame_t *f1 = vm_new_frame(vm);

  snek_object_t *s = new_snek_string(vm, "Hello from Snek VM!");
  frame_reference_object(f1, s);

  printf("Created string object in frame 1. Collecting garbage...\n");
  vm_collect_garbage(vm);
  printf("Object count in VM: %zu\n", vm->objects->count);

  frame_free(vm_frame_pop(vm));
  vm_collect_garbage(vm);
  printf("Freed frame 1 and collected garbage. Object count in VM: %zu\n", vm->objects->count);

  vm_free(vm);

  if (boot_all_freed()) {
    printf("All memory cleanly freed!\n");
  } else {
    printf("Memory leaks detected!\n");
  }

  return 0;
}
