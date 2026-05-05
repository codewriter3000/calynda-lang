    RUN_TEST(test_build_native_runs_simple_start_program); RUN_TEST(test_build_native_handles_direct_eight_argument_calls);
    RUN_TEST(test_build_native_runs_runtime_lambda_program); RUN_TEST(test_build_native_auto_calls_zero_arg_template_callable);
    RUN_TEST(test_build_native_runs_boot_program);
    RUN_TEST(test_build_native_runs_boot_program_with_block);
    RUN_TEST(test_build_native_boot_supports_arrays_strings_sequences);
    RUN_TEST(test_build_native_runs_void_start_program);
    RUN_TEST(test_build_native_runs_manual_malloc_free);
    RUN_TEST(test_build_native_typed_ptr_offset_stride);
    RUN_TEST(test_build_native_supports_string_index_and_length);
    RUN_TEST(test_build_native_supports_string_car_and_cdr);
    RUN_TEST(test_build_native_stackalloc_store_deref);
    RUN_TEST(test_build_native_stackalloc_typed_ptr);
    RUN_TEST(test_build_native_formats_hetero_arrays); RUN_TEST(test_build_native_indexes_hetero_arrays);
    RUN_TEST(test_build_native_compares_hetero_arrays_by_identity);
    RUN_TEST(test_build_native_dispatches_union_tag_and_payload);
    RUN_TEST(test_build_native_formats_union_variants);
    RUN_TEST(test_build_native_layout_field_access);
    RUN_TEST(test_build_native_double_capture_closure); RUN_TEST(test_build_native_primitive_types);
    RUN_TEST(test_build_native_varargs_passthrough); RUN_TEST(test_build_native_template_literal);
    RUN_TEST(test_build_native_threading_helpers);
    RUN_TEST(test_build_native_type_query_builtins);
    RUN_TEST(test_build_native_manual_checked_abort);
    RUN_TEST(test_build_native_car_multifile);
    RUN_TEST(test_build_native_malformed_source_reports_span);
    RUN_TEST(test_build_native_runs_recursive_top_level_lambda_program);
    RUN_TEST(test_build_native_swaps_array_elements);
    RUN_TEST(test_build_native_runs_default_argument_program);
    RUN_TEST(test_build_native_runs_deep_self_tail_recursion_without_stack_growth);
    RUN_TEST(test_build_native_dispatches_overloads_with_widening);
    RUN_TEST(test_build_native_runs_untyped_parameter_program);
    RUN_TEST(test_build_native_calls_untyped_callable_parameter);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
