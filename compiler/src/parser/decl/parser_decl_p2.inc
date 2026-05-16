    if (!parser_consume(parser, TOK_START, "Expected 'start'.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    if (parser_match(parser, TOK_LPAREN)) {
        if (!parser_parse_parameter_list(parser, &decl->as.start_decl.parameters, true) ||
            !parser_consume(parser, TOK_RPAREN, "Expected ')' after start parameters.")) {
            ast_top_level_decl_free(decl);
            return NULL;
        }
    }

    if (!parser_consume(parser, TOK_ARROW, "Expected '->' after 'start'.") ||
        !parser_parse_block_or_expression_body(parser, &decl->as.start_decl.body) ||
        !parser_consume(parser, TOK_SEMICOLON, "Expected ';' after start declaration.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    return decl;
}

AstTopLevelDecl *parse_boot_decl(Parser *parser) {
    AstTopLevelDecl *decl = ast_top_level_decl_new(AST_TOP_LEVEL_START);

    if (!decl) {
        parser_set_oom_error(parser);
        return NULL;
    }

    decl->as.start_decl.is_boot = true;
    decl->as.start_decl.start_span = parser_source_span(parser_current_token(parser));

    if (!parser_consume(parser, TOK_BOOT, "Expected 'boot'.") ||
        !parser_consume(parser, TOK_ARROW, "Expected '->' after 'boot'.") ||
        !parser_parse_block_or_expression_body(parser, &decl->as.start_decl.body) ||
        !parser_consume(parser, TOK_SEMICOLON, "Expected ';' after boot declaration.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    return decl;
}

AstTopLevelDecl *parse_binding_decl(Parser *parser) {
    AstTopLevelDecl *decl = ast_top_level_decl_new(AST_TOP_LEVEL_BINDING);
    const Token *name_token;

    if (!decl) {
        parser_set_oom_error(parser);
        return NULL;
    }

    while (parser_check(parser, TOK_PUBLIC) || parser_check(parser, TOK_PRIVATE) ||
           parser_check(parser, TOK_FINAL) || parser_check(parser, TOK_EXPORT) ||
           parser_check(parser, TOK_STATIC) || parser_check(parser, TOK_INTERNAL) ||
           parser_check(parser, TOK_THREAD_LOCAL)) {
        AstModifier modifier;
        const Token *modifier_token = parser_current_token(parser);

        switch (parser_current_token(parser)->type) {
        case TOK_PUBLIC:
            modifier = AST_MODIFIER_PUBLIC;
            break;
        case TOK_PRIVATE:
            modifier = AST_MODIFIER_PRIVATE;
            break;
        case TOK_EXPORT:
            modifier = AST_MODIFIER_EXPORT;
            break;
        case TOK_STATIC:
            modifier = AST_MODIFIER_STATIC;
            break;
        case TOK_INTERNAL:
            modifier = AST_MODIFIER_INTERNAL;
            break;
        case TOK_THREAD_LOCAL:
            modifier = AST_MODIFIER_THREAD_LOCAL;
            break;
        default:
            modifier = AST_MODIFIER_FINAL;
            break;
        }

        parser_advance(parser);
        if (!binding_decl_accepts_modifier(parser,
                                           &decl->as.binding_decl,
                                           modifier,
                                           modifier_token)) {
            ast_top_level_decl_free(decl);
            return NULL;
        }
    }

    if (parser_match(parser, TOK_VAR)) {
        decl->as.binding_decl.is_inferred_type = true;
    } else if (!parser_parse_type(parser, &decl->as.binding_decl.declared_type)) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    name_token = parser_current_token(parser);
    decl->as.binding_decl.name =
        parser_consume_identifier(parser, "Expected binding name.");
    if (!decl->as.binding_decl.name) {
        ast_top_level_decl_free(decl);
        return NULL;
    }
    decl->as.binding_decl.name_span = parser_source_span(name_token);

    if (!parser_consume(parser, TOK_ASSIGN, "Expected '=' after binding name.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    decl->as.binding_decl.initializer = parse_expression_node(parser);
    if (!decl->as.binding_decl.initializer) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    if (!parser_consume(parser, TOK_SEMICOLON, "Expected ';' after binding declaration.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    return decl;
}
