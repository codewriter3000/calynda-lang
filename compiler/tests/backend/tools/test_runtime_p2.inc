
static void test_runtime_start_process_boxes_cli_arguments(void) {
    char *argv[] = { (char *)"program", (char *)"hello", (char *)"world", NULL };
    int exit_code;

    startup_argument_count = 0;
    startup_first_argument = NULL;
    exit_code = calynda_rt_start_process(capture_start_arguments, 3, argv);

    ASSERT_EQ_WORD(23, (CalyndaRtWord)exit_code, "start process returns the start unit result as a process exit code");
    ASSERT_EQ_WORD(2,
                   (CalyndaRtWord)startup_argument_count,
                   "start process excludes argv[0] when boxing Calynda args");
    ASSERT_EQ_STR("hello",
                  startup_first_argument,
                  "start process preserves the first user argument as a runtime string");
    ASSERT_TRUE(!calynda_rt_is_object(startup_arguments_handle),
                "start process releases boxed cli arguments before returning");
}

static CalyndaRtWord set_thread_flag(const CalyndaRtWord *captures,
                                     size_t capture_count,
                                     const CalyndaRtWord *arguments,
                                     size_t argument_count) {
    (void)capture_count;
    (void)arguments;
    (void)argument_count;
    *(int *)(uintptr_t)captures[0] = 1;
    return 0;
}

void test_runtime_thread_and_mutex_helpers(void) {
    int ran = 0;
    CalyndaRtWord captures[1] = { (CalyndaRtWord)(uintptr_t)&ran };
    CalyndaRtWord callable = __calynda_rt_closure_new(set_thread_flag, 1, captures);
    CalyndaRtWord thread = __calynda_rt_thread_spawn(callable);
    CalyndaRtWord mutex = __calynda_rt_mutex_new();

    __calynda_rt_mutex_lock(mutex);
    __calynda_rt_mutex_unlock(mutex);
    __calynda_rt_thread_join(thread);

    ASSERT_EQ_WORD(1, (CalyndaRtWord)ran, "thread helper runs callable");
}

int main(void) {
    printf("Running runtime tests...\n\n");

    RUN_TEST(test_runtime_layout_dump_defines_object_model);
    RUN_TEST(test_runtime_array_index_and_store_helpers);
    RUN_TEST(test_runtime_array_car_and_cdr_helpers);
    RUN_TEST(test_runtime_string_index_and_cdr_helpers);
    RUN_TEST(test_runtime_hetero_arrays_reuse_type_descriptors);
    RUN_TEST(test_runtime_closure_new_and_call_callable);
    RUN_TEST(test_runtime_template_build_and_string_cast);
    RUN_TEST(test_runtime_member_load_returns_callable_builtin);
    RUN_TEST(test_runtime_start_process_cleans_hetero_arrays_and_nested_objects);
    RUN_TEST(test_runtime_checked_stackalloc_uses_tracked_scratch_storage);
    RUN_TEST(test_runtime_checked_registry_grows_past_legacy_pointer_limit);
    RUN_TEST(test_runtime_checked_registry_is_thread_safe_under_concurrency);
    RUN_TEST(test_runtime_start_process_boxes_cli_arguments);
    RUN_TEST(test_runtime_thread_and_mutex_helpers);
    RUN_TEST(test_runtime_deref_sized_and_store_sized_primitive_widths);
    /* alpha.2: future, atomic, cancel */
    RUN_TEST(test_runtime_thread_cancel_stops_thread);
    RUN_TEST(test_runtime_future_spawn_get_cancel);
    RUN_TEST(test_runtime_atomic_operations);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
