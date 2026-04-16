#ifndef CALYNDA_AST_INTERNAL_H
#define CALYNDA_AST_INTERNAL_H

#include "ast.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Shared dynamic array growth helper. */
bool ast_reserve_items(void **items, size_t *capacity,
                       size_t needed, size_t item_size);

/* Internal free helpers used across split files. */
void ast_expression_list_free_internal(AstExpressionList *list);
void ast_literal_free_internal(AstLiteral *literal);
void ast_lambda_body_init_internal(AstLambdaBody *body);
void ast_lambda_body_free_internal(AstLambdaBody *body);
void ast_binding_decl_free_fields_internal(AstBindingDecl *decl);
void ast_type_alias_decl_free_fields_internal(AstTypeAliasDecl *decl);
void ast_local_binding_free_fields_internal(AstLocalBindingStatement *binding);

#endif
