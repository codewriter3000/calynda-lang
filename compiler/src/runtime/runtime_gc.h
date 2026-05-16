#ifndef RUNTIME_GC_H
#define RUNTIME_GC_H

/*
 * GC backend interface.
 *
 * Each GC archive must export all five symbols below.  The active backend is
 * chosen at link time:
 *
 *   calynda_runtime.a    — legacy append-only registry  (--gc=legacy)
 *   calynda_runtime_ms.a — mark-and-sweep               (--gc=marksweep, default)
 *   <custom.a>           — user-supplied plugin         (--gc-plugin=path.a)
 *
 * None of the five symbols may block indefinitely; they must be async-signal-
 * safe with respect to the Calynda failure-context machinery (setjmp/longjmp).
 */

#include <stddef.h>

/* Called once, from the main thread, before any managed allocation. */
void rt_gc_init(void);

/*
 * Allocate and zero-initialise 'size' bytes of GC-tracked memory.
 * Never returns NULL; aborts the process on out-of-memory.
 * The returned block MUST start with a CalyndaRtObjectHeader whose fields the
 * caller initialises immediately after this returns.
 */
void *rt_alloc_managed(size_t size);

/*
 * Register a statically-allocated (non-heap) object as a GC root.
 * The object will never be freed by the GC; it merely acts as a root from
 * which live heap objects are reachable.
 */
void rt_register_static_managed(void *ptr);

/*
 * Called after each managed allocation.  The backend may use this to decide
 * whether to run a collection cycle.  Must be cheap when no collection is
 * needed.
 */
void rt_gc_hint(void);

/* Called once at process exit to release all remaining managed memory. */
void rt_gc_shutdown(void);

#endif /* RUNTIME_GC_H */
