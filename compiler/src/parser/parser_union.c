#include "parser_internal.h"

AstTopLevelDecl *parse_union_decl(Parser *parser) {
    AstTopLevelDecl *decl = ast_top_level_decl_new(AST_TOP_LEVEL_UNION);
    const Token *name_token;

    if (!decl) {
        parser_set_oom_error(parser);
        return NULL;
    }

    /* Consume optional modifiers before 'union'. */
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
        if (!ast_union_decl_add_modifier(&decl->as.union_decl, modifier)) {
            parser_set_oom_error(parser);
            ast_top_level_decl_free(decl);
            return NULL;
        }
    }

    if (!parser_consume(parser, TOK_UNION, "Expected 'union'.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    name_token = parser_current_token(parser);
    decl->as.union_decl.name =
        parser_consume_identifier(parser, "Expected union name.");
    if (!decl->as.union_decl.name) {
        ast_top_level_decl_free(decl);
        return NULL;
    }
    decl->as.union_decl.name_span = parser_source_span(name_token);

    /* Optional generic parameters: <T, E> */
    if (parser_match(parser, TOK_LT)) {
        do {
            char *param = parser_consume_identifier(
                parser, "Expected generic parameter name.");
            if (!param) {
                ast_top_level_decl_free(decl);
                return NULL;
            }
            if (!ast_union_decl_add_generic_param(&decl->as.union_decl, param)) {
                free(param);
                parser_set_oom_error(parser);
                ast_top_level_decl_free(decl);
                return NULL;
            }
            free(param);
        } while (parser_match(parser, TOK_COMMA));

        if (!parser_consume(parser, TOK_GT,
                            "Expected '>' after generic parameters.")) {
            ast_top_level_decl_free(decl);
            return NULL;
        }
    }

    if (!parser_consume(parser, TOK_LBRACE,
                        "Expected '{' to start union variants.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    /* Parse variant list: Name [ "(" Type ")" ] { "," ... } */
    do {
        AstUnionVariant variant;
        char *variant_name;

        memset(&variant, 0, sizeof(variant));
        variant_name = parser_consume_identifier(
            parser, "Expected union variant name.");
        if (!variant_name) {
            ast_top_level_decl_free(decl);
            return NULL;
        }
        variant.name = variant_name;

        if (parser_match(parser, TOK_LPAREN)) {
            AstType *payload = calloc(1, sizeof(*payload));
            if (!payload) {
                free(variant_name);
                parser_set_oom_error(parser);
                ast_top_level_decl_free(decl);
                return NULL;
            }
            if (!parser_parse_type(parser, payload)) {
                free(payload);
                free(variant_name);
                ast_top_level_decl_free(decl);
                return NULL;
            }
            if (!parser_consume(parser, TOK_RPAREN,
                                "Expected ')' after variant payload type.")) {
                ast_type_free(payload);
                free(payload);
                free(variant_name);
                ast_top_level_decl_free(decl);
                return NULL;
            }
            variant.payload_type = payload;
        }

        if (!ast_union_decl_add_variant(&decl->as.union_decl, &variant)) {
            if (variant.payload_type) {
                ast_type_free(variant.payload_type);
                free(variant.payload_type);
            }
            free(variant_name);
            parser_set_oom_error(parser);
            ast_top_level_decl_free(decl);
            return NULL;
        }
    } while (parser_match(parser, TOK_COMMA));

    if (!parser_consume(parser, TOK_RBRACE,
                        "Expected '}' after union variants.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    if (!parser_consume(parser, TOK_SEMICOLON,
                        "Expected ';' after union declaration.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    return decl;
}
