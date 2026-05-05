    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse car/cdr HIR source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build car/cdr HIR symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check car/cdr HIR source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build car/cdr HIR");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "car/cdr HIR dump string is not NULL");
    ASSERT_CONTAINS("__calynda_rt_array_car", dump, "HIR lowers car to a direct helper");
    ASSERT_CONTAINS("__calynda_rt_array_cdr", dump, "HIR lowers cdr to a direct helper");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_hir_dump_lowers_string_car_and_cdr(void) {
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
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse string car/cdr HIR source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build string car/cdr HIR symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check string car/cdr HIR source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build string car/cdr HIR");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "string car/cdr HIR dump string is not NULL");
    ASSERT_CONTAINS("Index type=char", dump, "HIR lowers string car to an index expression");
    ASSERT_CONTAINS("Literal kind=0 type=int32 text=0",
                    dump,
                    "HIR lowers string car with a zero literal index");
    ASSERT_CONTAINS("__calynda_rt_string_cdr",
                    dump,
                    "HIR lowers string cdr to the runtime string tail helper");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_hir_dump_inlines_default_arguments(void) {
    static const char source[] =
        "int32 add = (int32 left, int32 right = left + 1) -> left + right;\n"
        "start -> add(4);\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse HIR default argument source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build HIR default argument symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check HIR default argument source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build HIR for default arguments");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "HIR default-argument dump is not NULL");
    ASSERT_CONTAINS("Arg 1:\n                Literal kind=0 type=int32 text=4",
                    dump,
                    "HIR preserves explicit argument");
    ASSERT_CONTAINS("Arg 2:\n                Binary op=+ type=int32",
                    dump,
                    "HIR synthesizes omitted default argument");
    ASSERT_CONTAINS("Left:\n                    Literal kind=0 type=int32 text=4",
                    dump,
                    "default argument inlines earlier explicit arguments at the call site");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_hir_dump_expands_swap_statement(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32[] values = [1, 2, 3];\n"
        "    values[0] >< values[2];\n"
        "    return values[0];\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse swap HIR source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build swap HIR symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check swap HIR source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build HIR for swap statement");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "swap HIR dump string is not NULL");
    ASSERT_CONTAINS("Block statements=5", dump, "swap expands into extra HIR statements");
    ASSERT_CONTAINS("Local name=__swap_tmp_0 type=int32", dump, "swap introduces temp local");
    ASSERT_CONTAINS("Assignment op=0 type=int32", dump, "swap lowers to assignment expressions");
    ASSERT_CONTAINS("Symbol name=__swap_tmp_0 kind=local type=int32", dump,
                    "swap reuses temp local in final assignment");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_hir_dump_lowers_overload_calls_to_selected_signature(void) {
    static const char source[] =
        "int32 pick = (int32 value) -> value;\n"
        "int32 pick = (int32 left, int32 right) -> left + right;\n"
        "start(string[] args) -> {\n"
        "    return pick(1, 2);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse overload HIR source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build overload HIR symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check overload HIR source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build overload HIR");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "overload HIR dump string is not NULL");
    ASSERT_CONTAINS("callable=(int32) -> int32", dump,
                    "HIR dump keeps unary overload signature");
    ASSERT_CONTAINS("Symbol name=pick kind=top-level binding type=int32 callable=(int32, int32) -> int32",
                    dump,
                    "HIR call uses the selected binary overload signature");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}
