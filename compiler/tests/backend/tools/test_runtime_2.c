#define _POSIX_C_SOURCE 200809L

#include "runtime.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

extern int tests_run;
extern int tests_passed;
extern int tests_failed;

#define ASSERT_TRUE(condition, msg) do {                                    \
    tests_run++;                                                            \
    if (condition) {                                                        \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s\n",                            \
                __FILE__, __LINE__, (msg));                                 \
    }                                                                       \
} while (0)
#define ASSERT_EQ_WORD(expected, actual, msg) do {                          \
    tests_run++;                                                            \
    if ((expected) == (actual)) {                                           \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected %lld, got %lld\n",    \
                __FILE__, __LINE__, (msg),                                  \
                (long long)(expected), (long long)(actual));                \
    }                                                                       \
} while (0)

void test_runtime_checked_stackalloc_uses_tracked_scratch_storage(void) {
    CalyndaRtWord scratch = __calynda_bc_stackalloc(16);

    __calynda_bc_store(scratch, 91);
    ASSERT_EQ_WORD(91,
                   __calynda_bc_deref(scratch),
                   "checked stackalloc scratch storage supports tracked store/deref");
    __calynda_bc_free(scratch);
}

void test_runtime_checked_registry_grows_past_legacy_pointer_limit(void) {
    enum { LARGE_ALLOCATION_COUNT = 900 };
    CalyndaRtWord handles[LARGE_ALLOCATION_COUNT];
    size_t i;

    for (i = 0; i < LARGE_ALLOCATION_COUNT; i++) {
        handles[i] = __calynda_bc_malloc(8);
        __calynda_bc_store(handles[i], (CalyndaRtWord)(i + 1));
    }

    ASSERT_EQ_WORD(1,
                   __calynda_bc_deref(handles[0]),
                   "checked registry preserves the first tracked allocation past the legacy cap");
    ASSERT_EQ_WORD(451,
                   __calynda_bc_deref(handles[450]),
                   "checked registry preserves middle tracked allocations past the legacy cap");
    ASSERT_EQ_WORD(900,
                   __calynda_bc_deref(handles[LARGE_ALLOCATION_COUNT - 1]),
                   "checked registry preserves the last tracked allocation past the legacy cap");

    for (i = 0; i < LARGE_ALLOCATION_COUNT; i++) {
        __calynda_bc_free(handles[i]);
    }

    handles[0] = __calynda_bc_malloc(8);
    __calynda_bc_store(handles[0], 77);
    ASSERT_EQ_WORD(77,
                   __calynda_bc_deref(handles[0]),
                   "checked registry remains reusable after releasing many tracked allocations");
    __calynda_bc_free(handles[0]);
}

typedef struct {
    size_t thread_index;
    int    failures;
} CheckedRegistryThreadArgs;

static void *run_checked_registry_thread(void *opaque) {
    enum {
        CONCURRENT_THREAD_ITERATIONS = 128,
        CONCURRENT_THREAD_ALLOCS = 16
    };
    CheckedRegistryThreadArgs *args = (CheckedRegistryThreadArgs *)opaque;
    size_t iteration;

    for (iteration = 0; iteration < CONCURRENT_THREAD_ITERATIONS; iteration++) {
        CalyndaRtWord handles[CONCURRENT_THREAD_ALLOCS];
        size_t slot;

        for (slot = 0; slot < CONCURRENT_THREAD_ALLOCS; slot++) {
            CalyndaRtWord expected =
                (CalyndaRtWord)(args->thread_index * 100000 + iteration * 100 + slot);
            handles[slot] = __calynda_bc_malloc(sizeof(CalyndaRtWord));
            __calynda_bc_store(handles[slot], expected);
            if (__calynda_bc_deref(handles[slot]) != expected) {
                args->failures++;
            }
        }

        for (slot = 0; slot < CONCURRENT_THREAD_ALLOCS; slot++) {
            __calynda_bc_free(handles[slot]);
        }
    }

    return NULL;
}

void test_runtime_checked_registry_is_thread_safe_under_concurrency(void) {
    enum { CONCURRENT_THREAD_COUNT = 6 };
    pthread_t threads[CONCURRENT_THREAD_COUNT];
    CheckedRegistryThreadArgs args[CONCURRENT_THREAD_COUNT];
    size_t i;

    for (i = 0; i < CONCURRENT_THREAD_COUNT; i++) {
        args[i].thread_index = i + 1;
        args[i].failures = 0;
        ASSERT_EQ_WORD(0,
                       (CalyndaRtWord)pthread_create(&threads[i],
                                                     NULL,
                                                     run_checked_registry_thread,
                                                     &args[i]),
                       "checked registry spawns worker threads");
    }

    for (i = 0; i < CONCURRENT_THREAD_COUNT; i++) {
        ASSERT_EQ_WORD(0,
                       (CalyndaRtWord)pthread_join(threads[i], NULL),
                       "checked registry joins worker threads");
        ASSERT_EQ_WORD(0,
                       (CalyndaRtWord)args[i].failures,
                       "checked registry preserves values under concurrent access");
    }
}

static CalyndaRtWord hetero_cleanup_array = 0;
static CalyndaRtWord hetero_cleanup_string = 0;

static CalyndaRtWord capture_hetero_cleanup_handles(CalyndaRtWord arguments) {
    CalyndaRtWord elements[2];
    CalyndaRtTypeTag generic_tags[1] = { CALYNDA_RT_TYPE_RAW_WORD };
    CalyndaRtTypeTag tags[2] = { CALYNDA_RT_TYPE_INT64, CALYNDA_RT_TYPE_STRING };
    CalyndaRtTypeDescriptor type_desc = { "arr", 1, generic_tags, 2, NULL, tags };

    (void)arguments;
    hetero_cleanup_string = calynda_rt_make_string_copy("owned");
    elements[0] = 9;
    elements[1] = hetero_cleanup_string;
    hetero_cleanup_array = __calynda_rt_hetero_array_new(&type_desc, 2, elements);
    if (!calynda_rt_is_object(hetero_cleanup_string) ||
        !calynda_rt_is_object(hetero_cleanup_array)) {
        return 19;
    }
    return 17;
}

void test_runtime_start_process_cleans_hetero_arrays_and_nested_objects(void) {
    char *argv[] = { (char *)"program", NULL };
    int exit_code;

    hetero_cleanup_array = 0;
    hetero_cleanup_string = 0;
    exit_code = calynda_rt_start_process(capture_hetero_cleanup_handles, 1, argv);

    ASSERT_EQ_WORD(17,
                   (CalyndaRtWord)exit_code,
                   "start process creates hetero arrays and nested objects before cleanup");
    ASSERT_TRUE(hetero_cleanup_array != 0 && hetero_cleanup_string != 0,
                "hetero cleanup test captures both object handles");
    ASSERT_TRUE(!calynda_rt_is_object(hetero_cleanup_array),
                "start process cleanup releases hetero array containers");
    ASSERT_TRUE(!calynda_rt_is_object(hetero_cleanup_string),
                "start process cleanup releases nested hetero array objects independently");
}


/* ------------------------------------------------------------------ */
/*  G-RT-4: __calynda_deref_sized / __calynda_store_sized for every  */
/*          primitive width (1, 2, 4, 8 bytes)                        */
/* ------------------------------------------------------------------ */
void test_runtime_deref_sized_and_store_sized_primitive_widths(void) {
    uint8_t buf[8] = {0};
    CalyndaRtWord ptr = (CalyndaRtWord)(uintptr_t)buf;

    /* 1-byte round-trip */
    __calynda_store_sized(ptr, 0xAB, 1);
    ASSERT_EQ_WORD(0xAB, __calynda_deref_sized(ptr, 1),
                   "1-byte store/deref round-trip");

    /* 2-byte round-trip */
    __calynda_store_sized(ptr, 0x1234, 2);
    ASSERT_EQ_WORD(0x1234, __calynda_deref_sized(ptr, 2),
                   "2-byte store/deref round-trip");

    /* 4-byte round-trip */
    __calynda_store_sized(ptr, 0xDEADBEEFU, 4);
    ASSERT_EQ_WORD(0xDEADBEEFU, __calynda_deref_sized(ptr, 4),
                   "4-byte store/deref round-trip");

    /* 8-byte round-trip */
    __calynda_store_sized(ptr, (CalyndaRtWord)0x0102030405060708LL, 8);
    ASSERT_EQ_WORD((CalyndaRtWord)0x0102030405060708LL,
                   __calynda_deref_sized(ptr, 8),
                   "8-byte store/deref round-trip");
}

/* ------------------------------------------------------------------ */
/*  alpha.2: Thread cancel                                            */
/* ------------------------------------------------------------------ */

static volatile int cancel_thread_ran = 0;

static CalyndaRtWord cancel_thread_body(const CalyndaRtWord *captures,
                                        size_t capture_count,
                                        const CalyndaRtWord *arguments,
                                        size_t argument_count) {
    (void)captures;
    (void)capture_count;
    (void)arguments;
    (void)argument_count;
    cancel_thread_ran = 1;
    /* Sleep long enough for the test to cancel us */
    struct timespec ts = { 10, 0 };
    nanosleep(&ts, NULL);
    return 0;
}

void test_runtime_thread_cancel_stops_thread(void) {
    CalyndaRtWord closure  = __calynda_rt_closure_new(cancel_thread_body, 0, NULL);
    CalyndaRtWord thread   = __calynda_rt_thread_spawn(closure);

    /* Give the thread a moment to start */
    struct timespec ts = { 0, 20000000 }; /* 20 ms */
    nanosleep(&ts, NULL);

    __calynda_rt_thread_cancel(thread);

    ASSERT_TRUE(calynda_rt_is_object(thread), "thread cancel: handle is still a valid object");
    /* A second cancel on an already-joined thread must not crash */
    __calynda_rt_thread_cancel(thread);
    ASSERT_TRUE(1, "thread cancel: double-cancel does not crash");
}

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
