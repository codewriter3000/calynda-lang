void test_type_checker_rejects_ambiguous_overload_call(void) {
    static const char source[] =
        "int32 pick = (int32 value) -> 1;\n"
        "int32 pick = (int64 value) -> 2;\n"
        "start(string[] args) -> {\n"
        "    return pick(int16(7));\n"
        "};\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse ambiguous overload program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for ambiguous overload program");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "ambiguous overload call is rejected");
    REQUIRE_TRUE(type_checker_format_error(type_checker_get_error(&checker),
                                           diagnostic,
                                           sizeof(diagnostic)),
                 "format ambiguous overload diagnostic");
    ASSERT_CONTAINS("ambiguous", diagnostic,
                    "ambiguous overload diagnostic mentions ambiguity");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_resolves_overloads_by_arity(void) {
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
    const Symbol *binary_pick_symbol;
    const AstExpression *call_expr;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse overload arity program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for overload arity program");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "overload arity program passes type checking");

    binary_pick_symbol = symbol_table_find_symbol_for_declaration(
        &symbols,
        &program.top_level_decls[1]->as.binding_decl);
    REQUIRE_TRUE(binary_pick_symbol != NULL, "binary overload symbol found");
    call_expr = program.top_level_decls[2]
                    ->as.start_decl.body.as.block->statements[0]
                    ->as.return_expression;
    REQUIRE_TRUE(call_expr && call_expr->kind == AST_EXPR_CALL,
                 "arity-selected return expression is call");
    ASSERT_TRUE(symbol_table_resolve_identifier(&symbols, call_expr->as.call.callee) ==
                    binary_pick_symbol,
                "two-argument call selects two-parameter overload");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_accepts_typed_uninitialized_bindings(void) {
    static const char source[] =
        "int32 counter;\n"
        "start(string[] args) -> {\n"
        "    bool ready;\n"
        "    return ready ? counter : counter;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse typed uninitialized bindings");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for typed uninitialized bindings");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "typed uninitialized bindings pass type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_inferred_uninitialized_binding(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    var pending;\n"
        "    return 0;\n"
        "};\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse inferred uninitialized binding");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for inferred uninitialized binding");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "inferred uninitialized binding is rejected");
    REQUIRE_TRUE(type_checker_format_error(type_checker_get_error(&checker),
                                           diagnostic,
                                           sizeof(diagnostic)),
                 "format inferred uninitialized binding diagnostic");
    ASSERT_CONTAINS("requires an initializer", diagnostic,
                    "inferred uninitialized binding reports missing initializer");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_final_uninitialized_binding(void) {
    static const char source[] =
        "final int32 counter;\n"
        "start(string[] args) -> counter;\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse final uninitialized binding");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for final uninitialized binding");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "final uninitialized binding is rejected");
    REQUIRE_TRUE(type_checker_format_error(type_checker_get_error(&checker),
                                           diagnostic,
                                           sizeof(diagnostic)),
                 "format final uninitialized binding diagnostic");
    ASSERT_CONTAINS("requires an initializer", diagnostic,
                    "final uninitialized binding reports missing initializer");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}
