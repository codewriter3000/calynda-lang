void test_symbol_table_allows_future_atomic_builtin_identifiers(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    Future<int32> futureValue = Future.new();\n"
        "    Atomic<int32> atomicValue = Atomic.new();\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse Future/Atomic builtin identifiers");

    symbol_table_init(&table);
    REQUIRE_TRUE(symbol_table_build(&table, &program), "build symbols Future/Atomic builtin identifiers");
    ASSERT_EQ_INT(0, (int)table.unresolved_count,
                  "Future/Atomic builtin identifiers do not become unresolved");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_symbol_table_allows_car_cdr_builtin_identifiers(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    int32[] values = [1, 2, 3];\n"
        "    int32 head = car(values);\n"
        "    int32[] tail = cdr(values);\n"
        "    return head + int32(tail.length);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse car/cdr builtin identifiers");

    symbol_table_init(&table);
    REQUIRE_TRUE(symbol_table_build(&table, &program), "build symbols car/cdr builtin identifiers");
    ASSERT_EQ_INT(0, (int)table.unresolved_count,
                  "car/cdr builtin identifiers do not become unresolved");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_symbol_table_allows_type_query_builtin_identifiers(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    int32[] values = [1, 2, 3];\n"
        "    arr<?> mixed = [1, true];\n"
        "    string typeName = typeof(values);\n"
        "    bool ok = isint(1) && isfloat(float64(1)) && isbool(true) &&\n"
        "              isstring(typeName) && isarray(values) && isarray(mixed) &&\n"
        "              issametype(values, cdr(values));\n"
        "    return ok ? 0 : 1;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse typeof/is* builtin identifiers");

    symbol_table_init(&table);
    REQUIRE_TRUE(symbol_table_build(&table, &program), "build symbols typeof/is* builtin identifiers");
    ASSERT_EQ_INT(0, (int)table.unresolved_count,
                  "typeof/is* builtin identifiers do not become unresolved");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}
