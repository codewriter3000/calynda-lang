/*
 * runtime_alloc_ms_p2.inc — continuation of runtime_alloc_ms.c.
 *
 * Contains:
 *   - gc_mark_value / gc_mark_object_fields  (recursive mark traversal)
 *   - gc_mark_stack                          (conservative stack scan)
 *   - gc_mark_statics                        (scan registered static roots)
 *   - gc_sweep                               (reclaim unmarked objects)
 *   - gc_do_collect                          (full collection cycle)
 *
 * Included directly into runtime_alloc_ms.c; not a standalone translation
 * unit.  All symbols are static except gc_do_collect which is called from
 * rt_gc_hint() / rt_gc_shutdown() in the parent file.
 *
 * Mark bit: bit 0 of CalyndaRtObjectHeader.gc_word.
 */

/* -------------------------------------------------------------------------
 * Mark helpers
 * ---------------------------------------------------------------------- */

static void gc_mark_value(CalyndaRtWord word);

static void gc_mark_object_fields(CalyndaRtObjectHeader *header) {
    size_t i;

    switch ((CalyndaRtObjectKind)header->kind) {
    case CALYNDA_RT_OBJECT_ARRAY: {
        const CalyndaRtArray *arr = (const CalyndaRtArray *)(const void *)header;
        for (i = 0; i < arr->count; i++) {
            gc_mark_value(arr->elements[i]);
        }
        break;
    }
    case CALYNDA_RT_OBJECT_CLOSURE: {
        const CalyndaRtClosure *cl = (const CalyndaRtClosure *)(const void *)header;
        for (i = 0; i < cl->capture_count; i++) {
            gc_mark_value(cl->captures[i]);
        }
        break;
    }
    case CALYNDA_RT_OBJECT_UNION: {
        const CalyndaRtUnion *u = (const CalyndaRtUnion *)(const void *)header;
        gc_mark_value(u->payload);
        break;
    }
    case CALYNDA_RT_OBJECT_HETERO_ARRAY: {
        const CalyndaRtHeteroArray *ha =
            (const CalyndaRtHeteroArray *)(const void *)header;
        for (i = 0; i < ha->count; i++) {
            gc_mark_value(ha->elements[i]);
        }
        break;
    }
    case CALYNDA_RT_OBJECT_FUTURE: {
        const CalyndaRtFuture *f = (const CalyndaRtFuture *)(const void *)header;
        gc_mark_value(f->callable);
        gc_mark_value(f->result);
        break;
    }
    default:
        /* string, thread, mutex, atomic, package, extern_callable: no refs */
        break;
    }
}

static void gc_mark_value(CalyndaRtWord word) {
    CalyndaRtObjectHeader *header;

    /* Fast magic check (O(1)); rejects non-pointers and non-objects. */
    header = (CalyndaRtObjectHeader *)calynda_rt_as_object(word);
    if (!header) {
        return;
    }
    /* Verify the pointer is actually in our managed set. */
    if (!ms_set_contains(gc_slots, gc_cap, (void *)header)) {
        return;
    }
    if (header->gc_word & 1) {
        return; /* already marked */
    }
    header->gc_word |= 1;
    gc_mark_object_fields(header);
}

/* -------------------------------------------------------------------------
 * Conservative stack scan
 * ---------------------------------------------------------------------- */

static void gc_mark_stack(void) {
    volatile char anchor = 0;
    void *stack_top = (void *)&anchor;
    uintptr_t *p_start;
    uintptr_t *p_end;
    uintptr_t *p;

    if (!gc_stack_base) {
        return;
    }

    /* Stack grows downward: top < base on all supported platforms. */
    if ((uintptr_t)stack_top < (uintptr_t)gc_stack_base) {
        p_start = (uintptr_t *)stack_top;
        p_end   = (uintptr_t *)gc_stack_base;
    } else {
        p_start = (uintptr_t *)gc_stack_base;
        p_end   = (uintptr_t *)stack_top;
    }

    for (p = p_start; p < p_end; p++) {
        gc_mark_value((CalyndaRtWord)*p);
    }
}

/* -------------------------------------------------------------------------
 * Static root scan
 * ---------------------------------------------------------------------- */

static void gc_mark_statics(void) {
    size_t i;

    for (i = 0; i < gc_static_cap; i++) {
        void *p = gc_static_slots[i];
        if (p && p != MS_TOMBSTONE) {
            gc_mark_value((CalyndaRtWord)(uintptr_t)p);
        }
    }
}

/* -------------------------------------------------------------------------
 * Sweep phase
 * ---------------------------------------------------------------------- */

static void gc_sweep(void) {
    size_t i;

    for (i = 0; i < gc_cap; i++) {
        void *p = gc_slots[i];
        CalyndaRtObjectHeader *header;

        if (!p || p == MS_TOMBSTONE) {
            continue;
        }
        header = (CalyndaRtObjectHeader *)p;
        if (header->gc_word & 1) {
            header->gc_word &= ~(uintptr_t)1; /* clear mark bit for next cycle */
        } else {
            /* Unreachable: free and remove from set. */
            gc_slots[i] = MS_TOMBSTONE;
            gc_count--;
            rt_free_managed_object(p);
        }
    }
}

/* -------------------------------------------------------------------------
 * Full collection cycle  (called with gc_mutex held)
 * ---------------------------------------------------------------------- */

static void gc_do_collect(void) {
    jmp_buf spill_buf;

    /* If rt_gc_init() was never called (e.g. boot-mode programs) we have no
     * stack base, so we cannot identify any roots.  Skip the collection to
     * avoid freeing live objects. */
    if (!gc_stack_base) {
        return;
    }

    /*
     * setjmp spills all callee-saved (and, for many ABIs, caller-saved)
     * registers to the stack so that gc_mark_stack() can find any object
     * pointers they contain.  The return value is discarded; we never longjmp
     * back here.
     */
    (void)setjmp(spill_buf);

    gc_mark_stack();
    gc_mark_statics();
    gc_sweep();
}
