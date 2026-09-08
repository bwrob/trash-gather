/**
 * @file test_runner.c
 * @brief Master µnit test suite runner aggregating all GC, stack, frame, and
 * object unit test suites.
 */

#include "bootlib.h"
#include "munit.h"

extern MunitTest vm_tests[];
extern MunitTest mark_tests[];
extern MunitTest trace_tests[];
extern MunitTest object_tests[];
extern MunitTest frame_tests[];
extern MunitTest new_tests[];
extern MunitTest vm_stack_tests[];
extern MunitTest refcount_tests[];

static MunitSuite child_suites[] = {
    munit_suite("mark-and-sweep", vm_tests),
    munit_suite("mark", mark_tests),
    munit_suite("trace", trace_tests),
    munit_suite("object", object_tests),
    munit_suite("frame", frame_tests),
    munit_suite("new", new_tests),
    munit_suite("stack", vm_stack_tests),
    munit_suite("refcount", refcount_tests),
    {NULL, NULL, NULL, 0, MUNIT_SUITE_OPTION_NONE}};

static const MunitSuite master_suite = {(char *)"", NULL, child_suites, 1,
                                        MUNIT_SUITE_OPTION_NONE};

int main(int argc, char *argv[]) {
  return munit_suite_main(&master_suite, NULL, argc, argv);
}
