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
