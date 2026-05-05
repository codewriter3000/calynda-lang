void test_type_checker_rejects_swap_with_mismatched_types(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    int32 left = 1;\n"
        "    string right = \"hi\";\n"
        "    left >< right;\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *error;
    char diagnostic_buffer[256];
    char *diagnostic = diagnostic_buffer;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse mismatched swap program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for mismatched swap");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "mismatched swap fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "mismatched swap error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format mismatched swap error");
    ASSERT_EQ_STR("4:13: Swap statement requires matching target types but got int32 and string. Related location at 4:5.",
                  diagnostic,
                  "formatted mismatched swap diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}
