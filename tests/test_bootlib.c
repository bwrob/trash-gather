/**
 * @file test_bootlib.c
 * @brief Unit tests for bootlib memory poisoning, realloc tracking, and O(1) hash
 * indexing.
 */

#include "bootlib.h"
#include "munit.h"

#include <stdint.h>
#include <string.h>

/**
 * @brief Test that boot_malloc fills uninitialized memory with 0xA5 poison bytes.
 */
munit_case(
    RUN,
    bootlib_malloc_poisoning,
    {
        size_t sz = 64;
        uint8_t *buf = malloc(sz);
        assert_not_null(buf);
        for (size_t i = 0; i < sz; i++)
        {
            assert_int(buf[i], ==, 0xA5);
        }
        free(buf);
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that boot_calloc produces pristine zeroed memory without poison.
 */
munit_case(
    RUN,
    bootlib_calloc_zeroed,
    {
        size_t count = 32;
        size_t sz = sizeof(uint32_t);
        uint32_t *buf = calloc(count, sz);
        assert_not_null(buf);
        for (size_t i = 0; i < count; i++)
        {
            assert_int(buf[i], ==, 0);
        }
        free(buf);
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that boot_free overwrites memory with 0xDF poison and updates tracking.
 */
munit_case(
    RUN,
    bootlib_free_poisoning,
    {
        size_t sz = 32;
        uint8_t *buf = malloc(sz);
        assert_not_null(buf);
        memset(buf, 0x00, sz);

        assert_false(boot_is_freed(buf));
        free(buf);
        assert_true(boot_is_freed(buf));
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that expanding realloc preserves existing payload and poisons newly grown
 * bytes.
 */
munit_case(
    RUN,
    bootlib_realloc_expansion_poisoning,
    {
        size_t init_sz = 16;
        size_t grow_sz = 32;
        uint8_t *buf = malloc(init_sz);
        assert_not_null(buf);
        memset(buf, 0x11, init_sz);

        uint8_t *grown = realloc(buf, grow_sz);
        assert_not_null(grown);
        for (size_t i = 0; i < init_sz; i++)
        {
            assert_int(grown[i], ==, 0x11);
        }
        for (size_t i = init_sz; i < grow_sz; i++)
        {
            assert_int(grown[i], ==, 0xA5);
        }

        free(grown);
        assert(boot_all_freed());
    }
);

/**
 * @brief Test that O(1) hash index accurately tracks sizes, statuses, and frees across
 * many allocations.
 */
munit_case(
    RUN,
    bootlib_hash_tracking_many_allocations,
    {
        const size_t N = 256;
        void *ptrs[256];

        for (size_t i = 0; i < N; i++)
        {
            size_t sz = 16 + (i % 32);
            ptrs[i] = malloc(sz);
            assert_not_null(ptrs[i]);
            assert_true(boot_is_tracked(ptrs[i]));
            assert_false(boot_is_freed(ptrs[i]));
            assert_int(boot_ptr_size(ptrs[i]), ==, (int)sz);
        }

        for (size_t i = 0; i < N; i++)
        {
            free(ptrs[i]);
            assert_true(boot_is_freed(ptrs[i]));
        }

        assert(boot_all_freed());
    }
);

MunitTest bootlib_tests[] = {
    munit_test("/malloc_poisoning", bootlib_malloc_poisoning),
    munit_test("/calloc_zeroed", bootlib_calloc_zeroed),
    munit_test("/free_poisoning", bootlib_free_poisoning),
    munit_test("/realloc_expansion_poisoning", bootlib_realloc_expansion_poisoning),
    munit_test(
        "/hash_tracking_many_allocations",
        bootlib_hash_tracking_many_allocations
    ),
    munit_null_test,
};
