    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse type alias + threading");
    ASSERT_EQ_INT(AST_TOP_LEVEL_TYPE_ALIAS, program.top_level_decls[0]->kind,
                  "first decl is type alias");
    alias_decl = program.top_level_decls[0];
    ASSERT_TRUE(strcmp(alias_decl->as.type_alias_decl.name, "Grid") == 0,
                "type alias name is Grid");
    ASSERT_EQ_INT(AST_TYPE_PRIMITIVE, alias_decl->as.type_alias_decl.target_type.kind,
                  "alias target root is primitive array");
    ASSERT_EQ_INT(2, (int)alias_decl->as.type_alias_decl.target_type.dimension_count,
                  "alias target preserves array depth");

    thread_stmt = program.top_level_decls[1]->as.start_decl.body.as.block->statements[0];
    ASSERT_EQ_INT(AST_STMT_LOCAL_BINDING, thread_stmt->kind, "thread binding parses");
    ASSERT_EQ_INT(AST_TYPE_THREAD, thread_stmt->as.local_binding.declared_type.kind,
                  "Thread parses as built-in type");
    ASSERT_EQ_INT(AST_EXPR_SPAWN, thread_stmt->as.local_binding.initializer->kind,
                  "spawn parses as dedicated expression");

    ast_program_free(&program);
    parser_free(&parser);
}

void test_parse_swap_statement(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32[] values = [1, 2, 3];\n"
        "    values[0] >< values[2];\n"
        "    return values[1];\n"
        "};\n";
    Parser parser;
    AstProgram program;
    const AstStartDecl *start_decl;
    const AstStatement *swap_stmt;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse swap statement");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for swap statement");

    start_decl = &program.top_level_decls[0]->as.start_decl;
    REQUIRE_TRUE(start_decl->body.as.block->statement_count >= 2,
                 "swap statement block contains swap");
    swap_stmt = start_decl->body.as.block->statements[1];
    ASSERT_EQ_INT(AST_STMT_SWAP, swap_stmt->kind, "swap parses as AST_STMT_SWAP");
    ASSERT_EQ_INT(AST_EXPR_INDEX, swap_stmt->as.swap.left->kind,
                  "swap left side parses as index expression");
    ASSERT_EQ_INT(AST_EXPR_INDEX, swap_stmt->as.swap.right->kind,
                  "swap right side parses as index expression");

    ast_program_free(&program);
    parser_free(&parser);
}
