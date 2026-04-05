#include "type_checker_internal.h"

bool tc_validate_program_start_decls(TypeChecker *checker,
                                     const AstProgram *program) {
    const AstStartDecl *first_start = NULL;
    const AstStartDecl *first_boot = NULL;
    const AstStartDecl *duplicate = NULL;
    size_t i;

    if (!checker || !program) {
        return false;
    }

    for (i = 0; i < program->top_level_count; i++) {
        const AstTopLevelDecl *decl = program->top_level_decls[i];

        if (!decl || decl->kind != AST_TOP_LEVEL_START) {
            continue;
        }

        if (decl->as.start_decl.is_boot) {
            if (!first_boot) {
                first_boot = &decl->as.start_decl;
            } else {
                duplicate = &decl->as.start_decl;
                break;
            }
        } else {
            if (!first_start) {
                first_start = &decl->as.start_decl;
            } else {
                duplicate = &decl->as.start_decl;
                break;
            }
        }
    }

    if (!first_start && !first_boot) {
        tc_set_error(checker,
                     "Program must declare exactly one start or boot entry point.");
        return false;
    }

    if (first_start && first_boot) {
        tc_set_error_at(checker,
                        first_boot->start_span,
                        &first_start->start_span,
                        "Program cannot declare both start and boot entry points.");
        return false;
    }

    if (duplicate) {
        const AstStartDecl *first = first_start ? first_start : first_boot;
        const char *kind = first_start ? "start" : "boot";
        tc_set_error_at(checker,
                        duplicate->start_span,
                        &first->start_span,
                        "Program cannot declare multiple %s entry points.",
                        kind);
        return false;
    }

    return true;
}

const TypeCheckInfo *tc_resolve_symbol_info(TypeChecker *checker,
                                            const Symbol *symbol) {
    TypeCheckSymbolEntry *entry;
    TypeCheckInfo resolved_info;

    if (!checker || !symbol) {
        return NULL;
    }

    entry = tc_ensure_symbol_entry(checker, symbol);
    if (!entry) {
        return NULL;
    }

    if (entry->is_resolved) {
        return &entry->info;
    }

    if (entry->is_resolving) {
        tc_set_error_at(checker, symbol->declaration_span, NULL,
                        "Circular definition involving '%s'.",
                        symbol->name ? symbol->name : "<anonymous>");
        return NULL;
    }

    entry->is_resolving = true;
    resolved_info = tc_type_check_info_make(tc_checked_type_invalid());

    switch (symbol->kind) {
    case SYMBOL_KIND_PACKAGE:
    case SYMBOL_KIND_IMPORT:
        resolved_info = tc_type_check_info_make_external_value();
        break;

    case SYMBOL_KIND_PARAMETER:
        resolved_info = tc_type_check_info_make(tc_checked_type_from_ast_type(checker,
                                                                              symbol->declared_type));
        if (checker->has_error) {
            entry->is_resolving = false;
            return NULL;
        }
        if (resolved_info.type.kind == CHECKED_TYPE_VOID) {
            tc_set_error_at(checker, symbol->declaration_span, NULL,
                            "Parameter '%s' cannot have type void.",
                            symbol->name ? symbol->name : "<anonymous>");
            entry->is_resolving = false;
            return NULL;
        }
        break;

    case SYMBOL_KIND_TOP_LEVEL_BINDING:
        if (!tc_resolve_binding_symbol(checker,
                                       symbol,
                                       &((const AstBindingDecl *)symbol->declaration)->declared_type,
                                       ((const AstBindingDecl *)symbol->declaration)->is_inferred_type,
                                       ((const AstBindingDecl *)symbol->declaration)->initializer,
                                       &resolved_info)) {
            entry->is_resolving = false;
            return NULL;
        }
        break;

    case SYMBOL_KIND_ASM_BINDING: {
        const AstAsmDecl *asm_decl = (const AstAsmDecl *)symbol->declaration;
        CheckedType return_type = tc_checked_type_from_ast_type(checker,
                                                                &asm_decl->return_type);
        if (checker->has_error) {
            entry->is_resolving = false;
            return NULL;
        }
        resolved_info = tc_type_check_info_make_callable(return_type,
                                                         &asm_decl->parameters);
        break;
    }

    case SYMBOL_KIND_LOCAL:
        if (!tc_resolve_binding_symbol(checker,
                                       symbol,
                                       &((const AstLocalBindingStatement *)symbol->declaration)->declared_type,
                                       ((const AstLocalBindingStatement *)symbol->declaration)->is_inferred_type,
                                       ((const AstLocalBindingStatement *)symbol->declaration)->initializer,
                                       &resolved_info)) {
            entry->is_resolving = false;
            return NULL;
        }
        break;

    case SYMBOL_KIND_UNION:
        resolved_info = tc_type_check_info_make(
            tc_checked_type_named(symbol->name, symbol->generic_param_count, 0));
        break;

    case SYMBOL_KIND_TYPE_PARAMETER:
        resolved_info = tc_type_check_info_make(tc_checked_type_type_param(symbol->name));
        break;

    case SYMBOL_KIND_VARIANT:
        break;
    }

    entry->info = resolved_info;
    entry->is_resolving = false;
    entry->is_resolved = true;
    return &entry->info;
}

bool tc_resolve_binding_symbol(TypeChecker *checker,
                               const Symbol *symbol,
                               const AstType *declared_type,
                               bool is_inferred_type,
                               const AstExpression *initializer,
                               TypeCheckInfo *info) {
    const TypeCheckInfo *initializer_info;
    CheckedType target_type;
    CheckedType source_type;
    char source_text[64];
    char target_text[64];

    if (is_inferred_type) {
        initializer_info = tc_check_expression(checker, initializer);
        if (!initializer_info) {
            return false;
        }

        if (!initializer_info->is_callable &&
            (initializer_info->type.kind == CHECKED_TYPE_VOID ||
             initializer_info->type.kind == CHECKED_TYPE_NULL)) {
            tc_set_error_at(checker,
                            initializer ? initializer->source_span : symbol->declaration_span,
                            &symbol->declaration_span,
                            "Cannot infer a type for %s '%s' from %s.",
                            symbol_kind_name(symbol->kind),
                            symbol->name ? symbol->name : "<anonymous>",
                            initializer_info->type.kind == CHECKED_TYPE_VOID
                                ? "a void expression"
                                : "null");
            return false;
        }

        *info = *initializer_info;
        return true;
    }

    target_type = tc_checked_type_from_ast_type(checker, declared_type);
    if (checker->has_error) {
        return false;
    }

    if (initializer && initializer->kind == AST_EXPR_LAMBDA) {
        initializer_info = tc_check_lambda_expression(checker,
                                                      initializer,
                                                      &target_type,
                                                      &symbol->declaration_span);
    } else if (tc_checked_type_is_hetero_array(target_type) &&
               initializer && initializer->kind == AST_EXPR_ARRAY_LITERAL) {
        initializer_info = tc_check_hetero_array_literal(checker, initializer);
    } else {
        initializer_info = tc_check_expression(checker, initializer);
    }
    if (!initializer_info) {
        return false;
    }

    if (target_type.kind == CHECKED_TYPE_VOID && !initializer_info->is_callable) {
        tc_set_error_at(checker, symbol->declaration_span, NULL,
                        "%s '%s' cannot have type void unless initialized with a callable expression.",
                        symbol_kind_name(symbol->kind),
                        symbol->name ? symbol->name : "<anonymous>");
        return false;
    }

    source_type = tc_type_check_source_type(initializer_info);
    if (!(initializer_info->is_callable && source_type.kind == CHECKED_TYPE_EXTERNAL) &&
        !tc_checked_type_assignable(target_type, source_type)) {
        checked_type_to_string(source_type, source_text, sizeof(source_text));
        checked_type_to_string(target_type, target_text, sizeof(target_text));
        tc_set_error_at(checker,
                        initializer ? initializer->source_span : symbol->declaration_span,
                        &symbol->declaration_span,
                        "Cannot assign expression of type %s to %s '%s' of type %s.",
                        source_text,
                        symbol_kind_name(symbol->kind),
                        symbol->name ? symbol->name : "<anonymous>",
                        target_text);
        return false;
    }

    *info = *initializer_info;
    info->type = target_type;
    if (initializer_info->is_callable) {
        info->callable_return_type = target_type;
    }

    return true;
}
