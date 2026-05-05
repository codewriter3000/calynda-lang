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
        if (symbol->is_untyped) {
            resolved_info = tc_type_check_info_make_external_value();
            break;
        }
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

#include "type_checker_resolve_binding_p2.inc"
