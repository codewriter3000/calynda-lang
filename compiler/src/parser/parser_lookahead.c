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

    if (parser_token_at(parser, index)->type == TOK_MANUAL) {
        index++;
    }

    if (parser_token_at(parser, index)->type != TOK_LPAREN) {
        return false;
    }

    index++;
    if (parser_token_at(parser, index)->type == TOK_RPAREN) {
        return parser_token_at(parser, index + 1)->type == TOK_ARROW;
    }

    for (;;) {
        if (!scan_type_pattern(parser, &index)) {
            return false;
        }

        /* skip optional varargs ellipsis between type and name */
        if (parser_token_at(parser, index)->type == TOK_ELLIPSIS) {
            index++;
        }

        if (parser_token_at(parser, index)->type != TOK_IDENTIFIER) {
            return false;
        }

        index++;
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

bool looks_like_local_binding_statement(const Parser *parser) {
    size_t index = parser ? parser->current : 0;

    if (parser_token_at(parser, index)->type == TOK_THREAD_LOCAL) {
        return false;
    }

    if (parser_token_at(parser, index)->type == TOK_INTERNAL) {
        index++;
    }

    if (parser_token_at(parser, index)->type == TOK_FINAL) {
        index++;
    }

    if (parser_token_at(parser, index)->type == TOK_VAR) {
        index++;
    } else if (!scan_type_pattern(parser, &index)) {
        return false;
    }

    return parser_token_at(parser, index)->type == TOK_IDENTIFIER &&
           parser_token_at(parser, index + 1)->type == TOK_ASSIGN;
}

AstPrimitiveType primitive_type_from_token(TokenType type) {
    switch (type) {
    case TOK_INT8: return AST_PRIMITIVE_INT8;
    case TOK_INT16: return AST_PRIMITIVE_INT16;
    case TOK_INT32: return AST_PRIMITIVE_INT32;
    case TOK_INT64: return AST_PRIMITIVE_INT64;
    case TOK_UINT8: return AST_PRIMITIVE_UINT8;
    case TOK_UINT16: return AST_PRIMITIVE_UINT16;
    case TOK_UINT32: return AST_PRIMITIVE_UINT32;
    case TOK_UINT64: return AST_PRIMITIVE_UINT64;
    case TOK_FLOAT32: return AST_PRIMITIVE_FLOAT32;
    case TOK_FLOAT64: return AST_PRIMITIVE_FLOAT64;
    case TOK_BOOL: return AST_PRIMITIVE_BOOL;
    case TOK_CHAR: return AST_PRIMITIVE_CHAR;
    case TOK_BYTE: return AST_PRIMITIVE_BYTE;
    case TOK_SBYTE: return AST_PRIMITIVE_SBYTE;
    case TOK_SHORT: return AST_PRIMITIVE_SHORT;
    case TOK_INT: return AST_PRIMITIVE_INT;
    case TOK_LONG: return AST_PRIMITIVE_LONG;
    case TOK_ULONG: return AST_PRIMITIVE_ULONG;
    case TOK_UINT: return AST_PRIMITIVE_UINT;
    case TOK_FLOAT: return AST_PRIMITIVE_FLOAT;
    case TOK_DOUBLE: return AST_PRIMITIVE_DOUBLE;
    default: return AST_PRIMITIVE_STRING;
    }
}

char *copy_template_segment_text(const Token *token) {
    size_t start_offset = 0;
    size_t end_offset = 0;

    if (!token) {
        return NULL;
    }

    switch (token->type) {
    case TOK_TEMPLATE_FULL:
        start_offset = 1;
        end_offset = 1;
        break;
    case TOK_TEMPLATE_START:
    case TOK_TEMPLATE_MIDDLE:
        start_offset = 1;
        break;
    case TOK_TEMPLATE_END:
        start_offset = 1;
        end_offset = 1;
        break;
    default:
        return NULL;
    }

    if (token->length < start_offset + end_offset) {
        return ast_copy_text("");
    }

    return ast_copy_text_n(token->start + start_offset,
                           token->length - start_offset - end_offset);
}
