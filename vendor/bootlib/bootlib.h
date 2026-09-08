#ifndef BOOTLIB_H
#define BOOTLIB_H

/**
 * @file bootlib.h
 * @brief Boot.dev Memory Tracking, Leak Detection, and Testing Interceptor Library.
 *
 * bootlib intercepts C standard library allocation functions (malloc, calloc, realloc, free)
 * to record active allocations, track memory size, log file locations/lines, detect memory leaks,
 * and simulate allocation failures during testing.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit.h"

/**
 * @brief Define a µnit test case wrapper.
 * @param type Test type (e.g. RUN or SUBMIT).
 * @param name Function name for the test.
 * @param block Code block executing the test assertions.
 */
#define munit_case(type, name, block) \
  static MunitResult name(const MunitParameter params[], void* user_data_or_fixture) { \
    (void) params; \
    (void) user_data_or_fixture; \
    block \
    return MUNIT_OK; \
  }

/**
 * @brief Register a named µnit test case.
 * @param name Relative path string for test (e.g. "/simple").
 * @param test Function pointer to the test case.
 */
#define munit_test(name, test) \
  { (char*) (name), (test), NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }

/**
 * @brief Null test terminator entry for µnit test arrays.
 */
#define munit_null_test \
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }

/**
 * @brief Construct a µnit test suite structure.
 * @param name Suite name string.
 * @param tests Array of registered MunitTest items.
 */
#define munit_suite(name, tests) \
  (MunitSuite){ (char*)(name), (tests), NULL, 1, MUNIT_SUITE_OPTION_NONE }

/* Variadic assertion macros supporting optional message parameters */

/**
 * @brief Assert integer comparison outcome with optional failure message.
 */
#ifdef assert_int
#undef assert_int
#endif
#define GET_ASSERT_INT_MACRO(_1, _2, _3, _4, NAME, ...) NAME
#define assert_int_3(a, op, b) munit_assert_int(a, op, b)
#define assert_int_4(a, op, b, msg) munit_assert_int(a, op, b)
#define assert_int(...) GET_ASSERT_INT_MACRO(__VA_ARGS__, assert_int_4, assert_int_3)(__VA_ARGS__)

/**
 * @brief Assert equality between two integers.
 */
#ifdef assert_int_equal
#undef assert_int_equal
#endif
#define GET_ASSERT_INT_EQ_MACRO(_1, _2, _3, NAME, ...) NAME
#define assert_int_eq_2(a, b) munit_assert_int(a, ==, b)
#define assert_int_eq_3(a, b, msg) munit_assert_int(a, ==, b)
#define assert_int_equal(...) GET_ASSERT_INT_EQ_MACRO(__VA_ARGS__, assert_int_eq_3, assert_int_eq_2)(__VA_ARGS__)

/**
 * @brief Assert float comparison outcome with optional failure message.
 */
#ifdef assert_float
#undef assert_float
#endif
#define GET_ASSERT_FLOAT_MACRO(_1, _2, _3, _4, NAME, ...) NAME
#define assert_float_3(a, op, b) munit_assert_float(a, op, b)
#define assert_float_4(a, op, b, msg) munit_assert_float(a, op, b)
#define assert_float(...) GET_ASSERT_FLOAT_MACRO(__VA_ARGS__, assert_float_4, assert_float_3)(__VA_ARGS__)

/**
 * @brief Assert size_t comparison outcome with optional failure message.
 */
#ifdef assert_size
#undef assert_size
#endif
#define GET_ASSERT_SIZE_MACRO(_1, _2, _3, _4, NAME, ...) NAME
#define assert_size_3(a, op, b) munit_assert_size(a, op, b)
#define assert_size_4(a, op, b, msg) munit_assert_size(a, op, b)
#define assert_size(...) GET_ASSERT_SIZE_MACRO(__VA_ARGS__, assert_size_4, assert_size_3)(__VA_ARGS__)

/**
 * @brief Assert pointer is not NULL with optional failure message.
 */
#ifdef assert_not_null
#undef assert_not_null
#endif
#define GET_ASSERT_NOT_NULL_MACRO(_1, _2, NAME, ...) NAME
#define assert_not_null_1(ptr) munit_assert_not_null(ptr)
#define assert_not_null_2(ptr, msg) munit_assert_not_null(ptr)
#define assert_not_null(...) GET_ASSERT_NOT_NULL_MACRO(__VA_ARGS__, assert_not_null_2, assert_not_null_1)(__VA_ARGS__)

/**
 * @brief Assert pointer is NULL with optional failure message.
 */
#ifdef assert_null
#undef assert_null
#endif
#define GET_ASSERT_NULL_MACRO(_1, _2, NAME, ...) NAME
#define assert_null_1(ptr) munit_assert_null(ptr)
#define assert_null_2(ptr, msg) munit_assert_null(ptr)
#define assert_null(...) GET_ASSERT_NULL_MACRO(__VA_ARGS__, assert_null_2, assert_null_1)(__VA_ARGS__)

/**
 * @brief Assert pointer is not NULL (alias).
 */
#ifdef assert_ptr_not_null
#undef assert_ptr_not_null
#endif
#define GET_ASSERT_PTR_NOT_NULL_MACRO(_1, _2, NAME, ...) NAME
#define assert_ptr_not_null_1(ptr) munit_assert_not_null(ptr)
#define assert_ptr_not_null_2(ptr, msg) munit_assert_not_null(ptr)
#define assert_ptr_not_null(...) GET_ASSERT_PTR_NOT_NULL_MACRO(__VA_ARGS__, assert_ptr_not_null_2, assert_ptr_not_null_1)(__VA_ARGS__)

/**
 * @brief Assert pointer is NULL (alias).
 */
#ifdef assert_ptr_null
#undef assert_ptr_null
#endif
#define GET_ASSERT_PTR_NULL_MACRO(_1, _2, NAME, ...) NAME
#define assert_ptr_null_1(ptr) munit_assert_null(ptr)
#define assert_ptr_null_2(ptr, msg) munit_assert_null(ptr)
#define assert_ptr_null(...) GET_ASSERT_PTR_NULL_MACRO(__VA_ARGS__, assert_ptr_null_2, assert_ptr_null_1)(__VA_ARGS__)

/**
 * @brief Assert pointer equality between two memory addresses.
 */
#ifdef assert_ptr_equal
#undef assert_ptr_equal
#endif
#define GET_ASSERT_PTR_EQ_MACRO(_1, _2, _3, NAME, ...) NAME
#define assert_ptr_eq_2(a, b) munit_assert_ptr_equal(a, b)
#define assert_ptr_eq_3(a, b, msg) munit_assert_ptr_equal(a, b)
#define assert_ptr_equal(...) GET_ASSERT_PTR_EQ_MACRO(__VA_ARGS__, assert_ptr_eq_3, assert_ptr_eq_2)(__VA_ARGS__)

/**
 * @brief Assert pointer comparison outcome with optional failure message.
 */
#ifdef assert_ptr
#undef assert_ptr
#endif
#define GET_ASSERT_PTR_MACRO(_1, _2, _3, _4, NAME, ...) NAME
#define assert_ptr_3(a, op, b) munit_assert_ptr(a, op, b)
#define assert_ptr_4(a, op, b, msg) munit_assert_ptr(a, op, b)
#define assert_ptr(...) GET_ASSERT_PTR_MACRO(__VA_ARGS__, assert_ptr_4, assert_ptr_3)(__VA_ARGS__)

/**
 * @brief Assert string equality between two C null-terminated strings.
 */
#ifdef assert_string_equal
#undef assert_string_equal
#endif
#define GET_ASSERT_STR_EQ_MACRO(_1, _2, _3, NAME, ...) NAME
#define assert_str_eq_2(a, b) munit_assert_string_equal(a, b)
#define assert_str_eq_3(a, b, msg) munit_assert_string_equal(a, b)
#define assert_string_equal(...) GET_ASSERT_STR_EQ_MACRO(__VA_ARGS__, assert_str_eq_3, assert_str_eq_2)(__VA_ARGS__)

/* Memory tracking function declarations */

/**
 * @brief Intercepted malloc allocator.
 * @param size Number of bytes to allocate.
 * @param file Source filename calling malloc.
 * @param line Line number calling malloc.
 * @return Pointer to allocated memory, or NULL on failure.
 */
void *boot_malloc(size_t size, const char *file, int line);

/**
 * @brief Intercepted free deallocator.
 * @param ptr Pointer to memory block to free.
 */
void boot_free(void *ptr);

/**
 * @brief Intercepted realloc dynamic array resizer.
 * @param ptr Pointer to existing memory block (or NULL).
 * @param size Target allocation size in bytes.
 * @param file Source filename calling realloc.
 * @param line Line number calling realloc.
 * @return Pointer to resized memory block, or NULL on failure.
 */
void *boot_realloc(void *ptr, size_t size, const char *file, int line);

/**
 * @brief Intercepted calloc zero-initialized array allocator.
 * @param count Number of elements.
 * @param size Size of each element in bytes.
 * @param file Source filename calling calloc.
 * @param line Line number calling calloc.
 * @return Pointer to zeroed memory block, or NULL on failure.
 */
void *boot_calloc(size_t count, size_t size, const char *file, int line);

/**
 * @brief Check if a specific pointer has been freed.
 * @param ptr Memory pointer to query.
 * @return true if pointer is NULL, tracked as freed, or untracked; false if live.
 */
bool boot_is_freed(void *ptr);

/**
 * @brief Verify that all tracked memory allocations have been freed.
 * @return true if zero memory leaks remain; false if active allocations exist.
 */
bool boot_all_freed(void);

/**
 * @brief Calculate the total bytes currently allocated across all live pointers.
 * @return Total live memory consumption in bytes.
 */
size_t boot_alloc_size(void);

/**
 * @brief Query the byte size requested in the last realloc call.
 * @return Requested byte size.
 */
size_t boot_last_realloc_size(void);

/**
 * @brief Query the cumulative count of realloc calls.
 * @return Total number of realloc invocations.
 */
size_t boot_realloc_count(void);

/**
 * @brief Reset internal memory tracking counters and allocation records.
 */
void boot_reset_tracking(void);

/**
 * @brief Configure memory allocation failure simulation.
 * @param count Number of successful allocations before returning NULL on the next call.
 */
void boot_set_fail_alloc_after(int count);

#ifndef BOOTLIB_NO_OVERRIDE
#define malloc(size) boot_malloc(size, __FILE__, __LINE__)
#define free(ptr) boot_free(ptr)
#define realloc(ptr, size) boot_realloc(ptr, size, __FILE__, __LINE__)
#define calloc(count, size) boot_calloc(count, size, __FILE__, __LINE__)
#endif

#endif // BOOTLIB_H
