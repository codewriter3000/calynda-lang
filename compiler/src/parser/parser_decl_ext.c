#include "parser_internal.h"

AstBlock *parse_block(Parser *parser) {
    AstBlock *block = ast_block_new();

    if (!block) {
        parser_set_oom_error(parser);
        return NULL;
    }

    if (!parser_consume(parser, TOK_LBRACE, "Expected '{' to start block.")) {
        ast_block_free(block);
        return NULL;
    }

    while (!parser_check(parser, TOK_RBRACE) && !parser_check(parser, TOK_EOF)) {
        AstStatement *statement = parse_statement(parser);

        if (!statement) {
            ast_block_free(block);
            return NULL;
        }

        if (!parser_add_statement(parser, block, statement)) {
            ast_block_free(block);
            return NULL;
        }
    }

    if (!parser_consume(parser, TOK_RBRACE, "Expected '}' after block.")) {
        ast_block_free(block);
        return NULL;
    }

    return block;
}

AstTopLevelDecl *parse_asm_decl(Parser *parser) {
    AstTopLevelDecl *decl = ast_top_level_decl_new(AST_TOP_LEVEL_ASM);
    const Token *name_token;
    const Token *body_token;

    if (!decl) {
        parser_set_oom_error(parser);
        return NULL;
    }

    /* Parse modifiers */
    while (parser_check(parser, TOK_PUBLIC) || parser_check(parser, TOK_PRIVATE) ||
           parser_check(parser, TOK_FINAL) || parser_check(parser, TOK_EXPORT) ||
           parser_check(parser, TOK_STATIC) || parser_check(parser, TOK_INTERNAL) ||
           parser_check(parser, TOK_THREAD_LOCAL)) {
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
        case TOK_THREAD_LOCAL:
            parser_set_error(parser, *parser_current_token(parser),
                             "'thread_local' is only valid on top-level bindings.");
            ast_top_level_decl_free(decl);
            return NULL;
        default:
            modifier = AST_MODIFIER_FINAL;
            break;
        }

        parser_advance(parser);
        if (!ast_asm_decl_add_modifier(&decl->as.asm_decl, modifier)) {
            parser_set_oom_error(parser);
            ast_top_level_decl_free(decl);
            return NULL;
        }
    }

    /* Parse return type */
    if (!parser_parse_type(parser, &decl->as.asm_decl.return_type)) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    /* Parse name */
    name_token = parser_current_token(parser);
    decl->as.asm_decl.name =
        parser_consume_identifier(parser, "Expected asm binding name.");
    if (!decl->as.asm_decl.name) {
        ast_top_level_decl_free(decl);
        return NULL;
    }
    decl->as.asm_decl.name_span = parser_source_span(name_token);

    /* = asm ( params ) -> body ; */
    if (!parser_consume(parser, TOK_ASSIGN, "Expected '=' after asm binding name.") ||
        !parser_consume(parser, TOK_ASM, "Expected 'asm' keyword.") ||
        !parser_consume(parser, TOK_LPAREN, "Expected '(' after 'asm'.") ||
        !parser_parse_parameter_list(parser, &decl->as.asm_decl.parameters, true) ||
        !parser_consume(parser, TOK_RPAREN, "Expected ')' after asm parameters.") ||
        !parser_consume(parser, TOK_ARROW, "Expected '->' after asm parameters.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    /* Consume the TOK_ASM_BODY token (raw text between { and }) */
    body_token = parser_current_token(parser);
    if (body_token->type != TOK_ASM_BODY) {
        parser_set_error(parser, *body_token,
                         "Expected asm body block '{ ... }'.");
        ast_top_level_decl_free(decl);
        return NULL;
    }
    decl->as.asm_decl.body = ast_copy_text_n(body_token->start, body_token->length);
    decl->as.asm_decl.body_length = body_token->length;
    if (!decl->as.asm_decl.body) {
        parser_set_oom_error(parser);
        ast_top_level_decl_free(decl);
        return NULL;
    }
    parser_advance(parser);

    if (!parser_consume(parser, TOK_SEMICOLON,
                        "Expected ';' after asm declaration.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    return decl;
}
