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

void test_type_checker_rejects_fence_with_arguments(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    fence(1);\n"
        "    return 0;\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse bad fence arity");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols bad fence arity");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "fence rejects extra arguments");
    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "fence arity error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic)),
                 "format fence arity error");
    ASSERT_CONTAINS("fence expects exactly 0 arguments but got 1.",
                    diagnostic,
                    "fence arity error is specific");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_accepts_cache_builtins(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    mmio<uint32> reg = 0;\n"
        "    cacheclean(reg);\n"
        "    cacheclean(4096);\n"
        "    cachefinal();\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse cache builtin source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols cache builtin source");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "cache builtins accept integral and mmio addresses");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_cacheclean_non_address_argument(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    cacheclean(\"hello\");\n"
        "    return 0;\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse bad cacheclean source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols bad cacheclean source");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "cacheclean rejects non-address arguments");
    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "cacheclean type error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic)),
                 "format cacheclean type error");
    ASSERT_CONTAINS("cacheclean expects an integral, ptr<T>, or mmio<T> address but got string.",
                    diagnostic,
                    "cacheclean error mentions address-like contract");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}
