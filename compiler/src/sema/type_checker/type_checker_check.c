#include "type_checker_internal.h"

bool type_checker_check_program(TypeChecker *checker,
                                const AstProgram *program,
                                const SymbolTable *symbols) {
    const SymbolTableError *symbol_error;
    const UnresolvedIdentifier *unresolved;
    const TypeResolutionError *resolution_error;
    const Scope *root_scope;
    size_t i;

    if (!checker || !program || !symbols) {
        return false;
    }

    type_checker_free(checker);
    type_checker_init(checker);
    checker->program = program;
    checker->symbols = symbols;

    symbol_error = symbol_table_get_error(symbols);
    if (symbol_error) {
        checker->has_error = true;
        checker->error.primary_span = symbol_error->primary_span;
        checker->error.related_span = symbol_error->related_span;
        checker->error.has_related_span = symbol_error->has_related_span;
        strncpy(checker->error.message, symbol_error->message,
                sizeof(checker->error.message) - 1);
        checker->error.message[sizeof(checker->error.message) - 1] = '\0';
        return false;
    }

    unresolved = symbol_table_get_unresolved_identifier(symbols, 0);
    if (unresolved) {
        tc_set_error_at(checker, unresolved->source_span, NULL,
                        "Unresolved identifier '%s'.",
                        (unresolved->identifier &&
                         unresolved->identifier->kind == AST_EXPR_IDENTIFIER &&
                         unresolved->identifier->as.identifier)
                            ? unresolved->identifier->as.identifier
                            : "<unknown>");
        return false;
    }

    if (!type_resolver_resolve_program(&checker->resolver, program)) {
        resolution_error = type_resolver_get_error(&checker->resolver);
        checker->has_error = true;
        if (resolution_error) {
            checker->error.primary_span = resolution_error->primary_span;
            checker->error.related_span = resolution_error->related_span;
            checker->error.has_related_span = resolution_error->has_related_span;
            strncpy(checker->error.message,
                    resolution_error->message,
                    sizeof(checker->error.message) - 1);
            checker->error.message[sizeof(checker->error.message) - 1] = '\0';
        } else {
            strncpy(checker->error.message,
                    "Type resolution failed.",
                    sizeof(checker->error.message) - 1);
            checker->error.message[sizeof(checker->error.message) - 1] = '\0';
        }
        return false;
    }

    if (!tc_validate_program_start_decls(checker, program)) {
        return false;
    }

    root_scope = symbol_table_root_scope(symbols);
    if (!root_scope) {
        tc_set_error(checker, "Internal error: missing root scope.");
        return false;
    }

    for (i = 0; i < program->top_level_count; i++) {
        const AstTopLevelDecl *decl = program->top_level_decls[i];

        if (decl->kind == AST_TOP_LEVEL_BINDING) {
            const Symbol *symbol = symbol_table_find_symbol_for_declaration(
                symbols,
                &decl->as.binding_decl);

            if (!symbol) {
                tc_set_error_at(checker,
                                decl->as.binding_decl.name_span,
                                NULL,
                                "Internal error: missing symbol for '%s'.",
                                decl->as.binding_decl.name);
                return false;
            }

            if (!tc_validate_binding_modifiers(checker, &decl->as.binding_decl)) {
                return false;
            }

            if (!tc_resolve_symbol_info(checker, symbol)) {
                return false;
            }
        } else if (decl->kind == AST_TOP_LEVEL_TYPE_ALIAS) {
            const Symbol *symbol = symbol_table_find_symbol_for_declaration(
                symbols,
                &decl->as.type_alias_decl);

            if (!symbol) {
                tc_set_error_at(checker,
                                decl->as.type_alias_decl.name_span,
                                NULL,
                                "Internal error: missing symbol for type alias '%s'.",
                                decl->as.type_alias_decl.name);
                return false;
            }

            if (!tc_resolve_symbol_info(checker, symbol)) {
                return false;
            }
        } else if (decl->kind == AST_TOP_LEVEL_UNION) {
            const Symbol *symbol = symbol_table_find_symbol_for_declaration(
                symbols,
                &decl->as.union_decl);

            if (!symbol) {
                tc_set_error_at(checker,
                                decl->as.union_decl.name_span,
                                NULL,
                                "Internal error: missing symbol for union '%s'.",
                                decl->as.union_decl.name);
                return false;
            }

            if (!tc_resolve_symbol_info(checker, symbol)) {
                return false;
            }
        } else if (decl->kind == AST_TOP_LEVEL_ASM) {
            const Symbol *symbol = symbol_table_find_symbol_for_declaration(
                symbols,
                &decl->as.asm_decl);

            if (!symbol) {
                tc_set_error_at(checker,
                                decl->as.asm_decl.name_span,
                                NULL,
                                "Internal error: missing symbol for asm binding '%s'.",
                                decl->as.asm_decl.name);
                return false;
            }

            if (!tc_resolve_symbol_info(checker, symbol)) {
                return false;
            }
            if (!tc_check_parameter_defaults(checker, &decl->as.asm_decl.parameters)) {
                return false;
            }
        } else if (decl->kind == AST_TOP_LEVEL_LAYOUT) {
            /* Layout declarations are type metadata only — nothing to type-check. */
        } else if (!tc_check_start_decl(checker, &decl->as.start_decl)) {
            return false;
        }
    }

    return !checker->has_error;
}

bool checked_type_to_string(CheckedType type, char *buffer, size_t buffer_size) {
    size_t i;
    int written;

    if (!buffer || buffer_size == 0) {
        return false;
    }

    switch (type.kind) {
    case CHECKED_TYPE_INVALID:
        return snprintf(buffer, buffer_size, "<invalid>") >= 0;
    case CHECKED_TYPE_VOID:
        return snprintf(buffer, buffer_size, "void") >= 0;
    case CHECKED_TYPE_NULL:
        return snprintf(buffer, buffer_size, "null") >= 0;
    case CHECKED_TYPE_EXTERNAL:
        return snprintf(buffer, buffer_size, "<external>") >= 0;
    case CHECKED_TYPE_VALUE:
        written = snprintf(buffer, buffer_size, "%s", tc_primitive_type_name(type.primitive));
        if (written < 0 || (size_t)written >= buffer_size) {
            return false;
        }
        for (i = 0; i < type.array_depth; i++) {
            bool has_size = false;
            unsigned long long size = 0;

            if (!tc_checked_type_array_extent(type, i, &has_size, &size)) {
                return false;
            }
            written += snprintf(buffer + written,
                                buffer_size - (size_t)written,
                                has_size ? "[%llu]" : "[]",
                                size);
            if (written < 0 || (size_t)written >= buffer_size) {
                return false;
            }
        }
        return true;

    case CHECKED_TYPE_NAMED:
        written = snprintf(buffer, buffer_size, "%s", type.name ? type.name : "?");
        if (written < 0 || (size_t)written >= buffer_size) {
            return false;
        }
        if (type.generic_arg_count > 0) {
            written += snprintf(buffer + written, buffer_size - (size_t)written,
                                "<...%zu>", type.generic_arg_count);
            if (written < 0 || (size_t)written >= buffer_size) {
                return false;
            }
        }
        for (i = 0; i < type.array_depth; i++) {
            bool has_size = false;
            unsigned long long size = 0;

            if (!tc_checked_type_array_extent(type, i, &has_size, &size)) {
                return false;
            }
            written += snprintf(buffer + written,
                                buffer_size - (size_t)written,
                                has_size ? "[%llu]" : "[]",
                                size);
            if (written < 0 || (size_t)written >= buffer_size) {
                return false;
            }
        }
        return true;

    case CHECKED_TYPE_TYPE_PARAM:
        return snprintf(buffer, buffer_size, "%s", type.name ? type.name : "?") >= 0;

    case CHECKED_TYPE_FUNCTION:
        return snprintf(buffer, buffer_size, "<function(%zu)>", type.generic_arg_count) >= 0;
    }

    return false;
}

static bool tc_append_format(char *buffer,
                             size_t buffer_size,
                             size_t *written,
                             const char *format,
                             ...) {
    va_list args;
    int appended;

    if (!buffer || !written || *written >= buffer_size) {
        return false;
    }

    va_start(args, format);
    appended = vsnprintf(buffer + *written,
                         buffer_size - *written,
                         format,
                         args);
    va_end(args);

    if (appended < 0 || (size_t)appended >= buffer_size - *written) {
        return false;
    }

    *written += (size_t)appended;
    return true;
}

static bool tc_ast_type_slice_to_string_impl(const AstType *type,
                                             size_t consumed_dimensions,
                                             char *buffer,
                                             size_t buffer_size,
                                             size_t *written) {
    size_t i;

    if (!type) {
        return tc_append_format(buffer, buffer_size, written, "?");
    }

    switch (type->kind) {
    case AST_TYPE_VOID:
        if (!tc_append_format(buffer, buffer_size, written, "void")) {
            return false;
        }
        break;

    case AST_TYPE_PRIMITIVE:
        if (!tc_append_format(buffer,
                              buffer_size,
                              written,
                              "%s",
                              tc_primitive_type_name(type->primitive))) {
            return false;
        }
        break;

    case AST_TYPE_ARR:
        if (!tc_append_format(buffer, buffer_size, written, "arr")) {
            return false;
        }
        break;

    case AST_TYPE_PTR:
        if (!tc_append_format(buffer, buffer_size, written, "ptr")) {
            return false;
        }
        break;

    case AST_TYPE_NAMED:
        if (!tc_append_format(buffer,
                              buffer_size,
                              written,
                              "%s",
                              type->name ? type->name : "?")) {
            return false;
        }
        break;

    case AST_TYPE_THREAD:
        if (!tc_append_format(buffer, buffer_size, written, "Thread")) {
            return false;
        }
        break;

    case AST_TYPE_MUTEX:
        if (!tc_append_format(buffer, buffer_size, written, "Mutex")) {
            return false;
        }
        break;

    case AST_TYPE_FUTURE:
        if (!tc_append_format(buffer, buffer_size, written, "Future")) {
            return false;
        }
        break;

    case AST_TYPE_ATOMIC:
        if (!tc_append_format(buffer, buffer_size, written, "Atomic")) {
            return false;
        }
        break;
    }

    if (type->generic_args.count > 0) {
        if (!tc_append_format(buffer, buffer_size, written, "<")) {
            return false;
        }
        for (i = 0; i < type->generic_args.count; i++) {
            if (i > 0 && !tc_append_format(buffer, buffer_size, written, ", ")) {
                return false;
            }
            if (type->generic_args.items[i].kind == AST_GENERIC_ARG_WILDCARD) {
                if (!tc_append_format(buffer, buffer_size, written, "?")) {
                    return false;
                }
                continue;
            }
            if (!tc_ast_type_slice_to_string_impl(type->generic_args.items[i].type,
                                                  0,
                                                  buffer,
                                                  buffer_size,
                                                  written)) {
                return false;
            }
        }
        if (!tc_append_format(buffer, buffer_size, written, ">")) {
            return false;
        }
    }

    if (consumed_dimensions > type->dimension_count) {
        return false;
    }

    for (i = consumed_dimensions; i < type->dimension_count; i++) {
        if (type->dimensions[i].has_size && type->dimensions[i].size_literal) {
            if (!tc_append_format(buffer,
                                  buffer_size,
                                  written,
                                  "[%s]",
                                  type->dimensions[i].size_literal)) {
                return false;
            }
        } else if (!tc_append_format(buffer, buffer_size, written, "[]")) {
            return false;
        }
    }

    return true;
}

bool tc_ast_type_slice_to_string(const AstType *type,
                                 size_t consumed_dimensions,
                                 char *buffer,
                                 size_t buffer_size) {
    size_t written = 0;

    if (!buffer || buffer_size == 0) {
        return false;
    }

    buffer[0] = '\0';
    return tc_ast_type_slice_to_string_impl(type,
                                            consumed_dimensions,
                                            buffer,
                                            buffer_size,
                                            &written);
}

static bool tc_parse_array_size_value(const char *text,
                                      unsigned long long *value_out) {
    char *end = NULL;
    unsigned long long value;

    if (!text || text[0] == '\0') {
        return false;
    }

    value = strtoull(text, &end, 10);
    if (!end || *end != '\0' || value == 0) {
        return false;
    }

    if (value_out) {
        *value_out = value;
    }
    return true;
}

typedef enum {
    TC_SIZED_ARRAY_DIAG_ASSIGNMENT = 0,
    TC_SIZED_ARRAY_DIAG_ARGUMENT,
    TC_SIZED_ARRAY_DIAG_DEFAULT_VALUE,
    TC_SIZED_ARRAY_DIAG_RETURN_VALUE
} TcSizedArrayDiagnosticKind;

bool tc_expression_declared_array_shape(TypeChecker *checker,
                                        const AstExpression *expression,
                                        const AstType **declared_type_out,
                                        size_t *consumed_dimensions_out) {
    const TypeCheckInfo *info;
    const AstType *declared_type;
    size_t consumed_dimensions;

    if (declared_type_out) {
        *declared_type_out = NULL;
    }
    if (consumed_dimensions_out) {
        *consumed_dimensions_out = 0;
    }
    if (!checker || !expression) {
        return false;
    }

    info = type_checker_get_expression_info(checker, expression);
    if (info && info->array_shape_type &&
        info->array_shape_consumed_dimensions < info->array_shape_type->dimension_count) {
        if (declared_type_out) {
            *declared_type_out = info->array_shape_type;
        }
        if (consumed_dimensions_out) {
            *consumed_dimensions_out = info->array_shape_consumed_dimensions;
        }
        return true;
    }

    switch (expression->kind) {
    case AST_EXPR_IDENTIFIER:
        {
            const Symbol *symbol = symbol_table_resolve_identifier(checker->symbols,
                                                                   expression);

            if (!symbol || !symbol->declared_type ||
                symbol->declared_type->dimension_count == 0) {
                return false;
            }
            if (declared_type_out) {
                *declared_type_out = symbol->declared_type;
            }
            return true;
        }

    case AST_EXPR_INDEX:
        if (!tc_expression_declared_array_shape(checker,
                                                expression->as.index.target,
                                                &declared_type,
                                                &consumed_dimensions)) {
            return false;
        }
        if (!declared_type || consumed_dimensions >= declared_type->dimension_count) {
            return false;
        }
        if (declared_type_out) {
            *declared_type_out = declared_type;
        }
        if (consumed_dimensions_out) {
            *consumed_dimensions_out = consumed_dimensions + 1;
        }
        return true;

    case AST_EXPR_GROUPING:
        return tc_expression_declared_array_shape(checker,
                                                  expression->as.grouping.inner,
                                                  declared_type_out,
                                                  consumed_dimensions_out);

    case AST_EXPR_CAST:
        return tc_expression_declared_array_shape(checker,
                                                  expression->as.cast.expression,
                                                  declared_type_out,
                                                  consumed_dimensions_out);

    default:
        return false;
    }
}

static bool tc_validate_sized_array_checked_compatibility(TypeChecker *checker,
                                                          const AstType *target_type,
                                                          size_t target_consumed_dimensions,
                                                          const AstExpression *source_expression,
                                                          CheckedType source_type,
                                                          const AstSourceSpan *related_span,
                                                          TcSizedArrayDiagnosticKind diagnostic_kind,
                                                          const char *subject_kind,
                                                          const char *subject_name) {
    size_t i;

    if (!checker || !target_type || !source_expression ||
        target_consumed_dimensions >= target_type->dimension_count ||
        source_type.array_depth == 0) {
        return true;
    }

    for (i = 0; i < source_type.array_depth &&
                target_consumed_dimensions + i < target_type->dimension_count;
         i++) {
        const AstArrayDimension *target_dimension =
            &target_type->dimensions[target_consumed_dimensions + i];
        unsigned long long target_size;
        unsigned long long source_size;
        bool source_has_size = false;
        char target_text[128];
        char source_text[128];
        CheckedType source_slice;

        if (!target_dimension->has_size) {
            continue;
        }

        if (!tc_parse_array_size_value(target_dimension->size_literal, &target_size)) {
            continue;
        }
        if (!tc_checked_type_array_extent(source_type, i, &source_has_size, &source_size) ||
            (source_has_size && target_size == source_size)) {
            continue;
        }

        if (!tc_ast_type_slice_to_string(target_type,
                                         target_consumed_dimensions + i,
                                         target_text,
                                         sizeof(target_text))) {
            strncpy(target_text, "<array>", sizeof(target_text) - 1);
            target_text[sizeof(target_text) - 1] = '\0';
        }

        if (source_expression->kind == AST_EXPR_ARRAY_LITERAL && source_has_size) {
            size_t actual_count = (size_t)source_size;

            switch (diagnostic_kind) {
            case TC_SIZED_ARRAY_DIAG_ARGUMENT:
                tc_set_error_at(checker,
                                source_expression->source_span,
                                related_span,
                                "Array literal passed to parameter '%s' has %zu element%s, but target type %s requires %llu.",
                                subject_name ? subject_name : "<anonymous>",
                                actual_count,
                                actual_count == 1 ? "" : "s",
                                target_text,
                                target_size);
                break;

            case TC_SIZED_ARRAY_DIAG_RETURN_VALUE:
                tc_set_error_at(checker,
                                source_expression->source_span,
                                related_span,
                                "Array literal returned from %s has %zu element%s, but target type %s requires %llu.",
                                subject_kind ? subject_kind : "return value",
                                actual_count,
                                actual_count == 1 ? "" : "s",
                                target_text,
                                target_size);
                break;

            case TC_SIZED_ARRAY_DIAG_DEFAULT_VALUE:
                tc_set_error_at(checker,
                                source_expression->source_span,
                                related_span,
                                "Array literal default value for parameter '%s' has %zu element%s, but target type %s requires %llu.",
                                subject_name ? subject_name : "<anonymous>",
                                actual_count,
                                actual_count == 1 ? "" : "s",
                                target_text,
                                target_size);
                break;

            case TC_SIZED_ARRAY_DIAG_ASSIGNMENT:
            default:
                tc_set_error_at(checker,
                                source_expression->source_span,
                                related_span,
                                "Array literal assigned to %s '%s' has %zu element%s, but target type %s requires %llu.",
                                subject_kind ? subject_kind : "symbol",
                                subject_name ? subject_name : "<anonymous>",
                                actual_count,
                                actual_count == 1 ? "" : "s",
                                target_text,
                                target_size);
                break;
            }

            return false;
        }

        source_slice = tc_checked_type_consume_array_prefix(source_type, i);
        if (!checked_type_to_string(source_slice, source_text, sizeof(source_text))) {
            strncpy(source_text, "<array>", sizeof(source_text) - 1);
            source_text[sizeof(source_text) - 1] = '\0';
        }

        switch (diagnostic_kind) {
        case TC_SIZED_ARRAY_DIAG_ARGUMENT:
            tc_set_error_at(checker,
                            source_expression->source_span,
                            related_span,
                            "Cannot pass array of declared type %s to parameter '%s' of type %s.",
                            source_text,
                            subject_name ? subject_name : "<anonymous>",
                            target_text);
            break;

        case TC_SIZED_ARRAY_DIAG_RETURN_VALUE:
            tc_set_error_at(checker,
                            source_expression->source_span,
                            related_span,
                            "Cannot return array of declared type %s from %s expecting %s.",
                            source_text,
                            subject_kind ? subject_kind : "return value",
                            target_text);
            break;

        case TC_SIZED_ARRAY_DIAG_DEFAULT_VALUE:
            tc_set_error_at(checker,
                            source_expression->source_span,
                            related_span,
                            "Cannot use array of declared type %s as the default value for parameter '%s' of type %s.",
                            source_text,
                            subject_name ? subject_name : "<anonymous>",
                            target_text);
            break;

        case TC_SIZED_ARRAY_DIAG_ASSIGNMENT:
        default:
            tc_set_error_at(checker,
                            source_expression->source_span,
                            related_span,
                            "Cannot assign array of declared type %s to %s '%s' of type %s.",
                            source_text,
                            subject_kind ? subject_kind : "symbol",
                            subject_name ? subject_name : "<anonymous>",
                            target_text);
            break;
        }
        return false;
    }

    return true;
}

static bool tc_validate_sized_array_flow(TypeChecker *checker,
                                         const AstType *target_type,
                                         size_t consumed_dimensions,
                                         const AstExpression *source_expression,
                                         const AstSourceSpan *related_span,
                                         TcSizedArrayDiagnosticKind diagnostic_kind,
                                         const char *subject_kind,
                                         const char *subject_name) {
    const TypeCheckInfo *source_info;
    CheckedType source_type;

    if (!target_type || consumed_dimensions >= target_type->dimension_count) {
        return true;
    }

    source_info = type_checker_get_expression_info(checker, source_expression);
    if (!source_info) {
        source_info = tc_check_expression(checker, source_expression);
    }
    if (!source_info) {
        return false;
    }

    source_type = tc_type_check_source_type(source_info);
    return tc_validate_sized_array_checked_compatibility(checker,
                                                         target_type,
                                                         consumed_dimensions,
                                                         source_expression,
                                                         source_type,
                                                         related_span,
                                                         diagnostic_kind,
                                                         subject_kind,
                                                         subject_name);
}

bool tc_validate_sized_array_assignment(TypeChecker *checker,
                                        const AstType *target_type,
                                        size_t consumed_dimensions,
                                        const AstExpression *source_expression,
                                        const AstSourceSpan *related_span,
                                        const char *subject_kind,
                                        const char *subject_name) {
    return tc_validate_sized_array_flow(checker,
                                        target_type,
                                        consumed_dimensions,
                                        source_expression,
                                        related_span,
                                        TC_SIZED_ARRAY_DIAG_ASSIGNMENT,
                                        subject_kind,
                                        subject_name);
}

bool tc_validate_sized_array_argument(TypeChecker *checker,
                                      const AstType *target_type,
                                      size_t consumed_dimensions,
                                      const AstExpression *source_expression,
                                      const AstSourceSpan *related_span,
                                      const char *parameter_name) {
    return tc_validate_sized_array_flow(checker,
                                        target_type,
                                        consumed_dimensions,
                                        source_expression,
                                        related_span,
                                        TC_SIZED_ARRAY_DIAG_ARGUMENT,
                                        NULL,
                                        parameter_name);
}

bool tc_validate_sized_array_default_value(TypeChecker *checker,
                                           const AstType *target_type,
                                           size_t consumed_dimensions,
                                           const AstExpression *source_expression,
                                           const AstSourceSpan *related_span,
                                           const char *parameter_name) {
    return tc_validate_sized_array_flow(checker,
                                        target_type,
                                        consumed_dimensions,
                                        source_expression,
                                        related_span,
                                        TC_SIZED_ARRAY_DIAG_DEFAULT_VALUE,
                                        NULL,
                                        parameter_name);
}

bool tc_validate_sized_array_return_value(TypeChecker *checker,
                                          const AstType *target_type,
                                          size_t consumed_dimensions,
                                          const AstExpression *source_expression,
                                          const AstSourceSpan *related_span,
                                          const char *return_context_name) {
    return tc_validate_sized_array_flow(checker,
                                        target_type,
                                        consumed_dimensions,
                                        source_expression,
                                        related_span,
                                        TC_SIZED_ARRAY_DIAG_RETURN_VALUE,
                                        return_context_name,
                                        NULL);
}
