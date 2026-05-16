/* Bounds-checked manual{} memory operations.
 *
 * A growable shadow registry maps each live allocation's base pointer to its
 * allocated size.  Bounds violations abort with a diagnostic message.
 */

#include "runtime_internal.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uintptr_t base;
    size_t    size;
} BcEntry;

static BcEntry *bc_entries = NULL;
static size_t   bc_count = 0;
static size_t   bc_capacity = 0;
static pthread_mutex_t bc_mutex = PTHREAD_MUTEX_INITIALIZER;

static void bc_lock(void) {
    if (pthread_mutex_lock(&bc_mutex) != 0) {
        fprintf(stderr, "calynda bounds-check: mutex lock failed\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }
}

static void bc_unlock(void) {
    if (pthread_mutex_unlock(&bc_mutex) != 0) {
        fprintf(stderr, "calynda bounds-check: mutex unlock failed\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }
}

static void bc_ensure_capacity(size_t needed) {
    BcEntry *grown_entries;
    size_t new_capacity;

    if (bc_capacity >= needed) {
        return;
    }
    new_capacity = bc_capacity == 0 ? 64 : bc_capacity;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }
    grown_entries = realloc(bc_entries, new_capacity * sizeof(*bc_entries));
    if (!grown_entries) {
        fprintf(stderr, "calynda bounds-check: shadow registry resize failed\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }
    bc_entries = grown_entries;
    bc_capacity = new_capacity;
}

static BcEntry *bc_find_exact(uintptr_t base) {
    size_t i;

    for (i = 0; i < bc_count; i++) {
        if (bc_entries[i].base == base) {
            return &bc_entries[i];
        }
    }
    return NULL;
}

static BcEntry *bc_find_owner(uintptr_t ptr) {
    size_t i;

    for (i = 0; i < bc_count; i++) {
        uintptr_t base = bc_entries[i].base;

        if (ptr >= base && (size_t)(ptr - base) < bc_entries[i].size) {
            return &bc_entries[i];
        }
    }
    return NULL;
}

static void bc_register(uintptr_t base, size_t size) {
    bc_ensure_capacity(bc_count + 1);
    bc_entries[bc_count].base = base;
    bc_entries[bc_count].size = size;
    bc_count++;
}

static void bc_unregister(uintptr_t base) {
    BcEntry *entry = bc_find_exact(base);

    if (entry) {
        *entry = bc_entries[bc_count - 1];
        bc_count--;
    }
}

/* Check that ptr lies within [base, base+size). */
static void bc_check_access(uintptr_t ptr) {
    BcEntry *owner = bc_find_owner(ptr);
    if (owner) {
        return;
    }
    /* Try to find the nearest allocation to give a helpful hint */
    {
        BcEntry *nearest = NULL;
        uintptr_t nearest_dist = (uintptr_t)-1;
        size_t i;
        for (i = 0; i < bc_count; i++) {
            uintptr_t base = bc_entries[i].base;
            uintptr_t dist = ptr < base
                ? base - ptr
                : ptr - (base + bc_entries[i].size - 1);
            if (dist < nearest_dist) {
                nearest_dist = dist;
                nearest = &bc_entries[i];
            }
        }
        if (nearest) {
            fprintf(stderr,
                    "calynda bounds-check: out-of-bounds access at %p\n"
                    "   nearest allocation: [%p, %p) (size %zu)\n",
                    (void *)ptr,
                    (void *)nearest->base,
                    (void *)(nearest->base + nearest->size),
                    nearest->size);
        } else {
            fprintf(stderr,
                    "calynda bounds-check: out-of-bounds access at %p (no tracked allocations)\n",
                    (void *)ptr);
        }
    }
    rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
}

CalyndaRtWord __calynda_bc_malloc(CalyndaRtWord size) {
    void *p = malloc((size_t)size);
    if (!p) {
        fprintf(stderr, "calynda bounds-check: malloc failed\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }
    bc_lock();
    bc_register((uintptr_t)p, (size_t)size);
    bc_unlock();
    return (CalyndaRtWord)(uintptr_t)p;
}

CalyndaRtWord __calynda_bc_calloc(CalyndaRtWord n, CalyndaRtWord size) {
    void *p = calloc((size_t)n, (size_t)size);
    if (!p) {
        fprintf(stderr, "calynda bounds-check: calloc failed\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }
    bc_lock();
    bc_register((uintptr_t)p, (size_t)(n * size));
    bc_unlock();
    return (CalyndaRtWord)(uintptr_t)p;
}

CalyndaRtWord __calynda_bc_realloc(CalyndaRtWord ptr, CalyndaRtWord new_size) {
    void *old_p = (void *)(uintptr_t)ptr;
    void *new_p;

    bc_lock();
    bc_unregister((uintptr_t)old_p);
    new_p = realloc(old_p, (size_t)new_size);
    if (!new_p) {
        bc_unlock();
        fprintf(stderr, "calynda bounds-check: realloc failed\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }
    bc_register((uintptr_t)new_p, (size_t)new_size);
    bc_unlock();
    return (CalyndaRtWord)(uintptr_t)new_p;
}

void __calynda_bc_free(CalyndaRtWord ptr) {
    void *p = (void *)(uintptr_t)ptr;

    bc_lock();
    bc_unregister((uintptr_t)p);
    free(p);
    bc_unlock();
}

CalyndaRtWord __calynda_bc_stackalloc(CalyndaRtWord size) {
    return __calynda_bc_malloc(size);
}

CalyndaRtWord __calynda_bc_deref(CalyndaRtWord ptr) {
    CalyndaRtWord value;

    bc_lock();
    bc_check_access((uintptr_t)ptr);
    value = *((CalyndaRtWord *)(uintptr_t)ptr);
    bc_unlock();
    return value;
}

void __calynda_bc_store(CalyndaRtWord ptr, CalyndaRtWord value) {
    bc_lock();
    bc_check_access((uintptr_t)ptr);
    *((CalyndaRtWord *)(uintptr_t)ptr) = value;
    bc_unlock();
}

CalyndaRtWord __calynda_bc_offset(CalyndaRtWord ptr, CalyndaRtWord idx,
                                  CalyndaRtWord elem_size) {
    uintptr_t result = (uintptr_t)ptr + (uintptr_t)(idx * elem_size);

    bc_lock();
    bc_check_access((uintptr_t)ptr);
    bc_unlock();
    return (CalyndaRtWord)result;
}
