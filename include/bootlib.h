#ifndef BOOTLIB_H
#define BOOTLIB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit.h"

// Boot.dev munit macro helpers
#define munit_case(type, name, block) \
  static MunitResult name(const MunitParameter params[], void* user_data_or_fixture) { \
    (void) params; \
    (void) user_data_or_fixture; \
    block \
    return MUNIT_OK; \
  }

#define munit_test(name, test) \
  { (char*) (name), (test), NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }

#define munit_null_test \
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }

#define munit_suite(name, tests) \
  (MunitSuite){ (char*)(name), (tests), NULL, 1, MUNIT_SUITE_OPTION_NONE }

// Support 3 or 4 arguments for assert_int (ignoring optional msg in standard munit)
#ifdef assert_int
#undef assert_int
#endif
#define GET_ASSERT_INT_MACRO(_1, _2, _3, _4, NAME, ...) NAME
#define assert_int_3(a, op, b) munit_assert_int(a, op, b)
#define assert_int_4(a, op, b, msg) munit_assert_int(a, op, b)
#define assert_int(...) GET_ASSERT_INT_MACRO(__VA_ARGS__, assert_int_4, assert_int_3)(__VA_ARGS__)

// Memory tracking declarations
void *boot_malloc(size_t size, const char *file, int line);
void boot_free(void *ptr);
void *boot_realloc(void *ptr, size_t size, const char *file, int line);
void *boot_calloc(size_t count, size_t size, const char *file, int line);

bool boot_is_freed(void *ptr);
bool boot_all_freed(void);
void boot_reset_tracking(void);

#ifndef BOOTLIB_NO_OVERRIDE
#define malloc(size) boot_malloc(size, __FILE__, __LINE__)
#define free(ptr) boot_free(ptr)
#define realloc(ptr, size) boot_realloc(ptr, size, __FILE__, __LINE__)
#define calloc(count, size) boot_calloc(count, size, __FILE__, __LINE__)
#endif

#endif // BOOTLIB_H
