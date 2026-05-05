
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
