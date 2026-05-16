/*
 * runtime_alloc_ms.c — Conservative mark-and-sweep GC backend.
 *
 * Implements the five-symbol GC interface declared in runtime_gc.h.
 * The GC uses a conservative stack scan: every word on the stack that looks
 * like a valid managed pointer (passes the magic-number check) is treated as
 * a live root.  This works correctly on x86-64, aarch64, and riscv64 without
 * any changes to the compiler pipeline.
 *
 * Collection is triggered by rt_gc_hint() after every CALYNDA_GC_COLLECT_INTERVAL
 * allocations (default 10 000; override via the environment variable of the
 * same name at run-time).
 *
 * Thread safety: a single mutex serialises all registry mutations and
 * collection cycles.  rt_gc_hint() uses a non-blocking trylock so that
 * allocation threads are never stalled waiting for another in-flight
 * collection.
 *
 * This backend is linked into calynda_runtime_ms.a and is the default when
 * no --gc flag is passed to the compiler.
 */

#define _POSIX_C_SOURCE 200809L
#include "runtime_internal.h"

#include <pthread.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Tuning defaults
 * ---------------------------------------------------------------------- */

#define MS_DEFAULT_COLLECT_INTERVAL 10000u
#define MS_INITIAL_CAPACITY         64u

/* -------------------------------------------------------------------------
 * Open-addressing pointer hash set
 *
 * Slots:  NULL  = empty
 *         0x1   = tombstone (deleted)
 *         other = live pointer
 * ---------------------------------------------------------------------- */

#define MS_TOMBSTONE ((void *)(uintptr_t)1)

static size_t ms_slot(void *ptr, size_t cap) {
    return ((uintptr_t)ptr >> 4) & (cap - 1);
}

/* Grow set to new_cap (must be a power of two ≥ count).  Returns false OOM. */
static bool ms_set_grow(void ***slots, size_t *cap, size_t *count, size_t new_cap) {
    void **ns = calloc(new_cap, sizeof(*ns));
    size_t i;

    if (!ns) {
        return false;
    }
    for (i = 0; i < *cap; i++) {
        void *p = (*slots)[i];
        if (!p || p == MS_TOMBSTONE) {
            continue;
        }
        /* re-insert without tombstones */
        size_t h = ms_slot(p, new_cap);
        size_t j;
        for (j = 0; j < new_cap; j++) {
            size_t idx = (h + j) & (new_cap - 1);
            if (!ns[idx]) {
                ns[idx] = p;
                break;
            }
        }
    }
    free(*slots);
    *slots = ns;
    *cap   = new_cap;
    (void)count; /* count unchanged; just rehash */
    return true;
}

static bool ms_set_insert(void ***slots, size_t *cap, size_t *count, void *ptr) {
    size_t h, i, idx;

    /* Grow when load factor > 0.6 */
    if (*count + 1 > (*cap * 3) / 5) {
        size_t new_cap = (*cap == 0) ? MS_INITIAL_CAPACITY : (*cap * 2);
        if (!ms_set_grow(slots, cap, count, new_cap)) {
            return false;
        }
    }
    h = ms_slot(ptr, *cap);
    for (i = 0; i < *cap; i++) {
        idx = (h + i) & (*cap - 1);
        if (!(*slots)[idx] || (*slots)[idx] == MS_TOMBSTONE) {
            (*slots)[idx] = ptr;
            (*count)++;
            return true;
        }
        if ((*slots)[idx] == ptr) {
            return true; /* already present */
        }
    }
    return false; /* shouldn't happen after grow */
}

static bool ms_set_contains(void **slots, size_t cap, void *ptr) {
    size_t h, i, idx;

    if (!cap) {
        return false;
    }
    h = ms_slot(ptr, cap);
    for (i = 0; i < cap; i++) {
        idx = (h + i) & (cap - 1);
        if (!slots[idx]) {
            return false;
        }
        if (slots[idx] == MS_TOMBSTONE) {
            continue;
        }
        if (slots[idx] == ptr) {
            return true;
        }
    }
    return false;
}

/* -------------------------------------------------------------------------
 * GC state
 * ---------------------------------------------------------------------- */

static void         **gc_slots;       /* managed objects (owned) */
static size_t         gc_cap;
static size_t         gc_count;

static void         **gc_static_slots; /* static-root objects (not owned) */
static size_t         gc_static_cap;
static size_t         gc_static_count;

static size_t         gc_threshold = MS_DEFAULT_COLLECT_INTERVAL;
static size_t         gc_alloc_count;
static void          *gc_stack_base;  /* recorded at rt_gc_init() */

static pthread_mutex_t gc_mutex = PTHREAD_MUTEX_INITIALIZER;

/* -------------------------------------------------------------------------
 * Implementation (mark, sweep, collect) is in the companion include file.
 * ---------------------------------------------------------------------- */

#include "runtime_alloc_ms_p2.inc"

/* -------------------------------------------------------------------------
 * GC backend interface
 * ---------------------------------------------------------------------- */

void rt_gc_init(void) {
    const char *env;
    volatile char anchor = 0;

    gc_stack_base = (void *)&anchor;

    gc_threshold = MS_DEFAULT_COLLECT_INTERVAL;
    env = getenv("CALYNDA_GC_COLLECT_INTERVAL");
    if (env) {
        unsigned long v = strtoul(env, NULL, 10);
        if (v > 0) {
            gc_threshold = (size_t)v;
        }
    }
}

void *rt_alloc_managed(size_t size) {
    void *ptr;

    /* Trigger collection BEFORE allocating so the GC never encounters a
     * freshly-calloc'd block with an uninitialised magic field. */
    rt_gc_hint();

    ptr = calloc(1, size);

    if (!ptr) {
        fprintf(stderr, "runtime: out of memory\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }

    pthread_mutex_lock(&gc_mutex);
    if (!ms_set_insert(&gc_slots, &gc_cap, &gc_count, ptr)) {
        pthread_mutex_unlock(&gc_mutex);
        free(ptr);
        fprintf(stderr, "runtime: out of memory registering object\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }
    gc_alloc_count++;
    pthread_mutex_unlock(&gc_mutex);

    return ptr;
}

void rt_register_static_managed(void *ptr) {
    pthread_mutex_lock(&gc_mutex);
    ms_set_insert(&gc_static_slots, &gc_static_cap, &gc_static_count, ptr);
    pthread_mutex_unlock(&gc_mutex);
}

void rt_gc_hint(void) {
    if (pthread_mutex_trylock(&gc_mutex) != 0) {
        return; /* another thread is already collecting or allocating */
    }
    if (gc_alloc_count >= gc_threshold) {
        gc_alloc_count = 0;
        gc_do_collect(); /* defined in runtime_alloc_ms_p2.inc */
    }
    pthread_mutex_unlock(&gc_mutex);
}

void rt_gc_shutdown(void) {
    size_t i;

    pthread_mutex_lock(&gc_mutex);
    for (i = 0; i < gc_cap; i++) {
        void *p = gc_slots[i];
        if (p && p != MS_TOMBSTONE) {
            rt_free_managed_object(p);
        }
    }
    free(gc_slots);
    free(gc_static_slots);
    gc_slots        = NULL;
    gc_static_slots = NULL;
    gc_cap = gc_count = gc_static_cap = gc_static_count = 0;
    pthread_mutex_unlock(&gc_mutex);
}
