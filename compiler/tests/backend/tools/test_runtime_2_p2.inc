
/* ------------------------------------------------------------------ */
/*  alpha.2: Future spawn / get / cancel                              */
/* ------------------------------------------------------------------ */

static CalyndaRtWord future_return_99(const CalyndaRtWord *captures,
                                      size_t capture_count,
                                      const CalyndaRtWord *arguments,
                                      size_t argument_count) {
    (void)captures;
    (void)capture_count;
    (void)arguments;
    (void)argument_count;
    return 99;
}

static CalyndaRtWord future_sleep_and_return(const CalyndaRtWord *captures,
                                             size_t capture_count,
                                             const CalyndaRtWord *arguments,
                                             size_t argument_count) {
    (void)captures;
    (void)capture_count;
    (void)arguments;
    (void)argument_count;
    struct timespec ts = { 10, 0 };
    nanosleep(&ts, NULL);
    return 77;
}

void test_runtime_future_spawn_get_cancel(void) {
    CalyndaRtWord closure_get;
    CalyndaRtWord closure_cancel;
    CalyndaRtWord future_get;
    CalyndaRtWord future_cancel;
    CalyndaRtWord result;

    /* future_get: spawn a fast callable, get its result */
    closure_get = __calynda_rt_closure_new(future_return_99, 0, NULL);
    future_get  = __calynda_rt_future_spawn(closure_get);
    ASSERT_TRUE(calynda_rt_is_object(future_get), "future spawn returns a valid object");

    result = __calynda_rt_future_get(future_get);
    ASSERT_EQ_WORD(99, result, "future get returns the callable's return value");

    /* Double-get must not crash */
    result = __calynda_rt_future_get(future_get);
    ASSERT_EQ_WORD(99, result, "future get on already-joined future returns stored result");

    /* future_cancel: spawn a slow callable, cancel it before it finishes */
    closure_cancel = __calynda_rt_closure_new(future_sleep_and_return, 0, NULL);
    future_cancel  = __calynda_rt_future_spawn(closure_cancel);
    ASSERT_TRUE(calynda_rt_is_object(future_cancel), "future cancel: spawn returns valid object");

    struct timespec ts = { 0, 10000000 }; /* 10 ms */
    nanosleep(&ts, NULL);

    __calynda_rt_future_cancel(future_cancel);
    ASSERT_TRUE(1, "future cancel completes without crash");

    /* Double-cancel must not crash */
    __calynda_rt_future_cancel(future_cancel);
    ASSERT_TRUE(1, "future double-cancel does not crash");
}

/* ------------------------------------------------------------------ */
/*  alpha.2: Atomic<T> new / load / store / exchange                 */
/* ------------------------------------------------------------------ */

void test_runtime_atomic_operations(void) {
    CalyndaRtWord atom;
    CalyndaRtWord loaded;
    CalyndaRtWord old_val;

    atom = __calynda_rt_atomic_new(42);
    ASSERT_TRUE(calynda_rt_is_object(atom), "atomic new returns a valid object");

    loaded = __calynda_rt_atomic_load(atom);
    ASSERT_EQ_WORD(42, loaded, "atomic load reads the initial value");

    __calynda_rt_atomic_store(atom, 100);
    loaded = __calynda_rt_atomic_load(atom);
    ASSERT_EQ_WORD(100, loaded, "atomic store updates the value");

    old_val = __calynda_rt_atomic_exchange(atom, 200);
    ASSERT_EQ_WORD(100, old_val, "atomic exchange returns the old value");

    loaded = __calynda_rt_atomic_load(atom);
    ASSERT_EQ_WORD(200, loaded, "atomic exchange installs the new value");
}

void test_runtime_fence_helper_is_callable(void) {
    __calynda_rt_fence();
    ASSERT_TRUE(1, "fence helper completes without crashing");
}

void test_runtime_mmio_helpers_preserve_values(void) {
    uint32_t scratch = 0;
    CalyndaRtWord loaded;

    __calynda_mmio_store_sized((CalyndaRtWord)(uintptr_t)&scratch,
                               (CalyndaRtWord)0x11223344u,
                               (CalyndaRtWord)sizeof(scratch));
    loaded = __calynda_mmio_deref_sized((CalyndaRtWord)(uintptr_t)&scratch,
                                        (CalyndaRtWord)sizeof(scratch));

    ASSERT_EQ_WORD((CalyndaRtWord)0x11223344u,
                   (CalyndaRtWord)scratch,
                   "mmio sized store writes the volatile target");
    ASSERT_EQ_WORD((CalyndaRtWord)0x11223344u,
                   loaded,
                   "mmio sized deref reads the volatile target");
}

void test_runtime_cache_helpers_are_callable(void) {
    uintptr_t scratch = 0;

    __calynda_rt_cache_clean((CalyndaRtWord)(uintptr_t)&scratch);
    __calynda_rt_cache_final();
    ASSERT_TRUE(1, "cache helpers complete without crashing");
}
