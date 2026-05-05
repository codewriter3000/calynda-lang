    free(dump);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

int main(void) {
    printf("Running semantic dump tests...\n");

    RUN_TEST(test_semantic_dump_program_to_string);
    RUN_TEST(test_semantic_dump_program_to_file_with_unresolved_identifier);
    RUN_TEST(test_semantic_dump_shows_type_alias_symbol);

    printf("\nSemantic dump tests run: %d\n", tests_run);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return (tests_failed == 0) ? 0 : 1;
}
