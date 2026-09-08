#ifndef BOOTLIB_H
#define BOOTLIB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

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

// Variadic assertion macros supporting optional message parameters
#ifdef assert_int
#undef assert_int
#endif
#define GET_ASSERT_INT_MACRO(_1, _2, _3, _4, NAME, ...) NAME
#define assert_int_3(a, op, b) munit_assert_int(a, op, b)
#define assert_int_4(a, op, b, msg) munit_assert_int(a, op, b)
#define assert_int(...) GET_ASSERT_INT_MACRO(__VA_ARGS__, assert_int_4, assert_int_3)(__VA_ARGS__)

#ifdef assert_int_equal
#undef assert_int_equal
#endif
#define GET_ASSERT_INT_EQ_MACRO(_1, _2, _3, NAME, ...) NAME
#define assert_int_eq_2(a, b) munit_assert_int(a, ==, b)
#define assert_int_eq_3(a, b, msg) munit_assert_int(a, ==, b)
#define assert_int_equal(...) GET_ASSERT_INT_EQ_MACRO(__VA_ARGS__, assert_int_eq_3, assert_int_eq_2)(__VA_ARGS__)

#ifdef assert_size
#undef assert_size
#endif
#define GET_ASSERT_SIZE_MACRO(_1, _2, _3, _4, NAME, ...) NAME
#define assert_size_3(a, op, b) munit_assert_size(a, op, b)
#define assert_size_4(a, op, b, msg) munit_assert_size(a, op, b)
#define assert_size(...) GET_ASSERT_SIZE_MACRO(__VA_ARGS__, assert_size_4, assert_size_3)(__VA_ARGS__)

#ifdef assert_not_null
#undef assert_not_null
#endif
#define GET_ASSERT_NOT_NULL_MACRO(_1, _2, NAME, ...) NAME
#define assert_not_null_1(ptr) munit_assert_not_null(ptr)
#define assert_not_null_2(ptr, msg) munit_assert_not_null(ptr)
#define assert_not_null(...) GET_ASSERT_NOT_NULL_MACRO(__VA_ARGS__, assert_not_null_2, assert_not_null_1)(__VA_ARGS__)

#ifdef assert_null
#undef assert_null
#endif
#define GET_ASSERT_NULL_MACRO(_1, _2, NAME, ...) NAME
#define assert_null_1(ptr) munit_assert_null(ptr)
#define assert_null_2(ptr, msg) munit_assert_null(ptr)
#define assert_null(...) GET_ASSERT_NULL_MACRO(__VA_ARGS__, assert_null_2, assert_null_1)(__VA_ARGS__)

#ifdef assert_ptr_not_null
#undef assert_ptr_not_null
#endif
#define GET_ASSERT_PTR_NOT_NULL_MACRO(_1, _2, NAME, ...) NAME
#define assert_ptr_not_null_1(ptr) munit_assert_not_null(ptr)
#define assert_ptr_not_null_2(ptr, msg) munit_assert_not_null(ptr)
#define assert_ptr_not_null(...) GET_ASSERT_PTR_NOT_NULL_MACRO(__VA_ARGS__, assert_ptr_not_null_2, assert_ptr_not_null_1)(__VA_ARGS__)

#ifdef assert_ptr_null
#undef assert_ptr_null
#endif
#define GET_ASSERT_PTR_NULL_MACRO(_1, _2, NAME, ...) NAME
#define assert_ptr_null_1(ptr) munit_assert_null(ptr)
#define assert_ptr_null_2(ptr, msg) munit_assert_null(ptr)
#define assert_ptr_null(...) GET_ASSERT_PTR_NULL_MACRO(__VA_ARGS__, assert_ptr_null_2, assert_ptr_null_1)(__VA_ARGS__)

#ifdef assert_ptr_equal
#undef assert_ptr_equal
#endif
#define GET_ASSERT_PTR_EQ_MACRO(_1, _2, _3, NAME, ...) NAME
#define assert_ptr_eq_2(a, b) munit_assert_ptr_equal(a, b)
#define assert_ptr_eq_3(a, b, msg) munit_assert_ptr_equal(a, b)
#define assert_ptr_equal(...) GET_ASSERT_PTR_EQ_MACRO(__VA_ARGS__, assert_ptr_eq_3, assert_ptr_eq_2)(__VA_ARGS__)

#ifdef assert_ptr
#undef assert_ptr
#endif
#define GET_ASSERT_PTR_MACRO(_1, _2, _3, _4, NAME, ...) NAME
#define assert_ptr_3(a, op, b) munit_assert_ptr(a, op, b)
#define assert_ptr_4(a, op, b, msg) munit_assert_ptr(a, op, b)
#define assert_ptr(...) GET_ASSERT_PTR_MACRO(__VA_ARGS__, assert_ptr_4, assert_ptr_3)(__VA_ARGS__)

#ifdef assert_string_equal
#undef assert_string_equal
#endif
#define GET_ASSERT_STR_EQ_MACRO(_1, _2, _3, NAME, ...) NAME
#define assert_str_eq_2(a, b) munit_assert_string_equal(a, b)
#define assert_str_eq_3(a, b, msg) munit_assert_string_equal(a, b)
#define assert_string_equal(...) GET_ASSERT_STR_EQ_MACRO(__VA_ARGS__, assert_str_eq_3, assert_str_eq_2)(__VA_ARGS__)

// Memory tracking declarations
void *boot_malloc(size_t size, const char *file, int line);
void boot_free(void *ptr);
void *boot_realloc(void *ptr, size_t size, const char *file, int line);
void *boot_calloc(size_t count, size_t size, const char *file, int line);

bool boot_is_freed(void *ptr);
bool boot_all_freed(void);
size_t boot_alloc_size(void);
size_t boot_last_realloc_size(void);
size_t boot_realloc_count(void);
void boot_reset_tracking(void);

#ifndef BOOTLIB_NO_OVERRIDE
#define malloc(size) boot_malloc(size, __FILE__, __LINE__)
#define free(ptr) boot_free(ptr)
#define realloc(ptr, size) boot_realloc(ptr, size, __FILE__, __LINE__)
#define calloc(count, size) boot_calloc(count, size, __FILE__, __LINE__)
#endif

#endif // BOOTLIB_H
