#include "symbol_table.h"
#include "symbol_table_internal.h"

#include <stdlib.h>
#include <string.h>

bool st_analyze_expression(SymbolTable *table, const AstExpression *expression,
                           Scope *scope) {
    size_t i;

    if (!expression) {
        return true;
    }

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        if (expression->as.literal.kind == AST_LITERAL_TEMPLATE) {
            for (i = 0; i < expression->as.literal.as.template_parts.count; i++) {
                const AstTemplatePart *part =
                    &expression->as.literal.as.template_parts.items[i];

                if (part->kind == AST_TEMPLATE_PART_EXPRESSION &&
                    !st_analyze_expression(table, part->as.expression, scope)) {
                    return false;
                }
            }
        }
        return true;

    case AST_EXPR_IDENTIFIER:
        {
            const OverloadSet *resolved_overload_set =
                symbol_table_lookup_overload_set(table, scope, expression->as.identifier);
            const Symbol *resolved = symbol_table_lookup(table, scope,
                                                         expression->as.identifier);

            if (resolved_overload_set) {
                return st_resolutions_append(table,
                                             expression,
                                             scope,
                                             NULL,
                                             resolved_overload_set);
            }

            if (resolved) {
                return st_resolutions_append(table, expression, scope, resolved, NULL);
            }

            if (expression->as.identifier &&
                (strcmp(expression->as.identifier, "Mutex") == 0 ||
                 strcmp(expression->as.identifier, "Thread") == 0 ||
                 strcmp(expression->as.identifier, "Future") == 0 ||
                 strcmp(expression->as.identifier, "Atomic") == 0 ||
                 strcmp(expression->as.identifier, "car") == 0 ||
                 strcmp(expression->as.identifier, "cdr") == 0 ||
                 strcmp(expression->as.identifier, "typeof") == 0 ||
                 strcmp(expression->as.identifier, "isint") == 0 ||
                 strcmp(expression->as.identifier, "isfloat") == 0 ||
                 strcmp(expression->as.identifier, "isbool") == 0 ||
                 strcmp(expression->as.identifier, "isstring") == 0 ||
                 strcmp(expression->as.identifier, "isarray") == 0 ||
                 strcmp(expression->as.identifier, "issametype") == 0)) {
                return true;
            }

            return st_unresolved_append(table, expression, scope);
        }

    case AST_EXPR_LAMBDA:
        return st_analyze_lambda_expression(table, expression, scope);

    case AST_EXPR_ASSIGNMENT:
        return st_analyze_expression(table, expression->as.assignment.target, scope) &&
               st_analyze_expression(table, expression->as.assignment.value, scope);

    case AST_EXPR_TERNARY:
        return st_analyze_expression(table, expression->as.ternary.condition, scope) &&
               st_analyze_expression(table, expression->as.ternary.then_branch, scope) &&
               st_analyze_expression(table, expression->as.ternary.else_branch, scope);

    case AST_EXPR_BINARY:
        return st_analyze_expression(table, expression->as.binary.left, scope) &&
               st_analyze_expression(table, expression->as.binary.right, scope);

    case AST_EXPR_UNARY:
        return st_analyze_expression(table, expression->as.unary.operand, scope);

    case AST_EXPR_CALL:
        if (!st_analyze_expression(table, expression->as.call.callee, scope)) {
            return false;
        }
        for (i = 0; i < expression->as.call.arguments.count; i++) {
            if (!st_analyze_expression(table, expression->as.call.arguments.items[i], scope)) {
                return false;
            }
        }
        return true;

    case AST_EXPR_INDEX:
        return st_analyze_expression(table, expression->as.index.target, scope) &&
               st_analyze_expression(table, expression->as.index.index, scope);

    case AST_EXPR_MEMBER:
        return st_analyze_expression(table, expression->as.member.target, scope);

    case AST_EXPR_CAST:
        return st_analyze_expression(table, expression->as.cast.expression, scope);

    case AST_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expression->as.array_literal.elements.count; i++) {
            if (!st_analyze_expression(table,
                                       expression->as.array_literal.elements.items[i],
                                       scope)) {
                return false;
            }
        }
        return true;

    case AST_EXPR_GROUPING:
        return st_analyze_expression(table, expression->as.grouping.inner, scope);

    case AST_EXPR_DISCARD:
        return true;

    case AST_EXPR_POST_INCREMENT:
        return st_analyze_expression(table, expression->as.post_increment.operand, scope);

    case AST_EXPR_POST_DECREMENT:
        return st_analyze_expression(table, expression->as.post_decrement.operand, scope);

    case AST_EXPR_MEMORY_OP:
        for (i = 0; i < expression->as.memory_op.arguments.count; i++) {
            if (!st_analyze_expression(table,
                                       expression->as.memory_op.arguments.items[i],
                                       scope)) {
                return false;
            }
        }
        return true;
    case AST_EXPR_SPAWN:
        return st_analyze_expression(table, expression->as.spawn.callable, scope);
    }

    return false;
}

bool st_add_parameter_symbols(SymbolTable *table,
                              const AstParameterList *parameters,
                              Scope *scope) {
    size_t i;

    for (i = 0; i < parameters->count; i++) {
        const AstParameter *parameter = &parameters->items[i];
        const Symbol *conflicting_symbol = scope_lookup_local(scope, parameter->name);
        Symbol *symbol;

        if (conflicting_symbol) {
            st_set_error_at(table,
                            parameter->name_span,
                            &conflicting_symbol->declaration_span,
                            "Duplicate symbol '%s' in %s.",
                            parameter->name,
                            scope_kind_name(scope->kind));
            return false;
        }

        symbol = st_symbol_new(table, SYMBOL_KIND_PARAMETER,
                               parameter->name, NULL,
                               &parameter->type,
                               false, false,
                               false, false, false, false,
                               parameter->name_span,
                               parameter, scope);
        if (!symbol) {
            return false;
        }

        if (!st_scope_append_symbol(table, scope, symbol)) {
            st_symbol_free(symbol);
            return false;
        }
    }

    return true;
}

bool st_add_local_symbol(SymbolTable *table,
                         const AstLocalBindingStatement *binding,
                         Scope *scope) {
    const Symbol *conflicting_symbol = scope_lookup_local(scope, binding->name);
    Symbol *symbol;

    if (conflicting_symbol) {
        st_set_error_at(table,
                        binding->name_span,
                        &conflicting_symbol->declaration_span,
                        "Duplicate symbol '%s' in %s.",
                        binding->name,
                        scope_kind_name(scope->kind));
        return false;
    }

    symbol = st_symbol_new(table, SYMBOL_KIND_LOCAL,
                           binding->name, NULL,
                           &binding->declared_type,
                           binding->is_inferred_type,
                           binding->is_final,
                           false, false,
                           binding->is_internal, false,
                           binding->name_span,
                           binding, scope);
    if (!symbol) {
        return false;
    }

    if (!st_scope_append_symbol(table, scope, symbol)) {
        st_symbol_free(symbol);
        return false;
    }

    return true;
}
