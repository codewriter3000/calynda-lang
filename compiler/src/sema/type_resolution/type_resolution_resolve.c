#include "type_resolution_internal.h"

static bool parse_array_size_literal(const char *text, unsigned long long *value_out) {
    char *end = NULL;
    unsigned long long value;

    if (!text || text[0] == '\0') {
        return false;
    }

    errno = 0;
    value = strtoull(text, &end, 10);
    if (errno != 0 || !end || *end != '\0') {
        return false;
    }

    if (value_out) {
        *value_out = value;
    }
    return true;
}

static bool validate_array_dimensions(TypeResolver *resolver,
                                      const AstType *type,
                                      AstSourceSpan primary_span,
                                      const char *subject_kind,
                                      const char *subject_name) {
    size_t i;

    if (!type) {
        return false;
    }

    if (type->kind == AST_TYPE_VOID && type->dimension_count > 0) {
        tr_set_error_at(resolver,
                        primary_span,
                        NULL,
                        "%s '%s' cannot declare arrays of void.",
                        subject_kind,
                        subject_name ? subject_name : "<anonymous>");
        return false;
    }

    for (i = 0; i < type->dimension_count; i++) {
        if (type->dimensions[i].has_size) {
            unsigned long long size_value = 0;

            if (!parse_array_size_literal(type->dimensions[i].size_literal, &size_value) ||
                size_value == 0) {
                tr_set_error_at(resolver,
                                primary_span,
                                NULL,
                                "%s '%s' has invalid array dimension '%s'; sizes must be positive integers.",
                                subject_kind,
                                subject_name ? subject_name : "<anonymous>",
                                type->dimensions[i].size_literal
                                    ? type->dimensions[i].size_literal
                                    : "<missing>");
                return false;
            }
        }
    }

    return true;
}

bool tr_resolve_declared_type(TypeResolver *resolver,
                              const AstType *type,
                              AstSourceSpan primary_span,
                              const char *subject_kind,
                              const char *subject_name,
                              bool allow_void) {
    ResolvedType resolved_type;

    if (!type) {
        tr_set_error(resolver, "Internal error: missing type to resolve.");
        return false;
    }

    if (!validate_array_dimensions(resolver, type, primary_span, subject_kind, subject_name)) {
        return false;
    }

    if (type->kind == AST_TYPE_VOID) {
        if (!allow_void) {
            tr_set_error_at(resolver,
                            primary_span,
                            NULL,
                            "%s '%s' cannot have type void.",
                            subject_kind,
                            subject_name ? subject_name : "<anonymous>");
            return false;
        }

        resolved_type = tr_resolved_type_void();
    } else if (type->kind == AST_TYPE_NAMED) {
        resolved_type = tr_resolved_type_named(type->name,
                                               type->generic_args.count,
                                               type->dimension_count);
    } else if (type->kind == AST_TYPE_ARR) {
        resolved_type = tr_resolved_type_named("arr",
                                               type->generic_args.count,
                                               0);
    } else if (type->kind == AST_TYPE_PTR) {
        resolved_type = tr_resolved_type_named("ptr",
                                               type->generic_args.count,
                                               0);
    } else {
        resolved_type = tr_resolved_type_value(type->primitive, type->dimension_count);
    }

    return tr_append_type_entry(resolver, type, resolved_type);
}

static bool resolve_statement(TypeResolver *resolver, const AstStatement *statement);

bool tr_resolve_parameter(TypeResolver *resolver, const AstParameter *parameter) {
    if (!parameter) {
        return true;
    }

    return tr_resolve_declared_type(resolver,
                                    &parameter->type,
                                    parameter->name_span,
                                    "Parameter",
                                    parameter->name,
                                    false);
}

bool tr_resolve_binding_decl(TypeResolver *resolver, const AstBindingDecl *binding_decl) {
    if (!binding_decl) {
        return true;
    }

    if (binding_decl->is_inferred_type) {
        return true;
    }

    return tr_resolve_declared_type(resolver,
                                    &binding_decl->declared_type,
                                    binding_decl->name_span,
                                    "Top-level binding",
                                    binding_decl->name,
                                    true);
}

bool tr_resolve_start_decl(TypeResolver *resolver, const AstStartDecl *start_decl) {
    size_t i;

    if (!start_decl) {
        return true;
    }

    for (i = 0; i < start_decl->parameters.count; i++) {
        if (!tr_resolve_parameter(resolver, &start_decl->parameters.items[i])) {
            return false;
        }
    }

    if (start_decl->body.kind == AST_LAMBDA_BODY_BLOCK) {
        return tr_resolve_block(resolver, start_decl->body.as.block);
    }

    return tr_resolve_expression(resolver, start_decl->body.as.expression);
}

bool tr_resolve_block(TypeResolver *resolver, const AstBlock *block) {
    size_t i;

    if (!block) {
        return true;
    }

    for (i = 0; i < block->statement_count; i++) {
        if (!resolve_statement(resolver, block->statements[i])) {
            return false;
        }
    }

    return true;
}

static bool resolve_statement(TypeResolver *resolver, const AstStatement *statement) {
    if (!statement) {
        return true;
    }

    switch (statement->kind) {
    case AST_STMT_LOCAL_BINDING:
        if (!statement->as.local_binding.is_inferred_type &&
            !tr_resolve_declared_type(resolver,
                                      &statement->as.local_binding.declared_type,
                                      statement->as.local_binding.name_span,
                                      "Local",
                                      statement->as.local_binding.name,
                                      true)) {
            return false;
        }
        return tr_resolve_expression(resolver, statement->as.local_binding.initializer);

    case AST_STMT_RETURN:
        return tr_resolve_expression(resolver, statement->as.return_expression);

    case AST_STMT_EXIT:
        return true;

    case AST_STMT_THROW:
        return tr_resolve_expression(resolver, statement->as.throw_expression);

    case AST_STMT_EXPRESSION:
        return tr_resolve_expression(resolver, statement->as.expression);

    case AST_STMT_MANUAL:
        if (statement->as.manual.body) {
            return tr_resolve_block(resolver, statement->as.manual.body);
        }
        return true;
    }

    return false;
}
