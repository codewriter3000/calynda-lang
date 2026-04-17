#include "parser_internal.h"

bool parser_add_statement(Parser *parser, AstBlock *block,
                          AstStatement *statement) {
    if (ast_block_append_statement(block, statement)) {
        return true;
    }

    parser_set_oom_error(parser);
    ast_statement_free(statement);
    return false;
}

AstStatement *parse_statement(Parser *parser) {
    if (parser_check(parser, TOK_THREAD_LOCAL)) {
        parser_set_error(parser, *parser_current_token(parser),
                         "'thread_local' is only valid on top-level bindings.");
        return NULL;
    }

    if (parser_check(parser, TOK_RETURN)) {
        AstStatement *statement = ast_statement_new(AST_STMT_RETURN);

        if (!statement) {
            parser_set_oom_error(parser);
            return NULL;
        }

        statement->source_span = parser_source_span(parser_current_token(parser));

        parser_advance(parser);
        if (!parser_check(parser, TOK_SEMICOLON)) {
            statement->as.return_expression = parse_expression_node(parser);
            if (!statement->as.return_expression) {
                ast_statement_free(statement);
                return NULL;
            }
        }

        if (!parser_consume(parser, TOK_SEMICOLON, "Expected ';' after return statement.")) {
            ast_statement_free(statement);
            return NULL;
        }

        return statement;
    }

    if (parser_check(parser, TOK_EXIT)) {
        AstStatement *statement = ast_statement_new(AST_STMT_EXIT);

        if (!statement) {
            parser_set_oom_error(parser);
            return NULL;
        }

        statement->source_span = parser_source_span(parser_current_token(parser));

        parser_advance(parser);
        if (!parser_consume(parser, TOK_SEMICOLON, "Expected ';' after exit statement.")) {
            ast_statement_free(statement);
            return NULL;
        }

        return statement;
    }

    if (parser_check(parser, TOK_THROW)) {
        AstStatement *statement = ast_statement_new(AST_STMT_THROW);

        if (!statement) {
            parser_set_oom_error(parser);
            return NULL;
        }

        statement->source_span = parser_source_span(parser_current_token(parser));

        parser_advance(parser);
        statement->as.throw_expression = parse_expression_node(parser);
        if (!statement->as.throw_expression) {
            ast_statement_free(statement);
            return NULL;
        }

        if (!parser_consume(parser, TOK_SEMICOLON, "Expected ';' after throw statement.")) {
            ast_statement_free(statement);
            return NULL;
        }

        return statement;
    }

    if (parser_check(parser, TOK_MANUAL)) {
        AstStatement *statement = ast_statement_new(AST_STMT_MANUAL);

        if (!statement) {
            parser_set_oom_error(parser);
            return NULL;
        }

        statement->source_span = parser_source_span(parser_current_token(parser));

        parser_advance(parser);

        /* Optional 'checked' modifier: manual checked { ... } */
        if (parser_check(parser, TOK_CHECKED)) {
            parser_advance(parser);
            statement->as.manual.is_checked = true;
        }

        statement->as.manual.body = parse_block(parser);
        if (!statement->as.manual.body) {
            ast_statement_free(statement);
            return NULL;
        }

        if (!parser_consume(parser, TOK_SEMICOLON, "Expected ';' after manual block.")) {
            ast_statement_free(statement);
            return NULL;
        }

        return statement;
    }

    if (parser_check(parser, TOK_INTERNAL) ||
        parser_check(parser, TOK_FINAL) || parser_check(parser, TOK_VAR) ||
        (is_type_start_token(parser_current_token(parser)->type) &&
         looks_like_local_binding_statement(parser))) {
        AstStatement *statement = ast_statement_new(AST_STMT_LOCAL_BINDING);
        const Token *name_token;
        const Token *start_token = parser_current_token(parser);

        if (!statement) {
            parser_set_oom_error(parser);
            return NULL;
        }

        statement->source_span = parser_source_span(start_token);

        if (parser_match(parser, TOK_INTERNAL)) {
            statement->as.local_binding.is_internal = true;
        }

        if (parser_match(parser, TOK_FINAL)) {
            statement->as.local_binding.is_final = true;
        }

        if (parser_match(parser, TOK_VAR)) {
            statement->as.local_binding.is_inferred_type = true;
        } else if (!parser_parse_type(parser, &statement->as.local_binding.declared_type)) {
            ast_statement_free(statement);
            return NULL;
        }

        name_token = parser_current_token(parser);
        statement->as.local_binding.name =
            parser_consume_identifier(parser, "Expected local binding name.");
        if (!statement->as.local_binding.name) {
            ast_statement_free(statement);
            return NULL;
        }
        statement->as.local_binding.name_span = parser_source_span(name_token);

        if (!parser_consume(parser, TOK_ASSIGN, "Expected '=' after local binding name.")) {
            ast_statement_free(statement);
            return NULL;
        }

        statement->as.local_binding.initializer = parse_expression_node(parser);
        if (!statement->as.local_binding.initializer) {
            ast_statement_free(statement);
            return NULL;
        }

        if (!parser_consume(parser, TOK_SEMICOLON,
                            "Expected ';' after local binding statement.")) {
            ast_statement_free(statement);
            return NULL;
        }

        return statement;
    }

    {
        AstExpression *left = parse_expression_node(parser);
        AstStatement *statement;

        if (!left) {
            return NULL;
        }

        if (parser_match(parser, TOK_SWAP)) {
            statement = ast_statement_new(AST_STMT_SWAP);
            if (!statement) {
                parser_set_oom_error(parser);
                ast_expression_free(left);
                return NULL;
            }

            statement->source_span = left->source_span;
            statement->as.swap.left = left;
            statement->as.swap.right = parse_expression_node(parser);
            if (!statement->as.swap.right) {
                ast_statement_free(statement);
                return NULL;
            }

            if (!parser_consume(parser, TOK_SEMICOLON, "Expected ';' after swap statement.")) {
                ast_statement_free(statement);
                return NULL;
            }

            return statement;
        }

        statement = ast_statement_new(AST_STMT_EXPRESSION);
        if (!statement) {
            parser_set_oom_error(parser);
            ast_expression_free(left);
            return NULL;
        }

        statement->as.expression = left;
        statement->source_span = left->source_span;

        if (!parser_consume(parser, TOK_SEMICOLON, "Expected ';' after expression statement.")) {
            ast_statement_free(statement);
            return NULL;
        }

        return statement;
    }
}
