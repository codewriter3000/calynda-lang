    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse required-after-default program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols required-after-default");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "required parameter after default is rejected");
    REQUIRE_TRUE(type_checker_format_error(type_checker_get_error(&checker),
                                           diagnostic,
                                           sizeof(diagnostic)),
                 "format required-after-default diagnostic");
    ASSERT_CONTAINS("Required parameter 'right' cannot follow",
                    diagnostic,
                    "required-after-default diagnostic mentions ordering");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/* V2: Java-style primitive aliases type check                         */
/* ------------------------------------------------------------------ */

void test_type_checker_handles_java_primitive_aliases(void) {
    const char *source =
        "int x = () -> 42;\n"
        "double pi = () -> 3.14;\n"
        "start(string[] args) -> {\n"
        "    var result = x() + int(pi());\n"
        "    return result;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse java alias program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols java alias");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "java primitive aliases pass type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}
