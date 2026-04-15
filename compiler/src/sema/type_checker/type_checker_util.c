#include "type_checker_internal.h"

void tc_set_error(TypeChecker *checker, const char *format, ...) {
    va_list args;

    if (!checker || checker->has_error) {
        return;
    }

    checker->has_error = true;
    va_start(args, format);
    vsnprintf(checker->error.message, sizeof(checker->error.message), format, args);
    va_end(args);
}

void tc_set_error_at(TypeChecker *checker,
                     AstSourceSpan primary_span,
                     const AstSourceSpan *related_span,
                     const char *format, ...) {
    va_list args;

    if (!checker || checker->has_error) {
        return;
    }

    checker->has_error = true;
    checker->error.primary_span = primary_span;
    if (related_span && tc_source_span_is_valid(*related_span)) {
        checker->error.related_span = *related_span;
        checker->error.has_related_span = true;
    }

    va_start(args, format);
    vsnprintf(checker->error.message, sizeof(checker->error.message), format, args);
    va_end(args);
}

TypeCheckExpressionEntry *tc_ensure_expression_entry(TypeChecker *checker,
                                                     const AstExpression *expression) {
    size_t i;

    for (i = 0; i < checker->expression_count; i++) {
        if (checker->expression_entries[i].expression == expression) {
            return &checker->expression_entries[i];
        }
    }

    if (!tc_reserve_items((void **)&checker->expression_entries,
                          &checker->expression_capacity,
                          checker->expression_count + 1,
                          sizeof(*checker->expression_entries))) {
        tc_set_error(checker,
                     "Out of memory while storing expression type information.");
        return NULL;
    }

    checker->expression_entries[checker->expression_count].expression = expression;
    memset(&checker->expression_entries[checker->expression_count].info, 0,
           sizeof(checker->expression_entries[checker->expression_count].info));
    checker->expression_count++;
    return &checker->expression_entries[checker->expression_count - 1];
}

TypeCheckSymbolEntry *tc_ensure_symbol_entry(TypeChecker *checker,
                                             const Symbol *symbol) {
    size_t i;

    for (i = 0; i < checker->symbol_count; i++) {
        if (checker->symbol_entries[i].symbol == symbol) {
            return &checker->symbol_entries[i];
        }
    }

    if (!tc_reserve_items((void **)&checker->symbol_entries,
                          &checker->symbol_capacity,
                          checker->symbol_count + 1,
                          sizeof(*checker->symbol_entries))) {
        tc_set_error(checker,
                     "Out of memory while storing symbol type information.");
        return NULL;
    }

    checker->symbol_entries[checker->symbol_count].symbol = symbol;
    memset(&checker->symbol_entries[checker->symbol_count].info, 0,
           sizeof(checker->symbol_entries[checker->symbol_count].info));
    checker->symbol_entries[checker->symbol_count].is_resolved = false;
    checker->symbol_entries[checker->symbol_count].is_resolving = false;
    checker->symbol_count++;
    return &checker->symbol_entries[checker->symbol_count - 1];
}

const TypeCheckInfo *tc_store_expression_info(TypeChecker *checker,
                                              const AstExpression *expression,
                                              TypeCheckInfo info) {
    TypeCheckExpressionEntry *entry = tc_ensure_expression_entry(checker, expression);

    if (!entry) {
        return NULL;
    }

    entry->info = info;
    return &entry->info;
}

bool tc_validate_binding_modifiers(TypeChecker *checker,
                                   const AstBindingDecl *binding) {
    bool has_export = ast_decl_has_modifier(binding->modifiers,
                                            binding->modifier_count,
                                            AST_MODIFIER_EXPORT);
    bool has_private = ast_decl_has_modifier(binding->modifiers,
                                             binding->modifier_count,
                                             AST_MODIFIER_PRIVATE);
    bool has_public = ast_decl_has_modifier(binding->modifiers,
                                            binding->modifier_count,
                                            AST_MODIFIER_PUBLIC);

    if (has_export && has_private) {
        tc_set_error_at(checker, binding->name_span, NULL,
                        "Binding '%s' cannot be both export and private.",
                        binding->name);
        return false;
    }

    if (has_public && has_private) {
        tc_set_error_at(checker, binding->name_span, NULL,
                        "Binding '%s' cannot be both public and private.",
                        binding->name);
        return false;
    }

    return true;
}

bool tc_validate_internal_access(TypeChecker *checker,
                                 const AstExpression *identifier,
                                 const Symbol *symbol) {
    const SymbolResolution *resolution;
    const Scope *walk;
    bool crossed_lambda = false;

    if (!symbol->is_internal) {
        return true;
    }

    resolution = symbol_table_find_resolution(checker->symbols, identifier);
    if (!resolution || !resolution->scope) {
        return true;
    }

    walk = resolution->scope;
    while (walk && walk != symbol->scope) {
        if (walk->kind == SCOPE_KIND_LAMBDA) {
            crossed_lambda = true;
            break;
        }
        walk = walk->parent;
    }

    if (!crossed_lambda) {
        tc_set_error_at(checker,
                        identifier->source_span,
                        &symbol->declaration_span,
                        "Internal binding '%s' can only be called from a nested lambda.",
                        symbol->name);
        return false;
    }

    return true;
}

const TypeCheckInfo *tc_check_hetero_array_literal(TypeChecker *checker,
                                                   const AstExpression *expression) {
    TypeCheckInfo info;
    size_t i;

    if (expression->as.array_literal.elements.count == 0) {
        tc_set_error_at(checker,
                        expression->source_span,
                        NULL,
                        "Cannot create an empty heterogeneous array literal.");
        return NULL;
    }

    for (i = 0; i < expression->as.array_literal.elements.count; i++) {
        const TypeCheckInfo *element_info = tc_check_expression(
            checker, expression->as.array_literal.elements.items[i]);
        CheckedType element_type;

        if (!element_info) {
            return NULL;
        }

        element_type = tc_type_check_source_type(element_info);
        if (element_type.kind == CHECKED_TYPE_VOID ||
            element_type.kind == CHECKED_TYPE_NULL) {
            tc_set_error_at(checker,
                            expression->as.array_literal.elements.items[i]->source_span,
                            NULL,
                            "Heterogeneous array elements cannot have type %s.",
                            element_type.kind == CHECKED_TYPE_VOID
                                ? "void" : "null");
            return NULL;
        }
    }

    info = tc_type_check_info_make(tc_checked_type_named("arr", 1, 0));
    return tc_store_expression_info(checker, expression, info);
}
