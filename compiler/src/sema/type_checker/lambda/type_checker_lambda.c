#include "type_checker_internal.h"

static bool tc_find_parameter_index(const AstParameterList *parameters,
                                    const Symbol *symbol,
                                    size_t *out_index) {
    size_t i;

    if (!parameters || !symbol || !out_index) {
        return false;
    }

    for (i = 0; i < parameters->count; i++) {
        if (strcmp(parameters->items[i].name, symbol->name) == 0) {
            *out_index = i;
            return true;
        }
    }

    return false;
}

static bool tc_validate_default_parameter_references(TypeChecker *checker,
                                                     const AstExpression *expression,
                                                     const AstParameterList *parameters,
                                                     size_t parameter_index,
                                                     const AstParameter *parameter) {
    size_t i;

    if (!expression) {
        return true;
    }

    switch (expression->kind) {
    case AST_EXPR_IDENTIFIER:
        {
            const Symbol *symbol = symbol_table_resolve_identifier(checker->symbols, expression);
            size_t referenced_index;

            if (symbol &&
                symbol->kind == SYMBOL_KIND_PARAMETER &&
                tc_find_parameter_index(parameters, symbol, &referenced_index) &&
                referenced_index >= parameter_index) {
                tc_set_error_at(checker,
                                expression->source_span,
                                &parameter->name_span,
                                "Default value for parameter '%s' can only reference earlier parameters.",
                                parameter->name);
                return false;
            }
        }
        return true;

    case AST_EXPR_LAMBDA:
        return true;

    case AST_EXPR_ASSIGNMENT:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.assignment.target,
                                                        parameters,
                                                        parameter_index,
                                                        parameter) &&
               tc_validate_default_parameter_references(checker,
                                                        expression->as.assignment.value,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_TERNARY:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.ternary.condition,
                                                        parameters,
                                                        parameter_index,
                                                        parameter) &&
               tc_validate_default_parameter_references(checker,
                                                        expression->as.ternary.then_branch,
                                                        parameters,
                                                        parameter_index,
                                                        parameter) &&
               tc_validate_default_parameter_references(checker,
                                                        expression->as.ternary.else_branch,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_BINARY:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.binary.left,
                                                        parameters,
                                                        parameter_index,
                                                        parameter) &&
               tc_validate_default_parameter_references(checker,
                                                        expression->as.binary.right,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_UNARY:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.unary.operand,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_CALL:
        if (!tc_validate_default_parameter_references(checker,
                                                      expression->as.call.callee,
                                                      parameters,
                                                      parameter_index,
                                                      parameter)) {
            return false;
        }
        for (i = 0; i < expression->as.call.arguments.count; i++) {
            if (!tc_validate_default_parameter_references(checker,
                                                          expression->as.call.arguments.items[i],
                                                          parameters,
                                                          parameter_index,
                                                          parameter)) {
                return false;
            }
        }
        return true;

    case AST_EXPR_INDEX:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.index.target,
                                                        parameters,
                                                        parameter_index,
                                                        parameter) &&
               tc_validate_default_parameter_references(checker,
                                                        expression->as.index.index,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_MEMBER:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.member.target,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_CAST:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.cast.expression,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expression->as.array_literal.elements.count; i++) {
            if (!tc_validate_default_parameter_references(checker,
                                                          expression->as.array_literal.elements.items[i],
                                                          parameters,
                                                          parameter_index,
                                                          parameter)) {
                return false;
            }
        }
        return true;

    case AST_EXPR_LITERAL:
        if (expression->as.literal.kind == AST_LITERAL_TEMPLATE) {
            for (i = 0; i < expression->as.literal.as.template_parts.count; i++) {
                const AstTemplatePart *part =
                    &expression->as.literal.as.template_parts.items[i];

                if (part->kind == AST_TEMPLATE_PART_EXPRESSION &&
                    !tc_validate_default_parameter_references(checker,
                                                              part->as.expression,
                                                              parameters,
                                                              parameter_index,
                                                              parameter)) {
                    return false;
                }
            }
        }
        return true;

    case AST_EXPR_GROUPING:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.grouping.inner,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_POST_INCREMENT:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.post_increment.operand,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_POST_DECREMENT:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.post_decrement.operand,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_DISCARD:
        return true;

    case AST_EXPR_MEMORY_OP:
        for (i = 0; i < expression->as.memory_op.arguments.count; i++) {
            if (!tc_validate_default_parameter_references(checker,
                                                          expression->as.memory_op.arguments.items[i],
                                                          parameters,
                                                          parameter_index,
                                                          parameter)) {
                return false;
            }
        }
        return true;

    case AST_EXPR_SPAWN:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.spawn.callable,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);
    }

    return true;
}

bool tc_check_parameter_defaults(TypeChecker *checker,
                                 const AstParameterList *parameters) {
    size_t i;
    bool saw_default = false;

    if (!checker || !parameters) {
        return false;
    }

    for (i = 0; i < parameters->count; i++) {
        const AstParameter *parameter = &parameters->items[i];
        CheckedType parameter_type;

#include "type_checker_lambda_p2.inc"
