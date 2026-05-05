    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse typed ptr<int8> ops program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for typed ptr<int8> ops");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "typed ptr<int8> ops pass type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_accepts_string_index_as_char(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    string text = \"hi\";\n"
        "    char first = text[0];\n"
        "    return int32(first);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const AstBlock *body;
    const AstExpression *index_expression;
    const TypeCheckInfo *index_info;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse string index program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for string index program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "string index program passes type checking");

    body = program.top_level_decls[0]->as.start_decl.body.as.block;
    index_expression = body->statements[1]->as.local_binding.initializer;
    index_info = type_checker_get_expression_info(&checker, index_expression);
    REQUIRE_TRUE(index_info != NULL, "string index expression has type info");
    ASSERT_EQ_INT(CHECKED_TYPE_VALUE, index_info->type.kind, "string index type kind");
    ASSERT_EQ_INT(AST_PRIMITIVE_CHAR, index_info->type.primitive, "string index yields char");
    ASSERT_EQ_INT(0, (int)index_info->type.array_depth, "string index yields scalar char");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_accepts_string_and_array_length_members(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    string text = \"hello\";\n"
        "    int8[] bytes = [int8(1), int8(2), int8(3)];\n"
        "    int64 a = text.length;\n"
        "    int64 b = bytes.length;\n"
        "    return int32(a + b);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const AstBlock *body;
    const TypeCheckInfo *string_length_info;
    const TypeCheckInfo *array_length_info;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse length member program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for length member program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "length members pass type checking");

    body = program.top_level_decls[0]->as.start_decl.body.as.block;
    string_length_info = type_checker_get_expression_info(
        &checker, body->statements[2]->as.local_binding.initializer);
    array_length_info = type_checker_get_expression_info(
        &checker, body->statements[3]->as.local_binding.initializer);
    REQUIRE_TRUE(string_length_info != NULL, "string length has type info");
    REQUIRE_TRUE(array_length_info != NULL, "array length has type info");
    ASSERT_EQ_INT(AST_PRIMITIVE_INT64, string_length_info->type.primitive,
                  "string length yields int64");
    ASSERT_EQ_INT(AST_PRIMITIVE_INT64, array_length_info->type.primitive,
                  "array length yields int64");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_assignment_to_string_index_target(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    string text = \"hi\";\n"
        "    text[0] = 'H';\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse string index assignment program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for string index assignment");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "assignment to string index fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "string index assignment error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format string index assignment error");
    ASSERT_EQ_STR("3:5: Operator '=' requires an assignable target.",
                  diagnostic,
                  "string index assignment is rejected as non-assignable");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_assignment_to_length_member(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int8[] bytes = [int8(1), int8(2), int8(3)];\n"
        "    bytes.length = 1;\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse length assignment program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for length assignment");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "assignment to length member fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "length assignment error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format length assignment error");
    ASSERT_EQ_STR("3:5: Operator '=' requires an assignable target.",
                  diagnostic,
                  "length member is rejected as non-assignable");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}
