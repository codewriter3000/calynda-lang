
void test_parse_thread_local_local_binding_error(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    thread_local int32 counter = 0;\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    const ParserError *error;

    parser_init(&parser, source);
    ASSERT_TRUE(!parser_parse_program(&parser, &program),
                "thread_local local binding is rejected");
    error = parser_get_error(&parser);
    REQUIRE_TRUE(error != NULL, "thread_local local binding reports error");
    ASSERT_CONTAINS("thread_local", error->message,
                    "thread_local local binding error mentions thread_local");
    parser_free(&parser);
}

void test_parse_thread_local_static_redundancy_error(void) {
    const char *source = "thread_local static int32 counter = 0;\n";
    Parser parser;
    AstProgram program;
    const ParserError *error;

    parser_init(&parser, source);
    ASSERT_TRUE(!parser_parse_program(&parser, &program),
                "thread_local static binding is rejected");
    error = parser_get_error(&parser);
    REQUIRE_TRUE(error != NULL, "thread_local static binding reports error");
    ASSERT_CONTAINS("redundant", error->message,
                    "thread_local static error mentions redundancy");
    parser_free(&parser);
}

void test_parse_uninitialized_bindings(void) {
    const char *source =
        "int32 counter;\n"
        "start(string[] args) -> {\n"
        "    string message;\n"
        "    return counter;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    const AstBindingDecl *binding;
    const AstStatement *statement;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse omitted binding initializers");
    ASSERT_TRUE(parser_get_error(&parser) == NULL,
                "no parse error for omitted binding initializers");

    binding = &program.top_level_decls[0]->as.binding_decl;
    ASSERT_TRUE(binding->initializer == NULL,
                "top-level omitted initializer stays absent in AST");

    statement = program.top_level_decls[1]
                    ->as.start_decl.body.as.block->statements[0];
    ASSERT_EQ_INT(AST_STMT_LOCAL_BINDING, statement->kind,
                  "omitted local initializer parses as local binding");
    ASSERT_TRUE(statement->as.local_binding.initializer == NULL,
                "local omitted initializer stays absent in AST");

    ast_program_free(&program);
    parser_free(&parser);
}
