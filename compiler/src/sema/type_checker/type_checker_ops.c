#include "type_checker_internal.h"

bool tc_check_binary_operator(TypeChecker *checker,
                              const AstExpression *expression,
                              AstBinaryOperator operator,
                              CheckedType left_type,
                              CheckedType right_type,
                              CheckedType *result_type) {
    char left_text[64];
    char right_text[64];

    if (!result_type) {
        return false;
    }

    switch (operator) {
    case AST_BINARY_OP_LOGICAL_OR:
    case AST_BINARY_OP_LOGICAL_AND:
        if (tc_checked_type_is_bool(left_type) && tc_checked_type_is_bool(right_type)) {
            *result_type = tc_checked_type_value(AST_PRIMITIVE_BOOL, 0);
            return true;
        }
        break;

    case AST_BINARY_OP_BIT_OR:
    case AST_BINARY_OP_BIT_NAND:
    case AST_BINARY_OP_BIT_XOR:
    case AST_BINARY_OP_BIT_XNOR:
    case AST_BINARY_OP_BIT_AND:
        if (tc_checked_type_is_integral(left_type) && tc_checked_type_is_integral(right_type)) {
            *result_type = tc_promote_numeric_types(left_type, right_type);
            return true;
        }
        break;

    case AST_BINARY_OP_EQUAL:
    case AST_BINARY_OP_NOT_EQUAL:
        if (tc_checked_type_equals(left_type, right_type) ||
            (tc_checked_type_is_numeric(left_type) && tc_checked_type_is_numeric(right_type)) ||
            ((left_type.kind == CHECKED_TYPE_NULL && tc_checked_type_is_reference_like(right_type)) ||
             (right_type.kind == CHECKED_TYPE_NULL && tc_checked_type_is_reference_like(left_type))) ||
            (left_type.kind == CHECKED_TYPE_EXTERNAL || right_type.kind == CHECKED_TYPE_EXTERNAL)) {
            *result_type = tc_checked_type_value(AST_PRIMITIVE_BOOL, 0);
            return true;
        }
        break;

    case AST_BINARY_OP_LESS:
    case AST_BINARY_OP_GREATER:
    case AST_BINARY_OP_LESS_EQUAL:
    case AST_BINARY_OP_GREATER_EQUAL:
        if (tc_checked_type_is_numeric(left_type) && tc_checked_type_is_numeric(right_type)) {
            *result_type = tc_checked_type_value(AST_PRIMITIVE_BOOL, 0);
            return true;
        }
        break;

    case AST_BINARY_OP_SHIFT_LEFT:
    case AST_BINARY_OP_SHIFT_RIGHT:
        if (tc_checked_type_is_integral(left_type) && tc_checked_type_is_integral(right_type)) {
            *result_type = left_type;
            return true;
        }
        break;

    case AST_BINARY_OP_ADD:
        if (tc_checked_type_is_string(left_type) && tc_checked_type_is_string(right_type)) {
            *result_type = tc_checked_type_value(AST_PRIMITIVE_STRING, 0);
            return true;
        }
        if (tc_checked_type_is_numeric(left_type) && tc_checked_type_is_numeric(right_type)) {
            *result_type = tc_promote_numeric_types(left_type, right_type);
            return true;
        }
        break;

    case AST_BINARY_OP_SUBTRACT:
    case AST_BINARY_OP_MULTIPLY:
    case AST_BINARY_OP_DIVIDE:
        if (tc_checked_type_is_numeric(left_type) && tc_checked_type_is_numeric(right_type)) {
            *result_type = tc_promote_numeric_types(left_type, right_type);
            return true;
        }
        break;

    case AST_BINARY_OP_MODULO:
        if (tc_checked_type_is_integral(left_type) && tc_checked_type_is_integral(right_type)) {
            *result_type = tc_promote_numeric_types(left_type, right_type);
            return true;
        }
        break;
    }

    checked_type_to_string(left_type, left_text, sizeof(left_text));
    checked_type_to_string(right_type, right_text, sizeof(right_text));
    tc_set_error_at(checker,
                    expression->source_span,
                    NULL,
                    "Operator '%s' cannot be applied to types %s and %s.",
                    tc_binary_operator_name(operator),
                    left_text,
                    right_text);
    return false;
}

bool tc_map_compound_assignment(AstAssignmentOperator operator,
                                AstBinaryOperator *binary_operator) {
    if (!binary_operator) {
        return false;
    }

    switch (operator) {
    case AST_ASSIGN_OP_ADD:
        *binary_operator = AST_BINARY_OP_ADD;
        return true;
    case AST_ASSIGN_OP_SUBTRACT:
        *binary_operator = AST_BINARY_OP_SUBTRACT;
        return true;
    case AST_ASSIGN_OP_MULTIPLY:
        *binary_operator = AST_BINARY_OP_MULTIPLY;
        return true;
    case AST_ASSIGN_OP_DIVIDE:
        *binary_operator = AST_BINARY_OP_DIVIDE;
        return true;
    case AST_ASSIGN_OP_MODULO:
        *binary_operator = AST_BINARY_OP_MODULO;
        return true;
    case AST_ASSIGN_OP_BIT_AND:
        *binary_operator = AST_BINARY_OP_BIT_AND;
        return true;
    case AST_ASSIGN_OP_BIT_OR:
        *binary_operator = AST_BINARY_OP_BIT_OR;
        return true;
    case AST_ASSIGN_OP_BIT_XOR:
        *binary_operator = AST_BINARY_OP_BIT_XOR;
        return true;
    case AST_ASSIGN_OP_SHIFT_LEFT:
        *binary_operator = AST_BINARY_OP_SHIFT_LEFT;
        return true;
    case AST_ASSIGN_OP_SHIFT_RIGHT:
        *binary_operator = AST_BINARY_OP_SHIFT_RIGHT;
        return true;
    case AST_ASSIGN_OP_ASSIGN:
        return false;
    }

    return false;
}

bool tc_expression_is_assignment_target(TypeChecker *checker,
                                        const AstExpression *expression,
                                        const Symbol **root_symbol) {
    if (root_symbol) {
        *root_symbol = NULL;
    }

    if (!checker || !expression) {
        return false;
    }

    switch (expression->kind) {
    case AST_EXPR_IDENTIFIER:
        {
            const Symbol *symbol = symbol_table_resolve_identifier(checker->symbols, expression);

            if (root_symbol) {
                *root_symbol = symbol;
            }

            return symbol &&
                   (symbol->kind == SYMBOL_KIND_TOP_LEVEL_BINDING ||
                    symbol->kind == SYMBOL_KIND_PARAMETER ||
                    symbol->kind == SYMBOL_KIND_LOCAL);
        }

    case AST_EXPR_INDEX:
        {
            const TypeCheckInfo *target_info = type_checker_get_expression_info(checker,
                                                                                expression->as.index.target);

            if (!target_info) {
                return false;
            }

            if (tc_type_check_source_type(target_info).kind == CHECKED_TYPE_EXTERNAL) {
                return true;
            }

            return tc_expression_is_assignment_target(checker,
                                                      expression->as.index.target,
                                                      root_symbol);
        }

    case AST_EXPR_MEMBER:
        {
            const TypeCheckInfo *target_info = type_checker_get_expression_info(checker,
                                                                                expression->as.member.target);

            if (!target_info) {
                return false;
            }

            if (tc_type_check_source_type(target_info).kind == CHECKED_TYPE_EXTERNAL) {
                return true;
            }

            return tc_expression_is_assignment_target(checker,
                                                      expression->as.member.target,
                                                      root_symbol);
        }

    case AST_EXPR_GROUPING:
        return tc_expression_is_assignment_target(checker,
                                                  expression->as.grouping.inner,
                                                  root_symbol);

    case AST_EXPR_DISCARD:
        return true;

    default:
        return false;
    }
}
