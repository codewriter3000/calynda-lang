    decl = program.top_level_decls[0];
    ASSERT_EQ_INT(AST_TOP_LEVEL_START, (int)decl->kind, "boot decl uses AST_TOP_LEVEL_START");
    ASSERT_TRUE(decl->as.start_decl.is_boot, "is_boot flag is set");
    ASSERT_EQ_INT(0, (int)decl->as.start_decl.parameters.count, "boot has zero parameters");
    ASSERT_EQ_INT(AST_LAMBDA_BODY_BLOCK, (int)decl->as.start_decl.body.kind, "boot body is block");

    ast_program_free(&program);
    parser_free(&parser);
}


void test_parse_boot_decl_expression_body(void) {
    const char *source = "boot -> 42;\n";
    Parser parser;
    AstProgram program;
    AstTopLevelDecl *decl;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse boot expression body");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error");

    decl = program.top_level_decls[0];
    ASSERT_EQ_INT(AST_TOP_LEVEL_START, (int)decl->kind, "boot decl kind");
    ASSERT_TRUE(decl->as.start_decl.is_boot, "is_boot set for expression body");
    ASSERT_EQ_INT(AST_LAMBDA_BODY_EXPRESSION, (int)decl->as.start_decl.body.kind,
                  "boot body is expression");

    ast_program_free(&program);
    parser_free(&parser);
}
