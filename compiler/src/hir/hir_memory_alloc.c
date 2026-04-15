#include "hir_internal.h"
#include <stdlib.h>
#include <string.h>

HirBlock *hr_block_new(void) {
    HirBlock *block = malloc(sizeof(*block));

    if (!block) {
        return NULL;
    }

    memset(block, 0, sizeof(*block));
    return block;
}

HirStatement *hr_statement_new(HirStatementKind kind) {
    HirStatement *statement = malloc(sizeof(*statement));

    if (!statement) {
        return NULL;
    }

    memset(statement, 0, sizeof(*statement));
    statement->kind = kind;
    return statement;
}

HirExpression *hr_expression_new(HirExpressionKind kind) {
    HirExpression *expression = malloc(sizeof(*expression));

    if (!expression) {
        return NULL;
    }

    memset(expression, 0, sizeof(*expression));
    expression->kind = kind;
    expression->type = hr_checked_type_void_value();
    expression->callable_signature.return_type = hr_checked_type_void_value();
    return expression;
}

HirTopLevelDecl *hr_top_level_decl_new(HirTopLevelDeclKind kind) {
    HirTopLevelDecl *decl = malloc(sizeof(*decl));

    if (!decl) {
        return NULL;
    }

    memset(decl, 0, sizeof(*decl));
    decl->kind = kind;
    return decl;
}
