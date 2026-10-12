/**
 * @file bootlib.c
 * @brief Memory allocation tracking, failure simulation, and leak detection implementation.
 */

#ifndef BOOTLIB_NO_OVERRIDE
#define BOOTLIB_NO_OVERRIDE
#endif
#include "bootlib.h"

// Undefine macros so bootlib.c calls actual stdlib allocation functions
#ifdef malloc
#undef malloc
#endif
#ifdef free
#undef free
#endif
#ifdef realloc
#undef realloc
#endif
#ifdef calloc
#undef calloc
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOOT_POISON_ALLOC 0xA5
#define BOOT_POISON_FREE  0xDF

/**
 * Maximum capacity for tracking memory allocations in tests.
 */
#define MAX_BOOT_ALLOCATIONS 65536

#define BOOT_HASH_CAPACITY 131072
#define BOOT_HASH_MASK (BOOT_HASH_CAPACITY - 1)

/**
 * Internal tracking structure for a single memory allocation.
 */
typedef struct {
  void *ptr;        /**< Pointer returned by allocator */
  size_t size;      /**< Allocation size in bytes */
  bool freed;       /**< Deallocation status flag */
  const char *file; /**< Source file where allocation occurred */
  int line;         /**< Line number where allocation occurred */
  size_t alloc_id;  /**< Monotonically increasing allocation sequence ID */
} boot_alloc_t;

static boot_alloc_t g_allocs[MAX_BOOT_ALLOCATIONS];
static int g_hash_table[BOOT_HASH_CAPACITY];
static bool g_hash_initialized = false;

static size_t g_alloc_count = 0;
static size_t g_live_alloc_count = 0;
static size_t g_total_alloc_count = 0;
static size_t g_total_free_count = 0;
static size_t g_current_alloc_size = 0;
static size_t g_peak_alloc_size = 0;

static size_t g_last_realloc_size = 0;
static size_t g_realloc_count = 0;

static int g_fail_alloc_after = -1;
static int g_fail_alloc_repeat = 1;
static size_t g_fail_alloc_injected_count = 0;

/**
 * @brief Hash function for memory pointers using 64-bit integer mixing.
 * @param ptr Pointer address to hash.
 * @return 64-bit hash digest.
 */
static inline size_t hash_ptr(const void *ptr) {
  uintptr_t p = (uintptr_t)ptr;
  p ^= p >> 30;
  p *= 0xbf58476d1ce4e5b9ULL;
  p ^= p >> 27;
  p *= 0x94d049bb133111ebULL;
  p ^= p >> 31;
  return (size_t)p;
}

/**
 * @brief Lazily initialize the allocation pointer hash table if needed.
 */
static void hash_init_if_needed(void) {
  if (!g_hash_initialized) {
    memset(g_hash_table, -1, sizeof(g_hash_table));
    g_hash_initialized = true;
  }
}

/**
 * @brief Look up an allocation record index in the hash table by pointer.
 * @param ptr Memory pointer to query.
 * @return Index in g_allocs array, or -1 if untracked.
 */
static int find_alloc_index(const void *ptr) {
  if (ptr == NULL || g_alloc_count == 0) return -1;
  hash_init_if_needed();
  size_t h = hash_ptr(ptr) & BOOT_HASH_MASK;
  for (size_t step = 0; step < BOOT_HASH_CAPACITY; step++) {
    int idx = g_hash_table[(h + step) & BOOT_HASH_MASK];
    if (idx == -1) {
      return -1;
    }
    if ((size_t)idx < g_alloc_count && g_allocs[idx].ptr == ptr) {
      return idx;
    }
  }
  return -1;
}

/**
 * @brief Insert or associate an allocation index in the pointer hash table.
 * @param ptr Memory pointer key.
 * @param alloc_idx Corresponding index in g_allocs.
 */
static void hash_insert(const void *ptr, int alloc_idx) {
  hash_init_if_needed();
  size_t h = hash_ptr(ptr) & BOOT_HASH_MASK;
  for (size_t step = 0; step < BOOT_HASH_CAPACITY; step++) {
    size_t slot = (h + step) & BOOT_HASH_MASK;
    if (g_hash_table[slot] == -1 || g_hash_table[slot] == alloc_idx) {
      g_hash_table[slot] = alloc_idx;
      return;
    }
  }
}

/**
 * @brief Register or update an active allocation in the global tracking array.
 * @param ptr Pointer returned by memory allocator.
 * @param size Byte size of requested memory block.
 * @param file Source file initiating allocation.
 * @param line Source line number initiating allocation.
 */
static void track_add(void *ptr, size_t size, const char *file, int line) {
  if (ptr == NULL) return;

  g_total_alloc_count++;
  size_t current_id = g_total_alloc_count;

  int existing_idx = find_alloc_index(ptr);
  if (existing_idx >= 0) {
    size_t i = (size_t)existing_idx;
    if (g_allocs[i].freed) {
      g_live_alloc_count++;
      g_current_alloc_size += size;
    } else {
      g_current_alloc_size = g_current_alloc_size - g_allocs[i].size + size;
    }
    g_allocs[i].size = size;
    g_allocs[i].freed = false;
    g_allocs[i].file = file;
    g_allocs[i].line = line;
    g_allocs[i].alloc_id = current_id;
    if (g_current_alloc_size > g_peak_alloc_size) {
      g_peak_alloc_size = g_current_alloc_size;
    }
    return;
  }

  if (g_alloc_count < MAX_BOOT_ALLOCATIONS) {
    size_t idx = g_alloc_count;
    g_allocs[idx].ptr = ptr;
    g_allocs[idx].size = size;
    g_allocs[idx].freed = false;
    g_allocs[idx].file = file;
    g_allocs[idx].line = line;
    g_allocs[idx].alloc_id = current_id;
    hash_insert(ptr, (int)idx);
    g_alloc_count++;
    g_live_alloc_count++;
    g_current_alloc_size += size;
    if (g_current_alloc_size > g_peak_alloc_size) {
      g_peak_alloc_size = g_current_alloc_size;
    }
  } else {
    fprintf(stderr, "[bootlib warning] Max allocation tracking capacity reached (%d)\n", MAX_BOOT_ALLOCATIONS);
  }
}

/**
 * @brief Mark a pointer as freed in the tracking table.
 * @param ptr Pointer to deallocated memory block.
 * @return Size in bytes of freed block, or 0 if untracked.
 */
static size_t track_free(void *ptr) {
  if (ptr == NULL) return 0;
  int idx = find_alloc_index(ptr);
  if (idx >= 0) {
    size_t i = (size_t)idx;
    if (g_allocs[i].freed) {
      fprintf(stderr, "\n[bootlib error] Double free detected at %p (originally allocated at %s:%d, id=%zu)\n",
              ptr, g_allocs[i].file ? g_allocs[i].file : "unknown", g_allocs[i].line, g_allocs[i].alloc_id);
      abort();
    }
    g_allocs[i].freed = true;
    if (g_live_alloc_count > 0) {
      g_live_alloc_count--;
    }
    if (g_current_alloc_size >= g_allocs[i].size) {
      g_current_alloc_size -= g_allocs[i].size;
    } else {
      g_current_alloc_size = 0;
    }
    g_total_free_count++;
    return g_allocs[i].size;
  }
  return 0;
}

/**
 * @brief Configure allocation failure simulation.
 * @param count Successful allocation budget before injecting failure.
 */
void boot_set_fail_alloc_after(int count) {
  g_fail_alloc_after = count;
  g_fail_alloc_repeat = 1;
  g_fail_alloc_injected_count = 0;
}

/**
 * @brief Configure repetitive or persistent allocation failure simulation.
 * @param after Number of successful allocations before failure begins.
 * @param repeat Number of consecutive allocations to fail (-1 for persistent failure).
 */
void boot_set_fail_alloc_repeat(int after, int repeat) {
  g_fail_alloc_after = after;
  g_fail_alloc_repeat = repeat;
  g_fail_alloc_injected_count = 0;
}

/**
 * @brief Check whether the allocation failure injector was triggered since last configured.
 * @return true if an allocation attempt was rejected, false otherwise.
 */
bool boot_fail_alloc_triggered(void) {
  return g_fail_alloc_injected_count > 0;
}

/**
 * @brief Query the total number of allocations rejected by the failure injector.
 * @return Count of injected allocation failures.
 */
size_t boot_fail_alloc_injected_count(void) {
  return g_fail_alloc_injected_count;
}

/**
 * @brief Reset allocation failure simulation configuration and counters.
 */
void boot_reset_fail_alloc(void) {
  g_fail_alloc_after = -1;
  g_fail_alloc_repeat = 1;
  g_fail_alloc_injected_count = 0;
}

/**
 * @brief Helper to check if allocation failure simulation should trigger.
 * @return true if current allocation attempt must fail, false otherwise.
 */
static bool check_should_fail(void) {
  if (g_fail_alloc_after == 0) {
    g_fail_alloc_injected_count++;
    if (g_fail_alloc_repeat > 0) {
      g_fail_alloc_repeat--;
      if (g_fail_alloc_repeat == 0) {
        g_fail_alloc_after = -1;
      }
    }
    return true;
  }
  if (g_fail_alloc_after > 0) {
    g_fail_alloc_after--;
  }
  return false;
}

/**
 * @brief Intercepted malloc allocator.
 * @param size Allocation size in bytes.
 * @param file Calling file name.
 * @param line Calling line number.
 * @return Allocated pointer or NULL.
 */
void *boot_malloc(size_t size, const char *file, int line) {
  if (check_should_fail()) return NULL;
  void *ptr = malloc(size);
  if (ptr != NULL && size > 0) {
    memset(ptr, BOOT_POISON_ALLOC, size);
  }
  track_add(ptr, size, file, line);
  return ptr;
}

/**
 * @brief Intercepted free deallocator.
 * @param ptr Memory pointer to release.
 */
void boot_free(void *ptr) {
  if (ptr == NULL) return;
  size_t size = track_free(ptr);
  if (size > 0) {
    memset(ptr, BOOT_POISON_FREE, size);
  }
  free(ptr);
}

/**
 * @brief Intercepted realloc dynamic array resizer.
 * @param ptr Original memory pointer.
 * @param size Target allocation size.
 * @param file Calling file name.
 * @param line Calling line number.
 * @return Resized memory pointer or NULL.
 */
void *boot_realloc(void *ptr, size_t size, const char *file, int line) {
  if (check_should_fail()) return NULL;
  g_last_realloc_size = size;
  g_realloc_count++;

  size_t old_size = 0;
  if (ptr != NULL) {
    old_size = boot_ptr_size(ptr);
  }

  void *new_ptr = realloc(ptr, size);
  if (new_ptr != NULL) {
    if (ptr != NULL && new_ptr != ptr) {
      track_free(ptr);
    }
    track_add(new_ptr, size, file, line);
    if (size > old_size && old_size > 0) {
      memset((char *)new_ptr + old_size, BOOT_POISON_ALLOC, size - old_size);
    }
  }
  return new_ptr;
}

/**
 * @brief Intercepted calloc zero-initialized array allocator.
 * @param count Number of elements.
 * @param size Element size.
 * @param file Calling file name.
 * @param line Calling line number.
 * @return Zero-filled memory block or NULL.
 */
void *boot_calloc(size_t count, size_t size, const char *file, int line) {
  if (check_should_fail()) return NULL;
  void *ptr = calloc(count, size);
  track_add(ptr, count * size, file, line);
  return ptr;
}

/**
 * @brief Query if a pointer is freed.
 * @param ptr Pointer to query.
 * @return true if freed, false if active.
 */
bool boot_is_freed(void *ptr) {
  if (ptr == NULL) return true;
  int idx = find_alloc_index(ptr);
  if (idx >= 0) {
    return g_allocs[idx].freed;
  }
  return true;
}

/**
 * @brief Report memory leaks across all active allocations.
 * @return true if zero leaks exist, false if active leaks exist.
 */
bool boot_all_freed(void) {
  bool all_freed = true;
  size_t leak_count = 0;
  size_t leaked_bytes = 0;

  for (size_t i = 0; i < g_alloc_count; i++) {
    if (!g_allocs[i].freed) {
      if (all_freed) {
        fprintf(stderr, "\n=== Memory Leak Report (bootlib) ===\n");
        all_freed = false;
      }
      fprintf(stderr, "  LEAK #%zu: %zu bytes at %p (allocated at %s:%d)\n",
              ++leak_count, g_allocs[i].size, g_allocs[i].ptr,
              g_allocs[i].file ? g_allocs[i].file : "unknown", g_allocs[i].line);
      leaked_bytes += g_allocs[i].size;
    }
  }

  if (!all_freed) {
    fprintf(stderr, "Total Leaked: %zu bytes across %zu allocations\n=====================================\n\n",
            leaked_bytes, leak_count);
  }

  return all_freed;
}

/**
 * @brief Total currently allocated bytes.
 * @return Cumulative active size in bytes.
 */
size_t boot_alloc_size(void) {
  return g_current_alloc_size;
}

/**
 * @brief Query byte size requested in last realloc call.
 * @return Requested byte size.
 */
size_t boot_last_realloc_size(void) {
  return g_last_realloc_size;
}

/**
 * @brief Total count of realloc calls.
 * @return Number of realloc invocations.
 */
size_t boot_realloc_count(void) {
  return g_realloc_count;
}

/**
 * @brief Capture a memory allocation checkpoint.
 * @return Checkpoint snapshot record.
 */
boot_checkpoint_t boot_checkpoint(void) {
  boot_checkpoint_t cp;
  cp.min_alloc_id = g_total_alloc_count;
  cp.live_count = g_live_alloc_count;
  cp.alloc_size = g_current_alloc_size;
  return cp;
}

/**
 * @brief Verify that all allocations created since the checkpoint have been freed.
 * @param cp Checkpoint record to evaluate against.
 * @return true if zero leaks exist since checkpoint, false otherwise.
 */
bool boot_checkpoint_all_freed(boot_checkpoint_t cp) {
  bool clean = true;
  size_t leak_count = 0;
  size_t leaked_bytes = 0;

  for (size_t i = 0; i < g_alloc_count; i++) {
    if (!g_allocs[i].freed && g_allocs[i].alloc_id > cp.min_alloc_id) {
      if (clean) {
        fprintf(stderr, "\n=== Memory Leak Report (bootlib checkpoint > #%zu) ===\n", cp.min_alloc_id);
        clean = false;
      }
      fprintf(stderr, "  LEAK #%zu: %zu bytes at %p (allocated at %s:%d, id=%zu)\n",
              ++leak_count, g_allocs[i].size, g_allocs[i].ptr,
              g_allocs[i].file ? g_allocs[i].file : "unknown", g_allocs[i].line,
              g_allocs[i].alloc_id);
      leaked_bytes += g_allocs[i].size;
    }
  }

  if (!clean) {
    fprintf(stderr, "Total Leaked Since Checkpoint: %zu bytes across %zu allocations\n===========================================================\n\n",
            leaked_bytes, leak_count);
  }

  return clean;
}

/**
 * @brief Count active unreleased allocations made since the checkpoint.
 * @param cp Checkpoint record to evaluate against.
 * @return Number of unreleased allocations since checkpoint.
 */
size_t boot_checkpoint_leak_count(boot_checkpoint_t cp) {
  size_t leaks = 0;
  for (size_t i = 0; i < g_alloc_count; i++) {
    if (!g_allocs[i].freed && g_allocs[i].alloc_id > cp.min_alloc_id) {
      leaks++;
    }
  }
  return leaks;
}

/**
 * @brief Compute total live memory bytes allocated since the checkpoint.
 * @param cp Checkpoint record to evaluate against.
 * @return Active bytes allocated since checkpoint.
 */
size_t boot_checkpoint_alloc_size(boot_checkpoint_t cp) {
  size_t total = 0;
  for (size_t i = 0; i < g_alloc_count; i++) {
    if (!g_allocs[i].freed && g_allocs[i].alloc_id > cp.min_alloc_id) {
      total += g_allocs[i].size;
    }
  }
  return total;
}

/**
 * @brief Count currently active (unfreed) allocations.
 * @return Number of live allocations.
 */
size_t boot_live_alloc_count(void) {
  return g_live_alloc_count;
}

/**
 * @brief Cumulative total of memory allocation calls (malloc, calloc, realloc).
 * @return Cumulative allocation count.
 */
size_t boot_total_alloc_count(void) {
  return g_total_alloc_count;
}

/**
 * @brief Cumulative total of free deallocation calls.
 * @return Cumulative free count.
 */
size_t boot_total_free_count(void) {
  return g_total_free_count;
}

/**
 * @brief Peak memory usage in bytes across the tracking lifetime.
 * @return High-water mark of live allocated memory.
 */
size_t boot_peak_alloc_size(void) {
  return g_peak_alloc_size;
}

/**
 * @brief Count total unreleased memory allocations.
 * @return Number of currently leaking allocations.
 */
size_t boot_leak_count(void) {
  return g_live_alloc_count;
}

/**
 * @brief Query the allocated size of a specific tracked memory pointer.
 * @param ptr Pointer to look up.
 * @return Size in bytes of the allocated block, or 0 if untracked.
 */
size_t boot_ptr_size(void *ptr) {
  if (ptr == NULL) return 0;
  int idx = find_alloc_index(ptr);
  if (idx >= 0 && !g_allocs[idx].freed) {
    return g_allocs[idx].size;
  }
  return 0;
}

/**
 * @brief Check whether a pointer is currently recorded in the tracking table.
 * @param ptr Pointer to look up.
 * @return true if pointer is tracked, false otherwise.
 */
bool boot_is_tracked(void *ptr) {
  if (ptr == NULL) return false;
  return find_alloc_index(ptr) >= 0;
}

/**
 * @brief Reset internal tracking tables and allocation counters.
 */
void boot_reset_tracking(void) {
  g_alloc_count = 0;
  g_live_alloc_count = 0;
  g_total_alloc_count = 0;
  g_total_free_count = 0;
  g_current_alloc_size = 0;
  g_peak_alloc_size = 0;
  g_last_realloc_size = 0;
  g_realloc_count = 0;
  g_fail_alloc_after = -1;
  g_fail_alloc_repeat = 1;
  g_fail_alloc_injected_count = 0;
  memset(g_hash_table, -1, sizeof(g_hash_table));
  g_hash_initialized = true;
}
