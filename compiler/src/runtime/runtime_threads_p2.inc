    if (!atomic_object) {
        fprintf(stderr, "runtime: out of memory while creating atomic object\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }
    atomic_store_explicit(&atomic_object->value,
                          initial_value,
                          memory_order_seq_cst);
    return rt_make_object_word(atomic_object);
}

CalyndaRtWord __calynda_rt_atomic_load(CalyndaRtWord atomic_handle) {
    CalyndaRtAtomic *atomic_object =
        (CalyndaRtAtomic *)(void *)calynda_rt_as_object(atomic_handle);

    if (!atomic_object || atomic_object->header.kind != CALYNDA_RT_OBJECT_ATOMIC) {
        fprintf(stderr, "runtime: attempted load on non-atomic value\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }
    return atomic_load_explicit(&atomic_object->value, memory_order_seq_cst);
}

void __calynda_rt_atomic_store(CalyndaRtWord atomic_handle,
                               CalyndaRtWord value) {
    CalyndaRtAtomic *atomic_object =
        (CalyndaRtAtomic *)(void *)calynda_rt_as_object(atomic_handle);

    if (!atomic_object || atomic_object->header.kind != CALYNDA_RT_OBJECT_ATOMIC) {
        fprintf(stderr, "runtime: attempted store on non-atomic value\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }
    atomic_store_explicit(&atomic_object->value, value, memory_order_seq_cst);
}

CalyndaRtWord __calynda_rt_atomic_exchange(CalyndaRtWord atomic_handle,
                                           CalyndaRtWord new_value) {
    CalyndaRtAtomic *atomic_object =
        (CalyndaRtAtomic *)(void *)calynda_rt_as_object(atomic_handle);

    if (!atomic_object || atomic_object->header.kind != CALYNDA_RT_OBJECT_ATOMIC) {
        fprintf(stderr, "runtime: attempted exchange on non-atomic value\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }
    return atomic_exchange_explicit(&atomic_object->value,
                                    new_value,
                                    memory_order_seq_cst);
}
