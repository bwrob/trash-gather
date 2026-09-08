#define BOOTLIB_NO_OVERRIDE
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/**
 * Maximum capacity for tracking memory allocations in tests.
 */
#define MAX_BOOT_ALLOCATIONS 10240

/**
 * Internal tracking structure for a single memory allocation.
 */
typedef struct {
  void *ptr;         /**< Pointer returned by allocator */
  size_t size;       /**< Allocation size in bytes */
  bool freed;        /**< Deallocation status flag */
  const char *file;  /**< Source file where allocation occurred */
  int line;          /**< Line number where allocation occurred */
} boot_alloc_t;

static boot_alloc_t g_allocs[MAX_BOOT_ALLOCATIONS];
static size_t g_alloc_count = 0;

/**
 * Register or update an active allocation in the global tracking array.
 */
static void track_add(void *ptr, size_t size, const char *file, int line) {
  if (ptr == NULL) return;

  for (size_t i = 0; i < g_alloc_count; i++) {
    if (g_allocs[i].ptr == ptr) {
      g_allocs[i].size = size;
      g_allocs[i].freed = false;
      g_allocs[i].file = file;
      g_allocs[i].line = line;
      return;
    }
  }

  if (g_alloc_count < MAX_BOOT_ALLOCATIONS) {
    g_allocs[g_alloc_count].ptr = ptr;
    g_allocs[g_alloc_count].size = size;
    g_allocs[g_alloc_count].freed = false;
    g_allocs[g_alloc_count].file = file;
    g_allocs[g_alloc_count].line = line;
    g_alloc_count++;
  } else {
    fprintf(stderr, "[bootlib warning] Max allocation tracking capacity reached (%d)\n", MAX_BOOT_ALLOCATIONS);
  }
}

/**
 * Mark a pointer as freed in the tracking table.
 */
static void track_free(void *ptr) {
  if (ptr == NULL) return;
  for (size_t i = 0; i < g_alloc_count; i++) {
    if (g_allocs[i].ptr == ptr) {
      g_allocs[i].freed = true;
      return;
    }
  }
}

static size_t g_last_realloc_size = 0;
static size_t g_realloc_count = 0;
static int g_fail_alloc_after = -1;

void boot_set_fail_alloc_after(int count) {
  g_fail_alloc_after = count;
}

/**
 * Helper to check if allocation failure simulation should trigger.
 */
static bool check_should_fail(void) {
  if (g_fail_alloc_after == 0) {
    g_fail_alloc_after = -1;
    return true;
  }
  if (g_fail_alloc_after > 0) {
    g_fail_alloc_after--;
  }
  return false;
}

void *boot_malloc(size_t size, const char *file, int line) {
  if (check_should_fail()) return NULL;
  void *ptr = malloc(size);
  track_add(ptr, size, file, line);
  return ptr;
}

void boot_free(void *ptr) {
  if (ptr == NULL) return;
  track_free(ptr);
  free(ptr);
}

void *boot_realloc(void *ptr, size_t size, const char *file, int line) {
  if (check_should_fail()) return NULL;
  g_last_realloc_size = size;
  g_realloc_count++;
  if (ptr != NULL) {
    track_free(ptr);
  }
  void *new_ptr = realloc(ptr, size);
  if (new_ptr != NULL) {
    track_add(new_ptr, size, file, line);
  }
  return new_ptr;
}

void *boot_calloc(size_t count, size_t size, const char *file, int line) {
  if (check_should_fail()) return NULL;
  void *ptr = calloc(count, size);
  track_add(ptr, count * size, file, line);
  return ptr;
}

bool boot_is_freed(void *ptr) {
  if (ptr == NULL) return true;
  for (size_t i = 0; i < g_alloc_count; i++) {
    if (g_allocs[i].ptr == ptr) {
      return g_allocs[i].freed;
    }
  }
  return true;
}

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

size_t boot_alloc_size(void) {
  size_t total = 0;
  for (size_t i = 0; i < g_alloc_count; i++) {
    if (!g_allocs[i].freed) {
      total += g_allocs[i].size;
    }
  }
  return total;
}

size_t boot_last_realloc_size(void) {
  return g_last_realloc_size;
}

size_t boot_realloc_count(void) {
  return g_realloc_count;
}

void boot_reset_tracking(void) {
  g_alloc_count = 0;
  g_last_realloc_size = 0;
  g_realloc_count = 0;
  g_fail_alloc_after = -1;
}
