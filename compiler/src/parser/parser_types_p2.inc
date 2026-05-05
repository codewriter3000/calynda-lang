        memset(&parameter, 0, sizeof(parameter));
        if (parser_match(parser, TOK_PIPE) && parser_check(parser, TOK_VAR)) {
            parser_advance(parser); /* consume TOK_VAR */
            parameter.is_untyped = true;
            parameter.is_block = true;
        } else if (parser_match(parser, TOK_VAR)) {
            parameter.is_untyped = true;
        } else if (parser_check(parser, TOK_IDENTIFIER) &&
                   (parser_token_at(parser, parser->current + 1)->type == TOK_COMMA ||
                    parser_token_at(parser, parser->current + 1)->type == TOK_RPAREN ||
                    parser_token_at(parser, parser->current + 1)->type == TOK_ASSIGN)) {
            /* bare shorthand: untyped parameter with just a name, no type annotation */
            parameter.is_untyped = true;
        } else if (!parser_parse_type(parser, &parameter.type)) {
            ast_parameter_list_free(list);
            return false;
        }

        if (!parameter.is_untyped && parser_match(parser, TOK_ELLIPSIS)) {
            parameter.is_varargs = true;
        }

        name_token = parser_current_token(parser);
        parameter.name = parser_consume_identifier(parser, "Expected parameter name.");
        if (!parameter.name) {
            ast_type_free(&parameter.type);
            ast_parameter_list_free(list);
            return false;
        }
        parameter.name_span = parser_source_span(name_token);

        if (parser_match(parser, TOK_ASSIGN)) {
            parameter.default_expr = parse_expression_node(parser);
            if (!parameter.default_expr) {
                ast_type_free(&parameter.type);
                free(parameter.name);
                ast_parameter_list_free(list);
                return false;
            }
        }

        if (!parser_add_parameter(parser, list, &parameter)) {
            ast_parameter_list_free(list);
            return false;
        }

        if (parameter.is_varargs) {
            break;
        }

        if (!parser_match(parser, TOK_COMMA)) {
            break;
        }
    }

    return true;
}
