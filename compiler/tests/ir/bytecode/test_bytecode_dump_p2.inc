
int main(void) {
    printf("Running bytecode dump tests...\n\n");

    RUN_TEST(test_bytecode_dump_lowers_init_start_and_lambda_units);
    RUN_TEST(test_bytecode_dump_records_throw_literals);
    RUN_TEST(test_bytecode_dump_lowers_union_new_instructions);
    RUN_TEST(test_bytecode_dump_lowers_hetero_array_literals);
    RUN_TEST(test_bytecode_dump_lowers_hetero_array_index_reads);
    RUN_TEST(test_bytecode_dump_lowers_union_tag_and_payload_access);
    RUN_TEST(test_bytecode_dump_lowers_type_query_builtins);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
