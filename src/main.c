#include "bootlib.h"
#include "new.h"
#include "object.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

int main(
    void
)
{
    printf("Initializing Virtual Machine...\n");

    vm_new();
    frame_t *f1 = vm_new_frame();

    object_t *s = new_string("Hello from  VM!");
    frame_reference_object(f1, s);

    printf("Created string object in frame 1. Collecting garbage...\n");
    vm_collect_garbage();

    vm_t *vm = vm_get_current();
    printf("Object count in VM: %zu\n", vm->objects->count);

    frame_free(vm_frame_pop());
    vm_collect_garbage();
    printf(
        "Freed frame 1 and collected garbage. Object count in VM: %zu\n",
        vm->objects->count
    );

    vm_free();

    if (boot_all_freed())
    {
        printf("All memory cleanly freed!\n");
    }
    else
    {
        printf("Memory leaks detected!\n");
    }

    return 0;
}
