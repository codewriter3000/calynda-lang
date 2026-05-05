#include "parser_internal.h"

bool is_primitive_type_token(TokenType type) {
    switch (type) {
    case TOK_INT8:
    case TOK_INT16:
    case TOK_INT32:
    case TOK_INT64:
    case TOK_UINT8:
    case TOK_UINT16:
    case TOK_UINT32:
    case TOK_UINT64:
    case TOK_FLOAT32:
    case TOK_FLOAT64:
    case TOK_BOOL:
    case TOK_CHAR:
    case TOK_STRING:
    case TOK_BYTE:
    case TOK_SBYTE:
    case TOK_SHORT:
    case TOK_INT:
    case TOK_LONG:
    case TOK_ULONG:
    case TOK_UINT:
    case TOK_FLOAT:
    case TOK_DOUBLE:
        return true;
    default:
        return false;
    }
}

bool is_type_start_token(TokenType type) {
    return type == TOK_VOID || type == TOK_IDENTIFIER || type == TOK_ARR ||
            type == TOK_PTR || is_primitive_type_token(type);
}

static bool scan_default_parameter_expression_pattern(const Parser *parser, size_t *index) {
    int paren_depth = 0;
    int bracket_depth = 0;
    int brace_depth = 0;

    if (!parser || !index) {
        return false;
    }

    for (;;) {
        TokenType type = parser_token_at(parser, *index)->type;

        if (type == TOK_EOF || type == TOK_SEMICOLON) {
            return false;
        }

        if (paren_depth == 0 && bracket_depth == 0 && brace_depth == 0 &&
            (type == TOK_COMMA || type == TOK_RPAREN)) {
            return true;
        }

        switch (type) {
        case TOK_LPAREN: paren_depth++; break;
        case TOK_RPAREN:
            if (paren_depth > 0) {
                paren_depth--;
            } else {
                return true;
            }
            break;
        case TOK_LBRACKET: bracket_depth++; break;
        case TOK_RBRACKET:
            if (bracket_depth > 0) {
                bracket_depth--;
            }
            break;
        case TOK_LBRACE: brace_depth++; break;
        case TOK_RBRACE:
            if (brace_depth > 0) {
                brace_depth--;
            }
            break;
        default:
            break;
        }

        (*index)++;
    }
}

bool scan_generic_args_pattern(const Parser *parser, size_t *index) {
    int depth = 1;

    (*index)++;

    while (depth > 0) {
        TokenType t = parser_token_at(parser, *index)->type;

        if (t == TOK_EOF) {
            return false;
        }

        if (t == TOK_LT) {
            depth++;
        } else if (t == TOK_GT) {
            depth--;
        } else if (t == TOK_RSHIFT) {
            if (depth < 2) {
                return false;
            }
            depth -= 2;
        }

        (*index)++;
    }

    return true;
}

bool scan_type_pattern(const Parser *parser, size_t *index) {
    TokenType t = parser_token_at(parser, *index)->type;

    if (t == TOK_VOID) {
        (*index)++;
        return true;
    }

    if (t == TOK_ARR || t == TOK_PTR) {
        (*index)++;
        if (parser_token_at(parser, *index)->type != TOK_LT) {
            return false;
        }
        return scan_generic_args_pattern(parser, index);
    }

    if (t == TOK_IDENTIFIER) {
        (*index)++;
    } else if (is_primitive_type_token(t)) {
        (*index)++;
    } else {
        return false;
    }

    if (parser_token_at(parser, *index)->type == TOK_LT) {
        if (!scan_generic_args_pattern(parser, index)) {
            return false;
        }
    }

    while (parser_token_at(parser, *index)->type == TOK_LBRACKET) {
        (*index)++;
        if (parser_token_at(parser, *index)->type == TOK_INT_LIT) {
            (*index)++;
        }
        if (parser_token_at(parser, *index)->type != TOK_RBRACKET) {
            return false;
        }
        (*index)++;
    }

    return true;
}

bool looks_like_lambda_expression(const Parser *parser) {
    size_t index = parser ? parser->current : 0;
    bool has_manual = false;

    if (parser_token_at(parser, index)->type == TOK_MANUAL) {
        has_manual = true;
        index++;
    }

    /* shorthand (no manual): bare identifier followed by -> */
    if (!has_manual &&
        parser_token_at(parser, index)->type == TOK_IDENTIFIER &&
        parser_token_at(parser, index + 1)->type == TOK_ARROW) {
        return true;
    }

    if (parser_token_at(parser, index)->type != TOK_LPAREN) {
        return false;
    }

    index++;
    if (parser_token_at(parser, index)->type == TOK_RPAREN) {
        return parser_token_at(parser, index + 1)->type == TOK_ARROW;
    }

    for (;;) {
        bool consumed_name = false;

        if (parser_token_at(parser, index)->type == TOK_PIPE &&
            parser_token_at(parser, index + 1)->type == TOK_VAR) {
            index += 2;
        } else if (parser_token_at(parser, index)->type == TOK_VAR) {
            index++;
        } else if (parser_token_at(parser, index)->type == TOK_IDENTIFIER) {
            TokenType following = parser_token_at(parser, index + 1)->type;
            if (following == TOK_COMMA || following == TOK_RPAREN ||
                following == TOK_ASSIGN) {
                /* bare untyped shorthand: the identifier itself is the name */
                index++;
                consumed_name = true;
            } else {
                if (!scan_type_pattern(parser, &index)) {
                    return false;
                }
                if (parser_token_at(parser, index)->type == TOK_ELLIPSIS) {
                    index++;
                }
            }
        } else {
            if (!scan_type_pattern(parser, &index)) {
                return false;
            }

            /* skip optional varargs ellipsis between type and name */
            if (parser_token_at(parser, index)->type == TOK_ELLIPSIS) {
                index++;
            }
        }

        if (!consumed_name) {
            if (parser_token_at(parser, index)->type != TOK_IDENTIFIER) {
                return false;
            }
            index++;
        }
        if (parser_token_at(parser, index)->type == TOK_ASSIGN) {
            index++;
            if (!scan_default_parameter_expression_pattern(parser, &index)) {
                return false;
            }
        }
        if (parser_token_at(parser, index)->type == TOK_COMMA) {
            index++;
            continue;
        }

        if (parser_token_at(parser, index)->type != TOK_RPAREN) {
            return false;
        }

        return parser_token_at(parser, index + 1)->type == TOK_ARROW;
    }
}

#include "parser_lookahead_p2.inc"
