/*
 * runtime_alloc_legacy.c — Legacy GC backend.
 *
 * Implements the five-symbol GC interface declared in runtime_gc.h using the
 * original append-only object registry.  No collection ever occurs; all
 * managed memory is freed in bulk at process exit via rt_gc_shutdown().
 *
 * This backend is linked into calynda_runtime.a and can be selected at
 * compile time with:  calynda build --gc=legacy <source>
 */

#include "runtime_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Registry data structure
 * ---------------------------------------------------------------------- */

typedef struct {
    void *pointer;
    bool  owned;
} LegacyEntry;

typedef struct {
    LegacyEntry *items;
    size_t       count;
    size_t       capacity;
} LegacyRegistry;

static LegacyRegistry s_registry;

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

static bool legacy_append(void *pointer, bool owned) {
    size_t i;

    if (!pointer) {
        return true;
    }
    /* Deduplicate to keep the contract identical to the old registry. */
    for (i = 0; i < s_registry.count; i++) {
        if (s_registry.items[i].pointer == pointer) {
            return true;
        }
    }
    if (!rt_reserve_items((void **)&s_registry.items,
                          &s_registry.capacity,
                          s_registry.count + 1,
                          sizeof(*s_registry.items))) {
        return false;
    }
    s_registry.items[s_registry.count].pointer = pointer;
    s_registry.items[s_registry.count].owned   = owned;
    s_registry.count++;
    return true;
}

/* -------------------------------------------------------------------------
 * GC backend interface
 * ---------------------------------------------------------------------- */

void rt_gc_init(void) {
    /* Nothing to initialise; the registry starts zeroed. */
}

void *rt_alloc_managed(size_t size) {
    void *ptr = calloc(1, size);

    if (!ptr) {
        fprintf(stderr, "runtime: out of memory\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }
    if (!legacy_append(ptr, true)) {
        free(ptr);
        fprintf(stderr, "runtime: out of memory registering object\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }
    return ptr;
}

void rt_register_static_managed(void *ptr) {
    /* Static objects are registered with owned=false so they are never freed. */
    legacy_append(ptr, false);
}

void rt_gc_hint(void) {
    /* Legacy backend never collects; this is a no-op. */
}

void rt_gc_shutdown(void) {
    size_t i;

    for (i = 0; i < s_registry.count; i++) {
        if (s_registry.items[i].owned) {
            rt_free_managed_object(s_registry.items[i].pointer);
        }
    }
    free(s_registry.items);
    memset(&s_registry, 0, sizeof(s_registry));
}
