#include "bootlib.h"
#include "munit.h"

extern MunitTest vm_tests[];
extern MunitTest trace_tests[];

static MunitSuite child_suites[] = {
    munit_suite("mark-and-sweep", vm_tests),
    munit_suite("trace", trace_tests),
    { NULL, NULL, NULL, 0, MUNIT_SUITE_OPTION_NONE }
};

static const MunitSuite master_suite = {
    (char*) "",
    NULL,
    child_suites,
    1,
    MUNIT_SUITE_OPTION_NONE
};

int main(int argc, char *argv[]) {
  return munit_suite_main(&master_suite, NULL, argc, argv);
}
