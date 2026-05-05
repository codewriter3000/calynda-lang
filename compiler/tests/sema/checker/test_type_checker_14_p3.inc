void test_type_checker_rejects_bad_type_query_arity(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    return typeof(1, 2).length;\n"
        "};\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *error;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse bad typeof arity");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols bad typeof arity");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "typeof rejects extra arguments");
    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "typeof arity error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic)),
                 "format typeof arity error");
    ASSERT_CONTAINS("typeof expects exactly 1 argument but got 2.",
                    diagnostic,
                    "typeof arity error is specific");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}
