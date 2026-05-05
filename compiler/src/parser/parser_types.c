#include "parser_internal.h"

bool parser_parse_qualified_name(Parser *parser, AstQualifiedName *name) {
    ast_qualified_name_init(name);

    for (;;) {
        const Token *segment_token;
        char *segment;
        bool ok;

        if (!parser_check(parser, TOK_IDENTIFIER)) {
            parser_set_error(parser, *parser_current_token(parser),
                             "Expected identifier in qualified name.");
            ast_qualified_name_free(name);
            return false;
        }

        segment_token = parser_current_token(parser);
        segment = parser_copy_token_text(segment_token);
        if (!segment) {
            parser_set_oom_error(parser);
            ast_qualified_name_free(name);
            return false;
        }

        parser_extend_source_span(&name->source_span, segment_token);
        name->tail_span = parser_source_span(segment_token);

        ok = ast_qualified_name_append(name, segment);
        free(segment);
        if (!ok) {
            parser_set_oom_error(parser);
            ast_qualified_name_free(name);
            return false;
        }

        parser_advance(parser);
        if (!parser_check(parser, TOK_DOT)) {
            break;
        }
        /* Stop before '.' if followed by '*' or '{' (import wildcard / selective). */
        {
            const Token *after_dot = parser_token_at(parser, parser->current + 1);
            if (after_dot->type == TOK_STAR || after_dot->type == TOK_LBRACE) {
                break;
            }
        }
        parser_advance(parser); /* consume the '.' */
    }

    return true;
}

bool parser_parse_type(Parser *parser, AstType *type) {
    AstType parsed_type;

    memset(&parsed_type, 0, sizeof(parsed_type));

    if (parser_match(parser, TOK_VOID)) {
        ast_type_init_void(&parsed_type);
        *type = parsed_type;
        return true;
    }

    if (parser_match(parser, TOK_ARR) || parser_match(parser, TOK_PTR)) {
        parsed_type.kind = (parser_previous_token(parser)->type == TOK_PTR)
                               ? AST_TYPE_PTR : AST_TYPE_ARR;
        if (!parser_parse_generic_args(parser, &parsed_type)) {
            ast_type_free(&parsed_type);
            return false;
        }
        *type = parsed_type;
        return true;
    }

    if (parser_check(parser, TOK_IDENTIFIER)) {
        char *name = parser_copy_token_text(parser_current_token(parser));
        if (!name) {
            parser_set_oom_error(parser);
            return false;
        }
        parser_advance(parser);
        if (strcmp(name, "Thread") == 0) {
            ast_type_init_thread(&parsed_type);
        } else if (strcmp(name, "Mutex") == 0) {
            ast_type_init_mutex(&parsed_type);
        } else if (strcmp(name, "Future") == 0) {
            ast_type_init_future(&parsed_type);
        } else if (strcmp(name, "Atomic") == 0) {
            ast_type_init_atomic(&parsed_type);
        } else {
            ast_type_init_named(&parsed_type, name);
        }
        free(name);
        if (!parsed_type.name) {
            if (parsed_type.kind == AST_TYPE_THREAD || parsed_type.kind == AST_TYPE_MUTEX ||
                parsed_type.kind == AST_TYPE_FUTURE || parsed_type.kind == AST_TYPE_ATOMIC) {
                /* no-op */
            } else {
                parser_set_oom_error(parser);
                return false;
            }
        }
    } else if (parser_match(parser, TOK_CHECKED)) {
        /* 'checked' keyword used as a type-level marker in ptr<T, checked>. */
        ast_type_init_named(&parsed_type, "checked");
        if (!parsed_type.name) {
            parser_set_oom_error(parser);
            return false;
        }
    } else if (is_primitive_type_token(parser_current_token(parser)->type)) {
        ast_type_init_primitive(&parsed_type,
                                primitive_type_from_token(
                                    parser_current_token(parser)->type));
        parser_advance(parser);
    } else {
        parser_set_error(parser, *parser_current_token(parser), "Expected type.");
        return false;
    }

    /* Optional generic arguments. */
    if ((parsed_type.kind == AST_TYPE_NAMED || parsed_type.kind == AST_TYPE_ARR ||
         parsed_type.kind == AST_TYPE_PTR || parsed_type.kind == AST_TYPE_FUTURE ||
         parsed_type.kind == AST_TYPE_ATOMIC) &&
        parser_check(parser, TOK_LT)) {
        if (!parser_parse_generic_args(parser, &parsed_type)) {
            ast_type_free(&parsed_type);
            return false;
        }
    }

    /* Optional array dimensions. */
    while (parser_match(parser, TOK_LBRACKET)) {
        if (parser_match(parser, TOK_INT_LIT)) {
            char *size_literal = parser_copy_token_text(parser_previous_token(parser));
            bool ok;

            if (!size_literal) {
                parser_set_oom_error(parser);
                ast_type_free(&parsed_type);
                return false;
            }

            if (!parser_consume(parser, TOK_RBRACKET,
                                "Expected ']' after sized array dimension.")) {
                free(size_literal);
                ast_type_free(&parsed_type);
                return false;
            }

            ok = ast_type_add_dimension(&parsed_type, true, size_literal);
            free(size_literal);
            if (!ok) {
                parser_set_oom_error(parser);
                ast_type_free(&parsed_type);
                return false;
            }
        } else {
            if (!parser_consume(parser, TOK_RBRACKET,
                                "Expected ']' after array dimension.")) {
                ast_type_free(&parsed_type);
                return false;
            }

            if (!ast_type_add_dimension(&parsed_type, false, NULL)) {
                parser_set_oom_error(parser);
                ast_type_free(&parsed_type);
                return false;
            }
        }
    }

    *type = parsed_type;
    return true;
}

bool parser_parse_generic_args(Parser *parser, AstType *type) {
    if (!parser_consume(parser, TOK_LT, "Expected '<'.")) {
        return false;
    }

    do {
        AstGenericArg arg;

        memset(&arg, 0, sizeof(arg));

        if (parser_match(parser, TOK_QUESTION)) {
            arg.kind = AST_GENERIC_ARG_WILDCARD;
            arg.type = NULL;
        } else {
            arg.type = malloc(sizeof(AstType));
            if (!arg.type) {
                parser_set_oom_error(parser);
                return false;
            }

            if (!parser_parse_type(parser, arg.type)) {
                free(arg.type);
                return false;
            }

            arg.kind = AST_GENERIC_ARG_TYPE;
        }

        if (!ast_type_add_generic_arg(type, &arg)) {
            if (arg.type) {
                ast_type_free(arg.type);
                free(arg.type);
            }
            parser_set_oom_error(parser);
            return false;
        }
    } while (parser_match(parser, TOK_COMMA));

    return parser_consume_gt(parser,
                             "Expected '>' to close generic arguments.");
}

bool parser_parse_parameter_list(Parser *parser, AstParameterList *list,
                                 bool allow_empty) {
    ast_parameter_list_init(list);

    if (parser_check(parser, TOK_RPAREN)) {
        if (allow_empty) {
            return true;
        }

        parser_set_error(parser, *parser_current_token(parser),
                         "Expected at least one parameter.");
        return false;
    }

    for (;;) {
        AstParameter parameter;
        const Token *name_token;

#include "parser_types_p2.inc"
