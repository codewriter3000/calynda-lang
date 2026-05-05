#include "type_checker_internal.h"

static bool tc_checked_type_is_named_type(CheckedType type, const char *name) {
    return type.kind == CHECKED_TYPE_NAMED &&
           type.array_depth == 0 &&
           type.name != NULL &&
           strcmp(type.name, name) == 0;
}

static bool tc_checked_type_is_float_value(CheckedType type) {
    return type.kind == CHECKED_TYPE_VALUE &&
           type.array_depth == 0 &&
           tc_primitive_is_float(type.primitive);
}

static const AstExpression *tc_strip_grouping_expression(const AstExpression *expression) {
    while (expression && expression->kind == AST_EXPR_GROUPING) {
        expression = expression->as.grouping.inner;
    }
    return expression;
}

static bool tc_identifier_has_symbol_or_overload_resolution(const TypeChecker *checker,
                                                            const AstExpression *identifier) {
    const SymbolResolution *resolution;

    if (!checker || !identifier) {
        return false;
    }

    resolution = symbol_table_find_resolution(checker->symbols, identifier);
    return resolution != NULL &&
           (resolution->symbol != NULL || resolution->overload_set != NULL);
}

static bool tc_callable_parameters_accept_arity(const AstParameterList *parameters,
                                                size_t argument_count,
                                                size_t *required_count_out) {
    bool has_varargs = false;
    size_t required_count = 0;
    size_t maximum_count = 0;
    size_t i;

    if (!parameters) {
        if (required_count_out) {
            *required_count_out = 0;
        }
        return argument_count == 0;
    }

    maximum_count = parameters->count;
    if (parameters->count > 0 &&
        parameters->items[parameters->count - 1].is_varargs) {
        has_varargs = true;
        maximum_count = (size_t)-1;
    }

    for (i = 0; i < parameters->count; i++) {
        if (parameters->items[i].is_varargs) {
            break;
        }
        if (!parameters->items[i].default_expr) {
            required_count++;
        }
    }

    if (required_count_out) {
        *required_count_out = required_count;
    }

    return argument_count >= required_count && argument_count <= maximum_count;
}

static bool tc_callable_parameter_type_at(TypeChecker *checker,
                                          const AstParameterList *parameters,
                                          size_t argument_index,
                                          CheckedType *parameter_type_out) {
    size_t parameter_index;

    if (!checker || !parameter_type_out) {
        return false;
    }

    if (!parameters || parameters->count == 0) {
        return false;
    }

    parameter_index = argument_index < parameters->count
        ? argument_index
        : parameters->count - 1;
    if (parameters->items[parameter_index].is_untyped) {
        *parameter_type_out = tc_checked_type_external();
        return true;
    }
    *parameter_type_out = tc_checked_type_from_ast_type(
        checker,
        &parameters->items[parameter_index].type);
    return !checker->has_error;
}

static const Symbol *tc_select_overload_symbol(TypeChecker *checker,
                                               const AstExpression *callee_identifier,
                                               const OverloadSet *overload_set,
                                               const CheckedType *argument_types,
                                               size_t argument_count) {
    const Symbol *exact_match = NULL;
    const Symbol *widening_match = NULL;
    size_t exact_match_count = 0;
    size_t widening_match_count = 0;
    size_t arity_match_count = 0;
    size_t i;

    if (!checker || !callee_identifier || !overload_set) {
        return NULL;
    }

    for (i = 0; i < overload_set->symbol_count; i++) {
        const Symbol *candidate = overload_set->symbols[i];
        const TypeCheckInfo *candidate_info = tc_resolve_symbol_info(checker, candidate);
        bool exact_candidate = true;
        bool widening_candidate = true;
        size_t arg_index;

        if (!candidate_info) {
            return NULL;
        }
        if (!candidate_info->is_callable) {
            continue;
        }
        if (!tc_callable_parameters_accept_arity(candidate_info->parameters,
                                                 argument_count,
                                                 NULL)) {
            continue;
        }

        arity_match_count++;
        for (arg_index = 0; arg_index < argument_count; arg_index++) {
            CheckedType parameter_type;

            if (!candidate_info->parameters ||
                !tc_callable_parameter_type_at(checker,
                                              candidate_info->parameters,
                                              arg_index,
                                              &parameter_type)) {
                exact_candidate = false;
                widening_candidate = false;
                break;
            }

            if (!tc_checked_type_equals(parameter_type, argument_types[arg_index])) {
                exact_candidate = false;
            }
            if (!tc_checked_type_assignable(parameter_type, argument_types[arg_index])) {
                widening_candidate = false;
                break;
            }
        }
        if (checker->has_error) {
            return NULL;
        }

        if (exact_candidate) {
            exact_match = candidate;
            exact_match_count++;
            continue;
        }
        if (widening_candidate) {
            widening_match = candidate;
            widening_match_count++;
        }
    }

    if (exact_match_count == 1) {
        return exact_match;
    }
    if (exact_match_count > 1) {
        tc_set_error_at(checker,
                        callee_identifier->source_span,
                        &overload_set->declaration_span,
                        "Call to overload set '%s' is ambiguous.",
                        overload_set->name ? overload_set->name : "<anonymous>");
        return NULL;
    }
    if (widening_match_count == 1) {
        return widening_match;
    }
    if (widening_match_count > 1) {
        tc_set_error_at(checker,
                        callee_identifier->source_span,
                        &overload_set->declaration_span,
                        "Call to overload set '%s' is ambiguous.",
                        overload_set->name ? overload_set->name : "<anonymous>");
        return NULL;
    }

    if (arity_match_count == 0) {
        tc_set_error_at(checker,
                        callee_identifier->source_span,
                        &overload_set->declaration_span,
                        "No overload of '%s' accepts %zu argument%s.",
                        overload_set->name ? overload_set->name : "<anonymous>",
                        argument_count,
                        argument_count == 1 ? "" : "s");
    } else {
        tc_set_error_at(checker,
                        callee_identifier->source_span,
                        &overload_set->declaration_span,
                        "No overload of '%s' matches the provided argument types.",
                        overload_set->name ? overload_set->name : "<anonymous>");
    }
    return NULL;
}

static bool tc_try_resolve_overload_call(TypeChecker *checker,
                                         const AstExpression *expression) {
    const AstExpression *callee_identifier;
    const SymbolResolution *resolution;
    CheckedType *argument_types = NULL;
    size_t i;

    if (!checker || !expression || expression->kind != AST_EXPR_CALL) {
        return false;
    }

    callee_identifier = tc_strip_grouping_expression(expression->as.call.callee);
    if (!callee_identifier || callee_identifier->kind != AST_EXPR_IDENTIFIER) {
        return false;
    }

    resolution = symbol_table_find_resolution(checker->symbols, callee_identifier);
    if (!resolution || resolution->symbol || !resolution->overload_set) {
        return false;
    }

    if (expression->as.call.arguments.count > 0) {
        argument_types = calloc(expression->as.call.arguments.count, sizeof(*argument_types));
        if (!argument_types) {
            tc_set_error_at(checker,
                            expression->source_span,
                            NULL,
                            "Out of memory while resolving overload call.");
            return true;
        }
    }

#include "type_checker_expr_ext_p2.inc"
