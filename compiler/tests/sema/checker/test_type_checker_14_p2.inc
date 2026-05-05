    type_checker_set_global_strict_race_check(false);
    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse race warning source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols race warning");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "non-strict race capture remains a warning");

    warning = type_checker_get_warning(&checker);
    REQUIRE_TRUE(warning != NULL, "race warning is recorded");
    REQUIRE_TRUE(type_checker_format_error(warning, diagnostic, sizeof(diagnostic)),
                 "format race warning");
    ASSERT_CONTAINS("Possible data race", diagnostic, "warning text mentions race");
    ASSERT_CONTAINS("4:", diagnostic, "warning keeps source span");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_spawn_mutable_capture_in_strict_mode(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32 shared = 1;\n"
        "    Thread worker = spawn () -> {\n"
        "        int32 copy = shared;\n"
        "        _ = copy;\n"
        "        exit;\n"
        "    };\n"
        "    worker.join();\n"
        "    return 0;\n"
        "};\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *error;

    type_checker_set_global_strict_race_check(true);
    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse strict race source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols strict race");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "strict race capture is rejected");
    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "strict race error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic)),
                 "format strict race error");
    ASSERT_CONTAINS("Possible data race", diagnostic, "strict error mentions race");
    ASSERT_CONTAINS("Related location", diagnostic, "strict error points at declaration");

    type_checker_set_global_strict_race_check(false);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_allows_spawn_atomic_capture(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    Atomic<int32> shared = Atomic.new(0);\n"
        "    Thread worker = spawn () -> {\n"
        "        _ = shared.exchange(1);\n"
        "        exit;\n"
        "    };\n"
        "    worker.join();\n"
        "    return shared.load();\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse atomic capture source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols atomic capture");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "atomic capture is accepted");
    ASSERT_TRUE(type_checker_get_warning(&checker) == NULL,
                "atomic capture produces no race warning");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_allows_spawn_thread_local_capture(void) {
    static const char source[] =
        "thread_local int32 shared = 1;\n"
        "start(string[] args) -> {\n"
        "    Thread worker = spawn () -> {\n"
        "        int32 copy = shared;\n"
        "        _ = copy;\n"
        "        exit;\n"
        "    };\n"
        "    worker.join();\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse thread_local capture source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols thread_local capture");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "thread_local capture is accepted");
    ASSERT_TRUE(type_checker_get_warning(&checker) == NULL,
                "thread_local capture produces no warning");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_accepts_array_car_cdr_builtins(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32[] values = [1, 2, 3];\n"
        "    int32 head = car(values);\n"
        "    int32[] tail = cdr(values);\n"
        "    return head + int32(tail.length);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse car/cdr source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols car/cdr");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "car/cdr builtins type check on homogeneous arrays");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_accepts_string_car_cdr_builtins(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    string text = \"hello\";\n"
        "    char head = car(text);\n"
        "    string tail = cdr(text);\n"
        "    return int32(head) + int32(tail.length);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse string car/cdr source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols string car/cdr");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "car/cdr builtins type check on strings");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_car_on_non_sequence(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    return int32(car(1));\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse bad car source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols bad car");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "car rejects non-sequence arguments");
    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "car type error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic)),
                 "format car type error");
    ASSERT_CONTAINS("car expects a homogeneous array (T[]) or string argument but got int32.",
                    diagnostic,
                    "car error mentions sequence contract");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_accepts_type_query_builtins(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32[] values = [1, 2, 3];\n"
        "    arr<?> mixed = [1, true, \"hello\"];\n"
        "    string homogeneousType = typeof(values);\n"
        "    string heteroType = typeof(mixed);\n"
        "    bool ok = isint(1) && isfloat(float64(1)) && isbool(true) &&\n"
        "              isstring(homogeneousType) && isarray(values) && isarray(mixed) &&\n"
        "              issametype(values, cdr(values)) && !issametype(values, mixed);\n"
        "    return ok ? int32(homogeneousType.length + heteroType.length) : 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse typeof/is* source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols typeof/is*");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "typeof/is* builtins type check");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

#include "test_type_checker_14_p3.inc"
