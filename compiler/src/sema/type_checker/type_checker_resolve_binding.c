#include "type_checker_internal.h"

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
        resolved_info = tc_type_check_info_make_external_value();
        break;

    case SYMBOL_KIND_IMPORT:
        if (symbol->declared_type) {
            resolved_info = tc_type_check_info_make(
                tc_checked_type_from_ast_type(checker, symbol->declared_type));
            if (checker->has_error) {
                entry->is_resolving = false;
                return NULL;
            }
        } else {
            resolved_info = tc_type_check_info_make_external_value();
        }
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
        {
            CheckedType generic_arg_type;

            if (tc_type_check_info_first_generic_arg(checker,
                                                     NULL,
                                                     symbol->declared_type,
                                                     &generic_arg_type)) {
                tc_type_check_info_set_first_generic_arg(&resolved_info, generic_arg_type);
            }
            if (checker->has_error) {
                entry->is_resolving = false;
                return NULL;
            }
        }
        break;

    case SYMBOL_KIND_TOP_LEVEL_BINDING:
        {
            const AstBindingDecl *binding_decl =
                (const AstBindingDecl *)symbol->declaration;
            bool seeded_provisional_callable = false;

            if (!binding_decl->is_inferred_type &&
                binding_decl->initializer &&
                binding_decl->initializer->kind == AST_EXPR_LAMBDA) {
                CheckedType provisional_return_type = tc_checked_type_from_ast_type(
                    checker, &binding_decl->declared_type);

                if (checker->has_error) {
                    entry->is_resolving = false;
                    return NULL;
                }

                entry->info = tc_type_check_info_make_callable(
                    provisional_return_type,
                    &binding_decl->initializer->as.lambda.parameters);
                {
                    CheckedType generic_arg_type;

                    if (tc_type_check_info_first_generic_arg(checker,
                                                             NULL,
                                                             &binding_decl->declared_type,
                                                             &generic_arg_type)) {
                        tc_type_check_info_set_first_generic_arg(&entry->info,
                                                                 generic_arg_type);
                    }
                    if (checker->has_error) {
                        entry->info = tc_type_check_info_make(tc_checked_type_invalid());
                        entry->is_resolving = false;
                        return NULL;
                    }
                }

                /*
                 * Allow recursive references from within the lambda body to observe
                 * the callable signature while the initializer is still being
                 * checked. Non-lambda initializers still take the normal circular
                 * definition path.
                 */
                entry->is_resolved = true;
                seeded_provisional_callable = true;
            }

        if (!tc_resolve_binding_symbol(checker,
                                       symbol,
                                       &binding_decl->declared_type,
                                       binding_decl->is_inferred_type,
                                       binding_decl->initializer,
                                       &resolved_info)) {
            if (seeded_provisional_callable) {
                entry->info = tc_type_check_info_make(tc_checked_type_invalid());
                entry->is_resolved = false;
            }
            entry->is_resolving = false;
            return NULL;
        }
        }
        break;

    case SYMBOL_KIND_ASM_BINDING: {
        const AstAsmDecl *asm_decl = (const AstAsmDecl *)symbol->declaration;
        const AstType *return_type_ast = NULL;
        const AstParameterList *parameters = NULL;
        CheckedType return_type;

        if (symbol->has_external_return_type) {
            return_type_ast = &symbol->external_return_type;
            parameters = &symbol->external_parameters;
        } else if (asm_decl) {
            return_type_ast = &asm_decl->return_type;
            parameters = &asm_decl->parameters;
        }
        if (!return_type_ast || !parameters) {
            tc_set_error_at(checker, symbol->declaration_span, NULL,
                            "Missing callable metadata for asm binding '%s'.",
                            symbol->name ? symbol->name : "<anonymous>");
            entry->is_resolving = false;
            return NULL;
        }
        return_type = tc_checked_type_from_ast_type(checker, return_type_ast);
        if (checker->has_error) {
            entry->is_resolving = false;
            return NULL;
        }
        resolved_info = tc_type_check_info_make_callable(return_type,
                                                         parameters);
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

    case SYMBOL_KIND_TYPE_ALIAS:
        resolved_info = tc_type_check_info_make(
            tc_checked_type_from_ast_type(checker, symbol->declared_type));
        if (checker->has_error) {
            entry->is_resolving = false;
            return NULL;
        }
        {
            CheckedType generic_arg_type;

            if (tc_type_check_info_first_generic_arg(checker,
                                                     NULL,
                                                     symbol->declared_type,
                                                     &generic_arg_type)) {
                tc_type_check_info_set_first_generic_arg(&resolved_info, generic_arg_type);
            }
            if (checker->has_error) {
                entry->is_resolving = false;
                return NULL;
            }
        }
        break;

    case SYMBOL_KIND_LAYOUT:
        resolved_info = tc_type_check_info_make(
            tc_checked_type_named(symbol->name, 0, 0));
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
    CheckedType target_generic_arg;
    CheckedType source_generic_arg;
    bool has_target_generic_arg = false;
    bool has_source_generic_arg = false;

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
    has_target_generic_arg = tc_type_check_info_first_generic_arg(checker,
                                                                  NULL,
                                                                  declared_type,
                                                                  &target_generic_arg);
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
    has_source_generic_arg = tc_type_check_info_first_generic_arg(checker,
                                                                  initializer_info,
                                                                  NULL,
                                                                  &source_generic_arg);
    if (checker->has_error) {
        return false;
    }
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

    if (has_target_generic_arg && has_source_generic_arg &&
        !tc_checked_type_assignable(target_generic_arg, source_generic_arg)) {
        char source_generic_text[64];
        char target_generic_text[64];

        checked_type_to_string(source_generic_arg,
                               source_generic_text,
                               sizeof(source_generic_text));
        checked_type_to_string(target_generic_arg,
                               target_generic_text,
                               sizeof(target_generic_text));
        tc_set_error_at(checker,
                        initializer ? initializer->source_span : symbol->declaration_span,
                        &symbol->declaration_span,
                        "Cannot assign expression with generic value type %s to %s '%s' expecting %s.",
                        source_generic_text,
                        symbol_kind_name(symbol->kind),
                        symbol->name ? symbol->name : "<anonymous>",
                        target_generic_text);
        return false;
    }

    *info = *initializer_info;
    info->type = target_type;
    if (has_target_generic_arg) {
        tc_type_check_info_set_first_generic_arg(info, target_generic_arg);
    }
    if (initializer_info->is_callable) {
        info->callable_return_type = target_type;
    }

    return true;
}
