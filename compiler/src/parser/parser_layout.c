#include "parser_internal.h"
#include "ast_internal.h"

#include <stdlib.h>
#include <string.h>

/* Parse: layout Name { type field; ... };
   Fields use primitive type keywords only for 0.4.0. */
AstTopLevelDecl *parse_layout_decl(Parser *parser) {
    AstTopLevelDecl *decl;
    const Token *name_token;

    decl = ast_top_level_decl_new(AST_TOP_LEVEL_LAYOUT);
    if (!decl) {
        parser_set_oom_error(parser);
        return NULL;
    }

    if (!parser_consume(parser, TOK_LAYOUT, "Expected 'layout'.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    name_token = parser_current_token(parser);
    decl->as.layout_decl.name =
        parser_consume_identifier(parser, "Expected layout name.");
    if (!decl->as.layout_decl.name) {
        ast_top_level_decl_free(decl);
        return NULL;
    }
    decl->as.layout_decl.name_span = parser_source_span(name_token);

    if (!parser_consume(parser, TOK_LBRACE, "Expected '{' after layout name.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    while (!parser_check(parser, TOK_RBRACE) && !parser_check(parser, TOK_EOF)) {
        AstLayoutField field;
        char *field_name;

        memset(&field, 0, sizeof(field));
        if (!parser_parse_type(parser, &field.field_type)) {
            ast_top_level_decl_free(decl);
            return NULL;
        }

        field_name = parser_consume_identifier(parser, "Expected field name.");
        if (!field_name) {
            ast_type_free(&field.field_type);
            ast_top_level_decl_free(decl);
            return NULL;
        }
        field.name = field_name;

        if (!parser_consume(parser, TOK_SEMICOLON,
                            "Expected ';' after layout field.")) {
            ast_type_free(&field.field_type);
            free(field.name);
            ast_top_level_decl_free(decl);
            return NULL;
        }

        if (!ast_reserve_items((void **)&decl->as.layout_decl.fields,
                               &decl->as.layout_decl.field_capacity,
                               decl->as.layout_decl.field_count + 1,
                               sizeof(*decl->as.layout_decl.fields))) {
            ast_type_free(&field.field_type);
            free(field.name);
            parser_set_oom_error(parser);
            ast_top_level_decl_free(decl);
            return NULL;
        }
        decl->as.layout_decl.fields[decl->as.layout_decl.field_count++] = field;
    }

    if (!parser_consume(parser, TOK_RBRACE, "Expected '}' after layout body.") ||
        !parser_consume(parser, TOK_SEMICOLON,
                        "Expected ';' after layout declaration.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    return decl;
}
