    type_resolver_init(&resolver);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse Future/Atomic resolution");
    REQUIRE_TRUE(type_resolver_resolve_program(&resolver, &program),
                 "resolve Future/Atomic types");

    resolved = type_resolver_get_type(&resolver,
                                      &program.top_level_decls[0]
                                           ->as.binding_decl.declared_type);
    REQUIRE_TRUE(resolved != NULL, "Future resolved type exists");
    ASSERT_EQ_INT(RESOLVED_TYPE_NAMED, resolved->kind, "Future resolves as named");
    ASSERT_TRUE(resolved->name != NULL && strcmp(resolved->name, "Future") == 0,
                "Future resolved name is Future");
    ASSERT_EQ_INT(1, (int)resolved->generic_arg_count, "Future has one generic arg");

    resolved = type_resolver_get_type(&resolver,
                                      &program.top_level_decls[1]
                                           ->as.binding_decl.declared_type);
    REQUIRE_TRUE(resolved != NULL, "Atomic resolved type exists");
    ASSERT_EQ_INT(RESOLVED_TYPE_NAMED, resolved->kind, "Atomic resolves as named");
    ASSERT_TRUE(resolved->name != NULL && strcmp(resolved->name, "Atomic") == 0,
                "Atomic resolved name is Atomic");
    ASSERT_EQ_INT(1, (int)resolved->generic_arg_count, "Atomic has one generic arg");

    type_resolver_free(&resolver);
    ast_program_free(&program);
    parser_free(&parser);
}
