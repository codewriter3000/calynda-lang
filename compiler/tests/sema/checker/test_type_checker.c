#include "parser.h"
#include "symbol_table.h"
#include "type_checker.h"

#include <stdio.h>
#include <string.h>

int tests_run = 0;
int tests_passed = 0;
int tests_failed = 0;

#define ASSERT_EQ_INT(expected, actual, msg) do {                           \
    tests_run++;                                                            \
    if ((expected) == (actual)) {                                           \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected %d, got %d\n",      \
                __FILE__, __LINE__, (msg), (int)(expected), (int)(actual)); \
    }                                                                       \
} while (0)
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
#define REQUIRE_TRUE(condition, msg) do {                                   \
    tests_run++;                                                            \
    if (condition) {                                                        \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s\n",                            \
                __FILE__, __LINE__, (msg));                                 \
        return;                                                             \
    }                                                                       \
} while (0)
#define ASSERT_EQ_STR(expected, actual, msg) do {                           \
    tests_run++;                                                            \
    if ((actual) != NULL && strcmp((expected), (actual)) == 0) {            \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected \"%s\", got \"%s\"\n", \
                __FILE__, __LINE__, (msg), (expected),                      \
                (actual) ? (actual) : "(null)");                           \
    }                                                                       \
} while (0)
#define ASSERT_CONTAINS(needle, haystack, msg) do {                         \
    tests_run++;                                                            \
    if ((haystack) != NULL && strstr((haystack), (needle)) != NULL) {       \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: missing \"%s\" in \"%s\"\n", \
                __FILE__, __LINE__, (msg), (needle),                        \
                (haystack) ? (haystack) : "(null)");                       \
    }                                                                       \
} while (0)
#define RUN_TEST(fn) do {                                                   \
    printf("  %s ...\n", #fn);                                            \
    fn();                                                                   \
} while (0)

void test_type_checker_infers_symbols_and_expressions(void);
void test_type_checker_rejects_binding_type_mismatch(void);
void test_type_checker_rejects_bad_call_arguments(void);
void test_type_checker_rejects_non_bool_ternary_condition(void);
void test_type_checker_rejects_void_lambda_body_for_typed_binding(void);
void test_type_checker_rejects_bad_start_return_type(void);
void test_type_checker_rejects_void_zero_arg_callable_in_template(void);
void test_type_checker_reports_type_resolution_errors(void);
void test_type_checker_accepts_bare_start_entry_point(void);
void test_type_checker_requires_start_entry_point(void);
void test_type_checker_rejects_duplicate_start_entry_points(void);
void test_type_checker_rejects_invalid_start_parameter_type(void);
void test_type_checker_accepts_boot_entry_point(void);
void test_type_checker_rejects_boot_and_start_together(void);
void test_type_checker_rejects_duplicate_boot_entry_points(void);
void test_type_checker_allows_exit_in_void_lambda_block(void);
void test_type_checker_rejects_bare_return_in_non_void_lambda(void);
void test_type_checker_allows_exit_in_start_block(void);
void test_type_checker_allows_bare_return_in_start_block(void);
void test_type_checker_allows_assignment_to_local_array_element(void);
void test_type_checker_rejects_assignment_to_temporary_index_target(void);
void test_type_checker_rejects_assignment_to_import_symbol(void);
void test_type_checker_rejects_assignment_to_final_index_target(void);
void test_type_checker_rejects_incompatible_compound_assignment(void);
void test_type_checker_accepts_swap_between_array_elements(void);
void test_type_checker_rejects_swap_with_mismatched_types(void);
void test_type_checker_rejects_postfix_increment_on_non_numeric(void);
void test_type_checker_allows_discard_assignment(void);
void test_type_checker_allows_varargs_call(void);
void test_type_checker_rejects_varargs_type_mismatch(void);
void test_type_checker_allows_trailing_default_arguments(void);
void test_type_checker_rejects_required_parameter_after_default(void);
void test_type_checker_handles_java_primitive_aliases(void);
void test_type_checker_rejects_export_private_conflict(void);
void test_type_checker_allows_export_binding(void);
void test_type_checker_allows_static_binding(void);
void test_type_checker_rejects_internal_same_scope(void);
void test_type_checker_allows_internal_from_nested_lambda(void);
void test_type_checker_resolves_union_symbol(void);
void test_type_checker_resolves_named_type_local(void);
void test_type_checker_accepts_non_payload_variant(void);
void test_type_checker_accepts_payload_variant_constructor(void);
void test_type_checker_rejects_unknown_variant(void);
void test_type_checker_accepts_hetero_array(void);
void test_type_checker_accepts_hetero_array_index_reads(void);
void test_type_checker_rejects_hetero_array_index_writes(void);
void test_type_checker_rejects_hetero_array_external_equality_without_cast(void);
void test_type_checker_accepts_hetero_array_equality_after_explicit_casts(void);
void test_type_checker_rejects_arr_wildcard_from_multi_dimensional_array(void);
void test_type_checker_accepts_union_tag_and_payload_access(void);
void test_type_checker_rejects_unknown_union_value_member(void);
void test_type_checker_rejects_union_tag_assignment(void);
void test_type_checker_accepts_manual_block_with_memory_ops(void);
void test_type_checker_accepts_manual_lambda_shorthand(void);
void test_type_checker_accepts_calloc_memory_op(void);
void test_type_checker_accepts_manual_pointer_ops(void);
void test_type_checker_accepts_typed_ptr_deref_offset_store(void);
void test_type_checker_accepts_typed_ptr_int8_ops(void);
void test_type_checker_accepts_string_index_as_char(void);
void test_type_checker_accepts_string_and_array_length_members(void);
void test_type_checker_rejects_assignment_to_string_index_target(void);
void test_type_checker_rejects_assignment_to_length_member(void);
void test_type_checker_accepts_stackalloc_op(void);
void test_type_checker_rejects_stackalloc_non_integral_arg(void);
void test_type_checker_accepts_layout_declaration(void);
void test_type_checker_rejects_non_primitive_layout_field(void);
void test_type_checker_error_has_source_span(void);
void test_type_checker_accepts_pre_increment(void);
void test_type_checker_accepts_threading_builtins_and_type_alias(void);
void test_type_checker_accepts_spawn_non_void_callable_as_future(void);
void test_type_checker_accepts_future_and_atomic_builtins(void);
void test_type_checker_warns_on_spawn_mutable_capture(void);
void test_type_checker_rejects_spawn_mutable_capture_in_strict_mode(void);
void test_type_checker_allows_spawn_atomic_capture(void);
void test_type_checker_allows_spawn_thread_local_capture(void);
void test_type_checker_accepts_array_car_cdr_builtins(void);
void test_type_checker_rejects_car_on_non_array(void);
void test_type_checker_accepts_type_query_builtins(void);
void test_type_checker_rejects_bad_type_query_arity(void);
void test_type_checker_accepts_recursive_top_level_lambda_binding(void);
void test_type_checker_accepts_recursive_top_level_manual_lambda_binding(void);
void test_type_checker_rejects_recursive_non_lambda_top_level_binding(void);
void test_type_checker_accepts_wildcard_dep_archive_import(void);
void test_type_checker_resolves_overloads_by_exact_and_widening_match(void);
void test_type_checker_rejects_ambiguous_overload_call(void);
void test_type_checker_resolves_overloads_by_arity(void);


int main(void) {
    printf("Running type checker tests...\n\n");

    RUN_TEST(test_type_checker_infers_symbols_and_expressions);
    RUN_TEST(test_type_checker_rejects_binding_type_mismatch);
    RUN_TEST(test_type_checker_rejects_bad_call_arguments);
    RUN_TEST(test_type_checker_rejects_non_bool_ternary_condition);
    RUN_TEST(test_type_checker_rejects_void_lambda_body_for_typed_binding);
    RUN_TEST(test_type_checker_rejects_bad_start_return_type);
    RUN_TEST(test_type_checker_rejects_void_zero_arg_callable_in_template);
    RUN_TEST(test_type_checker_reports_type_resolution_errors);
    RUN_TEST(test_type_checker_accepts_bare_start_entry_point);
    RUN_TEST(test_type_checker_requires_start_entry_point);
    RUN_TEST(test_type_checker_rejects_duplicate_start_entry_points);
    RUN_TEST(test_type_checker_rejects_invalid_start_parameter_type);
    RUN_TEST(test_type_checker_accepts_boot_entry_point);
    RUN_TEST(test_type_checker_rejects_boot_and_start_together);
    RUN_TEST(test_type_checker_rejects_duplicate_boot_entry_points);
    RUN_TEST(test_type_checker_allows_exit_in_void_lambda_block);
    RUN_TEST(test_type_checker_rejects_bare_return_in_non_void_lambda);
    RUN_TEST(test_type_checker_allows_exit_in_start_block);
    RUN_TEST(test_type_checker_allows_bare_return_in_start_block);
    RUN_TEST(test_type_checker_allows_assignment_to_local_array_element);
    RUN_TEST(test_type_checker_rejects_assignment_to_temporary_index_target);
    RUN_TEST(test_type_checker_rejects_assignment_to_import_symbol);
    RUN_TEST(test_type_checker_rejects_assignment_to_final_index_target);
    RUN_TEST(test_type_checker_rejects_incompatible_compound_assignment);
    RUN_TEST(test_type_checker_accepts_swap_between_array_elements);
    RUN_TEST(test_type_checker_rejects_swap_with_mismatched_types);
    RUN_TEST(test_type_checker_rejects_postfix_increment_on_non_numeric);
    RUN_TEST(test_type_checker_allows_discard_assignment);
    RUN_TEST(test_type_checker_allows_varargs_call);
    RUN_TEST(test_type_checker_rejects_varargs_type_mismatch);
    RUN_TEST(test_type_checker_allows_trailing_default_arguments);
    RUN_TEST(test_type_checker_rejects_required_parameter_after_default);
    RUN_TEST(test_type_checker_handles_java_primitive_aliases);
    RUN_TEST(test_type_checker_rejects_export_private_conflict);
    RUN_TEST(test_type_checker_allows_export_binding);
    RUN_TEST(test_type_checker_allows_static_binding);
    RUN_TEST(test_type_checker_rejects_internal_same_scope);
    RUN_TEST(test_type_checker_allows_internal_from_nested_lambda);
    RUN_TEST(test_type_checker_resolves_union_symbol);
    RUN_TEST(test_type_checker_resolves_named_type_local);
    RUN_TEST(test_type_checker_accepts_non_payload_variant);
    RUN_TEST(test_type_checker_accepts_payload_variant_constructor);
    RUN_TEST(test_type_checker_rejects_unknown_variant);
    RUN_TEST(test_type_checker_accepts_hetero_array);
    RUN_TEST(test_type_checker_accepts_hetero_array_index_reads);
    RUN_TEST(test_type_checker_rejects_hetero_array_index_writes);
    RUN_TEST(test_type_checker_rejects_hetero_array_external_equality_without_cast);
    RUN_TEST(test_type_checker_accepts_hetero_array_equality_after_explicit_casts);
    RUN_TEST(test_type_checker_rejects_arr_wildcard_from_multi_dimensional_array);
    RUN_TEST(test_type_checker_accepts_union_tag_and_payload_access);
    RUN_TEST(test_type_checker_rejects_unknown_union_value_member);
    RUN_TEST(test_type_checker_rejects_union_tag_assignment);
    RUN_TEST(test_type_checker_accepts_manual_block_with_memory_ops);
    RUN_TEST(test_type_checker_accepts_manual_lambda_shorthand);
    RUN_TEST(test_type_checker_accepts_calloc_memory_op);
    RUN_TEST(test_type_checker_accepts_manual_pointer_ops);
    RUN_TEST(test_type_checker_accepts_typed_ptr_deref_offset_store);
    RUN_TEST(test_type_checker_accepts_typed_ptr_int8_ops);
    RUN_TEST(test_type_checker_accepts_string_index_as_char);
    RUN_TEST(test_type_checker_accepts_string_and_array_length_members);
    RUN_TEST(test_type_checker_rejects_assignment_to_string_index_target);
    RUN_TEST(test_type_checker_rejects_assignment_to_length_member);
    RUN_TEST(test_type_checker_accepts_stackalloc_op);
    RUN_TEST(test_type_checker_rejects_stackalloc_non_integral_arg);
    RUN_TEST(test_type_checker_accepts_layout_declaration);
    RUN_TEST(test_type_checker_rejects_non_primitive_layout_field);
    RUN_TEST(test_type_checker_error_has_source_span);
    RUN_TEST(test_type_checker_accepts_pre_increment);
    RUN_TEST(test_type_checker_accepts_threading_builtins_and_type_alias);
    RUN_TEST(test_type_checker_accepts_spawn_non_void_callable_as_future);
    RUN_TEST(test_type_checker_accepts_future_and_atomic_builtins);
    RUN_TEST(test_type_checker_warns_on_spawn_mutable_capture);
    RUN_TEST(test_type_checker_rejects_spawn_mutable_capture_in_strict_mode);
    RUN_TEST(test_type_checker_allows_spawn_atomic_capture);
    RUN_TEST(test_type_checker_allows_spawn_thread_local_capture);
    RUN_TEST(test_type_checker_accepts_array_car_cdr_builtins);
    RUN_TEST(test_type_checker_rejects_car_on_non_array);
    RUN_TEST(test_type_checker_accepts_type_query_builtins);
    RUN_TEST(test_type_checker_rejects_bad_type_query_arity);
    RUN_TEST(test_type_checker_accepts_recursive_top_level_lambda_binding);
    RUN_TEST(test_type_checker_accepts_recursive_top_level_manual_lambda_binding);
    RUN_TEST(test_type_checker_rejects_recursive_non_lambda_top_level_binding);
    RUN_TEST(test_type_checker_accepts_wildcard_dep_archive_import);
    RUN_TEST(test_type_checker_resolves_overloads_by_exact_and_widening_match);
    RUN_TEST(test_type_checker_rejects_ambiguous_overload_call);
    RUN_TEST(test_type_checker_resolves_overloads_by_arity);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
