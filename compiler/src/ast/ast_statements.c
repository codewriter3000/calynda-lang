#include "ast_internal.h"

bool ast_parameter_list_append(AstParameterList *list, AstParameter *parameter) {
    if (!list || !parameter) {
        return false;
    }

    if (!ast_reserve_items((void **)&list->items, &list->capacity,
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

void ast_local_binding_free_fields_internal(AstLocalBindingStatement *binding) {
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
        ast_local_binding_free_fields_internal(&statement->as.local_binding);
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
    case AST_STMT_MANUAL:
        ast_parameter_list_free(&statement->as.manual.parameters);
        ast_block_free(statement->as.manual.body);
        break;
    }

    free(statement);
}

void ast_binding_decl_free_fields_internal(AstBindingDecl *decl) {
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
        ast_lambda_body_init_internal(&decl->as.start_decl.body);
    } else if (kind == AST_TOP_LEVEL_BINDING) {
        ast_type_init_void(&decl->as.binding_decl.declared_type);
    } else if (kind == AST_TOP_LEVEL_UNION) {
        memset(&decl->as.union_decl, 0, sizeof(decl->as.union_decl));
    } else if (kind == AST_TOP_LEVEL_ASM) {
        ast_type_init_void(&decl->as.asm_decl.return_type);
        ast_parameter_list_init(&decl->as.asm_decl.parameters);
    } else if (kind == AST_TOP_LEVEL_LAYOUT) {
        memset(&decl->as.layout_decl, 0, sizeof(decl->as.layout_decl));
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
        ast_lambda_body_free_internal(&decl->as.start_decl.body);
        break;
    case AST_TOP_LEVEL_BINDING:
        ast_binding_decl_free_fields_internal(&decl->as.binding_decl);
        break;
    case AST_TOP_LEVEL_UNION:
        ast_union_decl_free_fields(&decl->as.union_decl);
        break;
    case AST_TOP_LEVEL_ASM:
        free(decl->as.asm_decl.modifiers);
        ast_type_free(&decl->as.asm_decl.return_type);
        free(decl->as.asm_decl.name);
        ast_parameter_list_free(&decl->as.asm_decl.parameters);
        free(decl->as.asm_decl.body);
        memset(&decl->as.asm_decl, 0, sizeof(decl->as.asm_decl));
        break;
    case AST_TOP_LEVEL_LAYOUT: {
        size_t fi;
        for (fi = 0; fi < decl->as.layout_decl.field_count; fi++) {
            ast_type_free(&decl->as.layout_decl.fields[fi].field_type);
            free(decl->as.layout_decl.fields[fi].name);
        }
        free(decl->as.layout_decl.fields);
        free(decl->as.layout_decl.name);
        memset(&decl->as.layout_decl, 0, sizeof(decl->as.layout_decl));
        break;
    }
    }

    free(decl);
}

bool ast_binding_decl_add_modifier(AstBindingDecl *decl, AstModifier modifier) {
    if (!decl) {
        return false;
    }

    if (!ast_reserve_items((void **)&decl->modifiers, &decl->modifier_capacity,
                           decl->modifier_count + 1, sizeof(*decl->modifiers))) {
        return false;
    }

    decl->modifiers[decl->modifier_count++] = modifier;
    return true;
}

bool ast_asm_decl_add_modifier(AstAsmDecl *decl, AstModifier modifier) {
    if (!decl) {
        return false;
    }

    if (!ast_reserve_items((void **)&decl->modifiers, &decl->modifier_capacity,
                           decl->modifier_count + 1, sizeof(*decl->modifiers))) {
        return false;
    }

    decl->modifiers[decl->modifier_count++] = modifier;
    return true;
}

bool ast_union_decl_add_modifier(AstUnionDecl *decl, AstModifier modifier) {
    if (!decl) {
        return false;
    }

    if (!ast_reserve_items((void **)&decl->modifiers, &decl->modifier_capacity,
                           decl->modifier_count + 1, sizeof(*decl->modifiers))) {
        return false;
    }

    decl->modifiers[decl->modifier_count++] = modifier;
    return true;
}

bool ast_decl_has_modifier(const AstModifier *modifiers, size_t count,
                           AstModifier modifier) {
    size_t i;

    for (i = 0; i < count; i++) {
        if (modifiers[i] == modifier) {
            return true;
        }
    }

    return false;
}
