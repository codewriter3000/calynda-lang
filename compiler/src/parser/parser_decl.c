#include "parser_internal.h"

/* Check if the declaration starting at the current position is an asm decl.
   Lookahead past modifiers, type, name, '=' to see if 'asm' follows. */
static bool looks_like_asm_decl(const Parser *parser) {
    size_t ahead = parser->current;

    /* Skip modifiers */
    while (true) {
        TokenType t = parser_token_at(parser, ahead)->type;
        if (t == TOK_PUBLIC || t == TOK_PRIVATE || t == TOK_FINAL ||
            t == TOK_EXPORT || t == TOK_STATIC || t == TOK_INTERNAL) {
            ahead++;
        } else {
            break;
        }
    }

    /* Skip type (use scan_type_pattern) */
    if (!scan_type_pattern(parser, &ahead)) {
        return false;
    }

    /* Skip name (identifier) */
    if (parser_token_at(parser, ahead)->type != TOK_IDENTIFIER) {
        return false;
    }
    ahead++;

    /* Check = */
    if (parser_token_at(parser, ahead)->type != TOK_ASSIGN) {
        return false;
    }
    ahead++;

    /* Check asm */
    return parser_token_at(parser, ahead)->type == TOK_ASM;
}

bool parser_parse_lambda_body(Parser *parser, AstLambdaBody *body) {
    if (parser_check(parser, TOK_LBRACE)) {
        AstBlock *block = parse_block(parser);

        if (!block) {
            return false;
        }

        body->kind = AST_LAMBDA_BODY_BLOCK;
        body->as.block = block;
        return true;
    }

    body->kind = AST_LAMBDA_BODY_EXPRESSION;
    body->as.expression = parse_assignment_expression(parser);
    return body->as.expression != NULL;
}

bool parser_parse_block_or_expression_body(Parser *parser,
                                           AstLambdaBody *body) {
    if (parser_check(parser, TOK_LBRACE)) {
        AstBlock *block = parse_block(parser);

        if (!block) {
            return false;
        }

        body->kind = AST_LAMBDA_BODY_BLOCK;
        body->as.block = block;
        return true;
    }

    body->kind = AST_LAMBDA_BODY_EXPRESSION;
    body->as.expression = parse_expression_node(parser);
    return body->as.expression != NULL;
}

AstTopLevelDecl *parse_top_level_decl(Parser *parser) {
    if (parser_check(parser, TOK_START)) {
        return parse_start_decl(parser);
    }

    if (parser_check(parser, TOK_BOOT)) {
        return parse_boot_decl(parser);
    }

    /* Check for asm declarations before binding/union dispatch. */
    if (looks_like_asm_decl(parser)) {
        return parse_asm_decl(parser);
    }

    if (parser_check(parser, TOK_LAYOUT)) {
        return parse_layout_decl(parser);
    }

    /* Peek past any modifier tokens to decide binding vs union. */
    if (parser_check(parser, TOK_PUBLIC) || parser_check(parser, TOK_PRIVATE) ||
        parser_check(parser, TOK_FINAL) || parser_check(parser, TOK_EXPORT) ||
        parser_check(parser, TOK_STATIC) || parser_check(parser, TOK_INTERNAL)) {
        size_t ahead = parser->current;
        while (true) {
            TokenType t = parser_token_at(parser, ahead)->type;
            if (t == TOK_PUBLIC || t == TOK_PRIVATE || t == TOK_FINAL ||
                t == TOK_EXPORT || t == TOK_STATIC || t == TOK_INTERNAL) {
                ahead++;
            } else {
                break;
            }
        }
        if (parser_token_at(parser, ahead)->type == TOK_UNION) {
            return parse_union_decl(parser);
        }
        return parse_binding_decl(parser);
    }

    if (parser_check(parser, TOK_VAR) ||
        is_type_start_token(parser_current_token(parser)->type)) {
        return parse_binding_decl(parser);
    }

    if (parser_check(parser, TOK_UNION)) {
        return parse_union_decl(parser);
    }

    parser_set_error(parser, *parser_current_token(parser),
                     "Expected top-level declaration.");
    return NULL;
}

AstTopLevelDecl *parse_start_decl(Parser *parser) {
    AstTopLevelDecl *decl = ast_top_level_decl_new(AST_TOP_LEVEL_START);

    if (!decl) {
        parser_set_oom_error(parser);
        return NULL;
    }

    decl->as.start_decl.start_span = parser_source_span(parser_current_token(parser));

    if (!parser_consume(parser, TOK_START, "Expected 'start'.") ||
        !parser_consume(parser, TOK_LPAREN, "Expected '(' after 'start'.") ||
        !parser_parse_parameter_list(parser, &decl->as.start_decl.parameters, false) ||
        !parser_consume(parser, TOK_RPAREN, "Expected ')' after start parameters.") ||
        !parser_consume(parser, TOK_ARROW, "Expected '->' after start parameters.") ||
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
        !parser_consume(parser, TOK_LPAREN, "Expected '(' after 'boot'.") ||
        !parser_consume(parser, TOK_RPAREN, "Expected ')' after 'boot('.") ||
        !parser_consume(parser, TOK_ARROW, "Expected '->' after boot parameters.") ||
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
           parser_check(parser, TOK_STATIC) || parser_check(parser, TOK_INTERNAL)) {
        AstModifier modifier;

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
        default:
            modifier = AST_MODIFIER_FINAL;
            break;
        }

        parser_advance(parser);
        if (!parser_add_modifier(parser, &decl->as.binding_decl, modifier)) {
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

