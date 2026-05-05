    car_archive_init(&archive);
    parser_init(&parser, main_source);
    symbol_table_init(&table);
    REQUIRE_TRUE(add_archive_source(&archive, "lib/math.cal", lib_source),
                 "build wildcard dependency archive");
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse wildcard dependency archive consumer");
    REQUIRE_TRUE(symbol_table_build_with_archive_deps(&table, &program,
                                                      &archive, 1),
                 "build symbols with wildcard dependency archive");

    symbol = symbol_table_find_import(&table, "squarePower");
    REQUIRE_TRUE(symbol != NULL, "wildcard import injects squarePower");
    ASSERT_EQ_INT(SYMBOL_KIND_ASM_BINDING, symbol->kind,
                  "wildcard import preserves callable symbol kind");
    ASSERT_EQ_STR("lib.math.squarePower", symbol->qualified_name,
                  "wildcard import preserves qualified name");

    car_archive_free(&archive);
    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_symbol_table_stores_top_level_overload_sets(void) {
    static const char source[] =
        "int32 pick = (int32 value) -> value;\n"
        "int32 pick = (string value) -> 0;\n"
        "start(string[] args) -> {\n"
        "    return pick(1);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;
    const Scope *root;
    const OverloadSet *overload_set;
    const AstExpression *call_expr;
    const SymbolResolution *resolution;

    parser_init(&parser, source);
    symbol_table_init(&table);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse overload-set source");
    REQUIRE_TRUE(symbol_table_build(&table, &program), "build overload-set symbols");

    root = symbol_table_root_scope(&table);
    REQUIRE_TRUE(root != NULL, "root scope exists for overload-set source");
    overload_set = scope_lookup_local_overload_set(root, "pick");
    REQUIRE_TRUE(overload_set != NULL, "top-level overload set recorded");
    ASSERT_EQ_INT(2, (int)overload_set->symbol_count, "overload set stores both callables");

    call_expr = program.top_level_decls[2]
                    ->as.start_decl.body.as.block->statements[0]
                    ->as.return_expression;
    REQUIRE_TRUE(call_expr != NULL && call_expr->kind == AST_EXPR_CALL,
                 "return expression is call");
    resolution = symbol_table_find_resolution(&table, call_expr->as.call.callee);
    REQUIRE_TRUE(resolution != NULL, "call callee resolution exists");
    ASSERT_TRUE(resolution->symbol == NULL,
                "callee keeps unresolved symbol until overload selection");
    ASSERT_TRUE(resolution->overload_set == overload_set,
                "callee resolution points at overload set");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}
