#include "ast.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static bool reserve_items(void **items, size_t *capacity,
                          size_t needed, size_t item_size) {
    size_t new_capacity;
    void *resized;

    if (needed <= *capacity) {
        return true;
    }

    new_capacity = (*capacity == 0) ? 4 : *capacity;
    while (new_capacity < needed) {
        if (new_capacity > SIZE_MAX / 2) {
            new_capacity = needed;
            break;
        }
        new_capacity *= 2;
    }

    if (item_size != 0 && new_capacity > SIZE_MAX / item_size) {
        return false;
    }

    resized = realloc(*items, new_capacity * item_size);
    if (!resized) {
        return false;
    }

    *items = resized;
    *capacity = new_capacity;
    return true;
}

static void ast_expression_list_free(AstExpressionList *list);
static void ast_literal_free(AstLiteral *literal);
static void ast_lambda_body_init(AstLambdaBody *body);
static void ast_lambda_body_free(AstLambdaBody *body);
static void ast_binding_decl_free_fields(AstBindingDecl *decl);
static void ast_local_binding_free_fields(AstLocalBindingStatement *binding);

char *ast_copy_text(const char *text) {
    if (!text) {
        return NULL;
    }
    return ast_copy_text_n(text, strlen(text));
}

char *ast_copy_text_n(const char *text, size_t length) {
    char *copy;

    if (!text || length == SIZE_MAX) {
        return NULL;
    }

    copy = malloc(length + 1);
    if (!copy) {
        return NULL;
    }

    if (length > 0) {
        memcpy(copy, text, length);
    }
    copy[length] = '\0';
    return copy;
}

void ast_qualified_name_init(AstQualifiedName *name) {
    if (!name) {
        return;
    }
    memset(name, 0, sizeof(*name));
}

void ast_qualified_name_free(AstQualifiedName *name) {
    size_t i;

    if (!name) {
        return;
    }

    for (i = 0; i < name->count; i++) {
        free(name->segments[i]);
    }

    free(name->segments);
    memset(name, 0, sizeof(*name));
}

bool ast_qualified_name_append(AstQualifiedName *name, const char *segment) {
    char *copy;

    if (!name || !segment) {
        return false;
    }

    copy = ast_copy_text(segment);
    if (!copy) {
        return false;
    }

    if (!reserve_items((void **)&name->segments, &name->capacity,
                       name->count + 1, sizeof(*name->segments))) {
        free(copy);
        return false;
    }

    name->segments[name->count++] = copy;
    return true;
}

void ast_type_init_void(AstType *type) {
    if (!type) {
        return;
    }
    memset(type, 0, sizeof(*type));
    type->kind = AST_TYPE_VOID;
}

void ast_type_init_primitive(AstType *type, AstPrimitiveType primitive) {
    if (!type) {
        return;
    }
    memset(type, 0, sizeof(*type));
    type->kind = AST_TYPE_PRIMITIVE;
    type->primitive = primitive;
}

void ast_type_free(AstType *type) {
    size_t i;

    if (!type) {
        return;
    }

    for (i = 0; i < type->dimension_count; i++) {
        free(type->dimensions[i].size_literal);
    }

    free(type->dimensions);
    memset(type, 0, sizeof(*type));
}

bool ast_type_add_dimension(AstType *type, bool has_size, const char *size_literal) {
    AstArrayDimension dimension;

    if (!type) {
        return false;
    }

    memset(&dimension, 0, sizeof(dimension));
    dimension.has_size = has_size;
    if (has_size) {
        if (!size_literal) {
            return false;
        }

        dimension.size_literal = ast_copy_text(size_literal);
        if (!dimension.size_literal) {
            return false;
        }
    }

    if (!reserve_items((void **)&type->dimensions, &type->dimension_capacity,
                       type->dimension_count + 1, sizeof(*type->dimensions))) {
        free(dimension.size_literal);
        return false;
    }

    type->dimensions[type->dimension_count++] = dimension;
    return true;
}

void ast_parameter_list_init(AstParameterList *list) {
    if (!list) {
        return;
    }
    memset(list, 0, sizeof(*list));
}

void ast_parameter_list_free(AstParameterList *list) {
    size_t i;

    if (!list) {
        return;
    }

    for (i = 0; i < list->count; i++) {
        ast_type_free(&list->items[i].type);
        free(list->items[i].name);
    }

    free(list->items);
    memset(list, 0, sizeof(*list));
}

bool ast_parameter_list_append(AstParameterList *list, AstParameter *parameter) {
    if (!list || !parameter) {
        return false;
    }

    if (!reserve_items((void **)&list->items, &list->capacity,
                       list->count + 1, sizeof(*list->items))) {
        return false;
    }

    list->items[list->count++] = *parameter;
    memset(parameter, 0, sizeof(*parameter));
    return true;
}

AstBlock *ast_block_new(void) {
    return calloc(1, sizeof(AstBlock));
}

void ast_block_free(AstBlock *block) {
    size_t i;

    if (!block) {
        return;
    }

    for (i = 0; i < block->statement_count; i++) {
        ast_statement_free(block->statements[i]);
    }

    free(block->statements);
    free(block);
}

bool ast_block_append_statement(AstBlock *block, AstStatement *statement) {
    if (!block || !statement) {
        return false;
    }

    if (!reserve_items((void **)&block->statements, &block->statement_capacity,
                       block->statement_count + 1, sizeof(*block->statements))) {
        return false;
    }

    block->statements[block->statement_count++] = statement;
    return true;
}

static void ast_expression_list_free(AstExpressionList *list) {
    size_t i;

    if (!list) {
        return;
    }

    for (i = 0; i < list->count; i++) {
        ast_expression_free(list->items[i]);
    }

    free(list->items);
    memset(list, 0, sizeof(*list));
}

bool ast_expression_list_append(AstExpressionList *list, AstExpression *expression) {
    if (!list || !expression) {
        return false;
    }

    if (!reserve_items((void **)&list->items, &list->capacity,
                       list->count + 1, sizeof(*list->items))) {
        return false;
    }

    list->items[list->count++] = expression;
    return true;
}

bool ast_template_literal_append_text(AstLiteral *literal, const char *text) {
    AstTemplatePart part;

    if (!literal || literal->kind != AST_LITERAL_TEMPLATE || !text) {
        return false;
    }

    memset(&part, 0, sizeof(part));
    part.kind = AST_TEMPLATE_PART_TEXT;
    part.as.text = ast_copy_text(text);
    if (!part.as.text) {
        return false;
    }

    if (!reserve_items((void **)&literal->as.template_parts.items,
                       &literal->as.template_parts.capacity,
                       literal->as.template_parts.count + 1,
                       sizeof(*literal->as.template_parts.items))) {
        free(part.as.text);
        return false;
    }

    literal->as.template_parts.items[literal->as.template_parts.count++] = part;
    return true;
}

bool ast_template_literal_append_expression(AstLiteral *literal,
                                            AstExpression *expression) {
    AstTemplatePart part;

    if (!literal || literal->kind != AST_LITERAL_TEMPLATE || !expression) {
        return false;
    }

    memset(&part, 0, sizeof(part));
    part.kind = AST_TEMPLATE_PART_EXPRESSION;
    part.as.expression = expression;

    if (!reserve_items((void **)&literal->as.template_parts.items,
                       &literal->as.template_parts.capacity,
                       literal->as.template_parts.count + 1,
                       sizeof(*literal->as.template_parts.items))) {
        return false;
    }

    literal->as.template_parts.items[literal->as.template_parts.count++] = part;
    return true;
}

static void ast_literal_free(AstLiteral *literal) {
    size_t i;

    if (!literal) {
        return;
    }

    switch (literal->kind) {
    case AST_LITERAL_INTEGER:
    case AST_LITERAL_FLOAT:
    case AST_LITERAL_CHAR:
    case AST_LITERAL_STRING:
        free(literal->as.text);
        break;
    case AST_LITERAL_TEMPLATE:
        for (i = 0; i < literal->as.template_parts.count; i++) {
            AstTemplatePart *part = &literal->as.template_parts.items[i];
            if (part->kind == AST_TEMPLATE_PART_TEXT) {
                free(part->as.text);
            } else {
                ast_expression_free(part->as.expression);
            }
        }
        free(literal->as.template_parts.items);
        break;
    case AST_LITERAL_BOOL:
    case AST_LITERAL_NULL:
        break;
    }

    memset(literal, 0, sizeof(*literal));
    literal->kind = AST_LITERAL_NULL;
}

static void ast_lambda_body_init(AstLambdaBody *body) {
    if (!body) {
        return;
    }
    memset(body, 0, sizeof(*body));
    body->kind = AST_LAMBDA_BODY_EXPRESSION;
}

static void ast_lambda_body_free(AstLambdaBody *body) {
    if (!body) {
        return;
    }

    if (body->kind == AST_LAMBDA_BODY_BLOCK) {
        ast_block_free(body->as.block);
    } else {
        ast_expression_free(body->as.expression);
    }

    ast_lambda_body_init(body);
}

AstExpression *ast_expression_new(AstExpressionKind kind) {
    AstExpression *expression = calloc(1, sizeof(*expression));

    if (!expression) {
        return NULL;
    }

    expression->kind = kind;
    if (kind == AST_EXPR_LITERAL) {
        expression->as.literal.kind = AST_LITERAL_NULL;
    } else if (kind == AST_EXPR_LAMBDA) {
        ast_parameter_list_init(&expression->as.lambda.parameters);
        ast_lambda_body_init(&expression->as.lambda.body);
    }

    return expression;
}

void ast_expression_free(AstExpression *expression) {
    if (!expression) {
        return;
    }

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        ast_literal_free(&expression->as.literal);
        break;
    case AST_EXPR_IDENTIFIER:
        free(expression->as.identifier);
        break;
    case AST_EXPR_LAMBDA:
        ast_parameter_list_free(&expression->as.lambda.parameters);
        ast_lambda_body_free(&expression->as.lambda.body);
        break;
    case AST_EXPR_ASSIGNMENT:
        ast_expression_free(expression->as.assignment.target);
        ast_expression_free(expression->as.assignment.value);
        break;
    case AST_EXPR_TERNARY:
        ast_expression_free(expression->as.ternary.condition);
        ast_expression_free(expression->as.ternary.then_branch);
        ast_expression_free(expression->as.ternary.else_branch);
        break;
    case AST_EXPR_BINARY:
        ast_expression_free(expression->as.binary.left);
        ast_expression_free(expression->as.binary.right);
        break;
    case AST_EXPR_UNARY:
        ast_expression_free(expression->as.unary.operand);
        break;
    case AST_EXPR_CALL:
        ast_expression_free(expression->as.call.callee);
        ast_expression_list_free(&expression->as.call.arguments);
        break;
    case AST_EXPR_INDEX:
        ast_expression_free(expression->as.index.target);
        ast_expression_free(expression->as.index.index);
        break;
    case AST_EXPR_MEMBER:
        ast_expression_free(expression->as.member.target);
        free(expression->as.member.member);
        break;
    case AST_EXPR_CAST:
        ast_expression_free(expression->as.cast.expression);
        break;
    case AST_EXPR_ARRAY_LITERAL:
        ast_expression_list_free(&expression->as.array_literal.elements);
        break;
    case AST_EXPR_GROUPING:
        ast_expression_free(expression->as.grouping.inner);
        break;
    }

    free(expression);
}

static void ast_local_binding_free_fields(AstLocalBindingStatement *binding) {
    if (!binding) {
        return;
    }

    ast_type_free(&binding->declared_type);
    free(binding->name);
    ast_expression_free(binding->initializer);
    memset(binding, 0, sizeof(*binding));
}

AstStatement *ast_statement_new(AstStatementKind kind) {
    AstStatement *statement = calloc(1, sizeof(*statement));

    if (!statement) {
        return NULL;
    }

    statement->kind = kind;
    if (kind == AST_STMT_LOCAL_BINDING) {
        ast_type_init_void(&statement->as.local_binding.declared_type);
    }

    return statement;
}

void ast_statement_free(AstStatement *statement) {
    if (!statement) {
        return;
    }

    switch (statement->kind) {
    case AST_STMT_LOCAL_BINDING:
        ast_local_binding_free_fields(&statement->as.local_binding);
        break;
    case AST_STMT_RETURN:
        ast_expression_free(statement->as.return_expression);
        break;
    case AST_STMT_EXIT:
        break;
    case AST_STMT_THROW:
        ast_expression_free(statement->as.throw_expression);
        break;
    case AST_STMT_EXPRESSION:
        ast_expression_free(statement->as.expression);
        break;
    }

    free(statement);
}

static void ast_binding_decl_free_fields(AstBindingDecl *decl) {
    if (!decl) {
        return;
    }

    free(decl->modifiers);
    ast_type_free(&decl->declared_type);
    free(decl->name);
    ast_expression_free(decl->initializer);
    memset(decl, 0, sizeof(*decl));
}

AstTopLevelDecl *ast_top_level_decl_new(AstTopLevelDeclKind kind) {
    AstTopLevelDecl *decl = calloc(1, sizeof(*decl));

    if (!decl) {
        return NULL;
    }

    decl->kind = kind;
    if (kind == AST_TOP_LEVEL_START) {
        ast_parameter_list_init(&decl->as.start_decl.parameters);
        ast_lambda_body_init(&decl->as.start_decl.body);
    } else if (kind == AST_TOP_LEVEL_BINDING) {
        ast_type_init_void(&decl->as.binding_decl.declared_type);
    }

    return decl;
}

void ast_top_level_decl_free(AstTopLevelDecl *decl) {
    if (!decl) {
        return;
    }

    switch (decl->kind) {
    case AST_TOP_LEVEL_START:
        ast_parameter_list_free(&decl->as.start_decl.parameters);
        ast_lambda_body_free(&decl->as.start_decl.body);
        break;
    case AST_TOP_LEVEL_BINDING:
        ast_binding_decl_free_fields(&decl->as.binding_decl);
        break;
    }

    free(decl);
}

bool ast_binding_decl_add_modifier(AstBindingDecl *decl, AstModifier modifier) {
    if (!decl) {
        return false;
    }

    if (!reserve_items((void **)&decl->modifiers, &decl->modifier_capacity,
                       decl->modifier_count + 1, sizeof(*decl->modifiers))) {
        return false;
    }

    decl->modifiers[decl->modifier_count++] = modifier;
    return true;
}

void ast_program_init(AstProgram *program) {
    if (!program) {
        return;
    }
    memset(program, 0, sizeof(*program));
}

void ast_program_free(AstProgram *program) {
    size_t i;

    if (!program) {
        return;
    }

    ast_qualified_name_free(&program->package_name);

    for (i = 0; i < program->import_count; i++) {
        ast_qualified_name_free(&program->imports[i]);
    }
    free(program->imports);

    for (i = 0; i < program->top_level_count; i++) {
        ast_top_level_decl_free(program->top_level_decls[i]);
    }
    free(program->top_level_decls);

    memset(program, 0, sizeof(*program));
}

bool ast_program_set_package(AstProgram *program, AstQualifiedName *package_name) {
    if (!program || !package_name) {
        return false;
    }

    ast_qualified_name_free(&program->package_name);
    program->package_name = *package_name;
    program->has_package = true;
    ast_qualified_name_init(package_name);
    return true;
}

bool ast_program_add_import(AstProgram *program, AstQualifiedName *import_name) {
    if (!program || !import_name) {
        return false;
    }

    if (!reserve_items((void **)&program->imports, &program->import_capacity,
                       program->import_count + 1, sizeof(*program->imports))) {
        return false;
    }

    program->imports[program->import_count++] = *import_name;
    ast_qualified_name_init(import_name);
    return true;
}

bool ast_program_add_top_level_decl(AstProgram *program, AstTopLevelDecl *decl) {
    if (!program || !decl) {
        return false;
    }

    if (!reserve_items((void **)&program->top_level_decls,
                       &program->top_level_capacity,
                       program->top_level_count + 1,
                       sizeof(*program->top_level_decls))) {
        return false;
    }

    program->top_level_decls[program->top_level_count++] = decl;
    return true;
}