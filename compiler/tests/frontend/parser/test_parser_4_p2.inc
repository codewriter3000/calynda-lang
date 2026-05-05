    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse import alias");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for import alias");
    ASSERT_EQ_INT(1, program.import_count, "one import");
    ASSERT_EQ_INT(AST_IMPORT_ALIAS, program.imports[0].kind, "import alias kind");
    ASSERT_EQ_STR("std", program.imports[0].alias, "import alias name");
    ASSERT_EQ_INT(2, program.imports[0].module_name.count, "import module segments");
    ast_program_free(&program);
    parser_free(&parser);
}


void test_parse_import_wildcard(void) {
    const char *source = "import io.stdlib.*;\nint32 x = 0;\n";
    Parser parser;
    AstProgram program;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse import wildcard");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for import wildcard");
    ASSERT_EQ_INT(1, program.import_count, "one import");
    ASSERT_EQ_INT(AST_IMPORT_WILDCARD, program.imports[0].kind, "import wildcard kind");
    ast_program_free(&program);
    parser_free(&parser);
}


void test_parse_import_selective(void) {
    const char *source = "import io.stdlib.{print, readLine};\nint32 x = 0;\n";
    Parser parser;
    AstProgram program;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse import selective");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for import selective");
    ASSERT_EQ_INT(1, program.import_count, "one import");
    ASSERT_EQ_INT(AST_IMPORT_SELECTIVE, program.imports[0].kind,
                  "import selective kind");
    ASSERT_EQ_INT(2, program.imports[0].selected_count, "selective import count");
    ASSERT_EQ_STR("print", program.imports[0].selected_names[0],
                  "first selective import name");
    ASSERT_EQ_STR("readLine", program.imports[0].selected_names[1],
                  "second selective import name");
    ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/* V2 feature: Java-style primitive aliases                           */
/* ------------------------------------------------------------------ */
void test_parse_java_primitive_aliases(void) {
    const char *source =
        "int x = 5;\n"
        "double pi = 3.14;\n"
        "byte b = 0;\n";
    Parser parser;
    AstProgram program;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse java aliases");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for java aliases");
    ASSERT_EQ_INT(3, program.top_level_count, "three top-level decls");
    ASSERT_EQ_INT(AST_PRIMITIVE_INT,
                  program.top_level_decls[0]->as.binding_decl.declared_type.primitive,
                  "int alias primitive type");
    ASSERT_EQ_INT(AST_PRIMITIVE_DOUBLE,
                  program.top_level_decls[1]->as.binding_decl.declared_type.primitive,
                  "double alias primitive type");
    ASSERT_EQ_INT(AST_PRIMITIVE_BYTE,
                  program.top_level_decls[2]->as.binding_decl.declared_type.primitive,
                  "byte alias primitive type");
    ast_program_free(&program);
    parser_free(&parser);
}
