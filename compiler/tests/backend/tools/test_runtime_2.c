#include "runtime.h"

#include <stdint.h>
#include <stdio.h>

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