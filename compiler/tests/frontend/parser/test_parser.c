#include "parser.h"

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

void test_parse_program_structure(void);
void test_parse_expression_precedence(void);
void test_parse_assignment_and_ternary(void);
void test_parse_template_literal(void);
void test_parse_errors(void);
void test_parse_prefix_increment_decrement(void);
void test_parse_postfix_increment_decrement(void);
void test_parse_discard_expression(void);
void test_parse_varargs_parameter(void);
void test_parse_v2_modifiers(void);
void test_parse_thread_local_binding_modifier(void);
void test_parse_import_alias(void);
void test_parse_import_wildcard(void);
void test_parse_import_selective(void);
void test_parse_java_primitive_aliases(void);
void test_parse_union_declaration(void);
void test_parse_union_with_modifiers(void);
void test_parse_internal_local_binding(void);
void test_parse_named_type_in_binding(void);
void test_parse_arr_wildcard_type(void);
void test_parse_future_and_atomic_types(void);
void test_parse_thread_local_local_binding_error(void);
void test_parse_thread_local_static_redundancy_error(void);
void test_parse_nested_generic_rshift(void);
void test_parse_named_type_in_parameter(void);
void test_parse_asm_decl(void);
void test_parse_asm_decl_with_modifiers(void);
void test_parse_boot_decl(void);
void test_parse_boot_decl_expression_body(void);
void test_parse_manual_statement_with_memory_ops(void);
void test_parse_calloc_memory_op(void);
void test_parse_ptr_type_in_manual(void);
void test_parse_deref_store_offset_addr_ops(void);
void test_parse_stackalloc_op(void);
void test_parse_layout_declaration(void);
void test_parse_malformed_unclosed_brace(void);
void test_parse_malformed_missing_semicolon(void);
void test_parse_malformed_stray_token(void);
void test_parse_manual_checked_block(void);
void test_parse_manual_lambda_shorthand(void);
void test_parse_union_multi_generic(void);
void test_parse_type_alias_and_spawn_threading(void);


int main(void) {
    printf("Running parser tests...\n\n");

    RUN_TEST(test_parse_program_structure);
    RUN_TEST(test_parse_expression_precedence);
    RUN_TEST(test_parse_assignment_and_ternary);
    RUN_TEST(test_parse_template_literal);
    RUN_TEST(test_parse_errors);
    RUN_TEST(test_parse_prefix_increment_decrement);
    RUN_TEST(test_parse_postfix_increment_decrement);
    RUN_TEST(test_parse_discard_expression);
    RUN_TEST(test_parse_varargs_parameter);
    RUN_TEST(test_parse_v2_modifiers);
    RUN_TEST(test_parse_thread_local_binding_modifier);
    RUN_TEST(test_parse_import_alias);
    RUN_TEST(test_parse_import_wildcard);
    RUN_TEST(test_parse_import_selective);
    RUN_TEST(test_parse_java_primitive_aliases);
    RUN_TEST(test_parse_union_declaration);
    RUN_TEST(test_parse_union_with_modifiers);
    RUN_TEST(test_parse_internal_local_binding);
    RUN_TEST(test_parse_named_type_in_binding);
    RUN_TEST(test_parse_arr_wildcard_type);
    RUN_TEST(test_parse_future_and_atomic_types);
    RUN_TEST(test_parse_thread_local_local_binding_error);
    RUN_TEST(test_parse_thread_local_static_redundancy_error);
    RUN_TEST(test_parse_nested_generic_rshift);
    RUN_TEST(test_parse_named_type_in_parameter);
    RUN_TEST(test_parse_asm_decl);
    RUN_TEST(test_parse_asm_decl_with_modifiers);
    RUN_TEST(test_parse_boot_decl);
    RUN_TEST(test_parse_boot_decl_expression_body);
    RUN_TEST(test_parse_manual_statement_with_memory_ops);
    RUN_TEST(test_parse_calloc_memory_op);
    RUN_TEST(test_parse_ptr_type_in_manual);
    RUN_TEST(test_parse_deref_store_offset_addr_ops);
    RUN_TEST(test_parse_stackalloc_op);
    RUN_TEST(test_parse_layout_declaration);
    RUN_TEST(test_parse_malformed_unclosed_brace);
    RUN_TEST(test_parse_malformed_missing_semicolon);
    RUN_TEST(test_parse_malformed_stray_token);
    RUN_TEST(test_parse_manual_checked_block);
    RUN_TEST(test_parse_manual_lambda_shorthand);
    RUN_TEST(test_parse_union_multi_generic);
    RUN_TEST(test_parse_type_alias_and_spawn_threading);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
