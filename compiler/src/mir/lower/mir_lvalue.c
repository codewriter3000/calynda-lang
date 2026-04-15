#include "mir_internal.h"

bool mr_map_compound_assignment(AstAssignmentOperator operator,
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

bool mr_lower_assignment_target(MirUnitBuildContext *context,
                                const HirExpression *expression,
                                MirLValue *lvalue) {
    if (!context || !expression || !lvalue) {
        return false;
    }
    memset(lvalue, 0, sizeof(*lvalue));
    lvalue->type = expression->type;
    switch (expression->kind) {
    case HIR_EXPR_SYMBOL:
        if (expression->as.symbol.kind == SYMBOL_KIND_PARAMETER ||
            expression->as.symbol.kind == SYMBOL_KIND_LOCAL) {
            size_t local_index = mr_find_local_index(context->unit,
                                                      expression->as.symbol.symbol);

            if (local_index == (size_t)-1) {
                mr_set_error(context->build,
                              expression->source_span,
                              NULL,
                              "Internal error: missing MIR assignment local for symbol '%s'.",
                              expression->as.symbol.name);
                return false;
            }

            lvalue->kind = MIR_LVALUE_LOCAL;
            lvalue->as.local_index = local_index;
            return true;
        }

        lvalue->kind = MIR_LVALUE_GLOBAL;
        lvalue->as.global_name = ast_copy_text(expression->as.symbol.name);
        if (!lvalue->as.global_name) {
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR assignment targets.");
            return false;
        }
        return true;

    case HIR_EXPR_INDEX:
        lvalue->kind = MIR_LVALUE_INDEX;
        if (!mr_lower_expression(context,
                              expression->as.index.target,
                              &lvalue->as.index.target) ||
            !mr_lower_expression(context,
                              expression->as.index.index,
                              &lvalue->as.index.index)) {
            mr_lvalue_free(lvalue);
            return false;
        }
        return true;

    case HIR_EXPR_MEMBER:
        lvalue->kind = MIR_LVALUE_MEMBER;
        if (!mr_lower_expression(context,
                              expression->as.member.target,
                              &lvalue->as.member.target)) {
            mr_lvalue_free(lvalue);
            return false;
        }
        lvalue->as.member.member = ast_copy_text(expression->as.member.member);
        if (!lvalue->as.member.member) {
            mr_lvalue_free(lvalue);
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR member targets.");
            return false;
        }
        return true;

    default:
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Internal error: unsupported MIR assignment target kind %d.",
                      expression->kind);
        return false;
    }
}

bool mr_load_lvalue_value(MirUnitBuildContext *context,
                          const MirLValue *lvalue,
                          AstSourceSpan source_span,
                          MirValue *value) {
    MirInstruction instruction;
    if (!context || !lvalue || !value) {
        return false;
    }

    *value = mr_invalid_value();
    switch (lvalue->kind) {
    case MIR_LVALUE_LOCAL:
        value->kind = MIR_VALUE_LOCAL;
        value->type = lvalue->type;
        value->as.local_index = lvalue->as.local_index;
        return true;

    case MIR_LVALUE_GLOBAL:
        return mr_value_from_global(context->build,
                                     lvalue->as.global_name,
                                     lvalue->type,
                                     value);
    case MIR_LVALUE_INDEX:
        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_INDEX_LOAD;
        instruction.as.index_load.dest_temp = context->unit->next_temp_index++;
        if (!mr_value_clone(context->build,
                             &lvalue->as.index.target,
                             &instruction.as.index_load.target) ||
            !mr_value_clone(context->build,
                             &lvalue->as.index.index,
                             &instruction.as.index_load.index)) {
            mr_instruction_free(&instruction);
            return false;
        }
        if (!mr_current_block(context) ||
            !mr_append_instruction(mr_current_block(context), instruction)) {
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                          source_span,
                          NULL,
                          "Out of memory while lowering MIR index loads.");
            return false;
        }
        value->kind = MIR_VALUE_TEMP;
        value->type = lvalue->type;
        value->as.temp_index = instruction.as.index_load.dest_temp;
        return true;

    case MIR_LVALUE_MEMBER:
        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_MEMBER;
        instruction.as.member.dest_temp = context->unit->next_temp_index++;
        if (!mr_value_clone(context->build,
                             &lvalue->as.member.target,
                             &instruction.as.member.target)) {
            mr_instruction_free(&instruction);
            return false;
        }
        instruction.as.member.member = ast_copy_text(lvalue->as.member.member);
        if (!instruction.as.member.member) {
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                          source_span,
                          NULL,
                          "Out of memory while lowering MIR member loads.");
            return false;
        }
        if (!mr_current_block(context) ||
            !mr_append_instruction(mr_current_block(context), instruction)) {
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                          source_span,
                          NULL,
                          "Out of memory while lowering MIR member loads.");
            return false;
        }
        value->kind = MIR_VALUE_TEMP;
        value->type = lvalue->type;
        value->as.temp_index = instruction.as.member.dest_temp;
        return true;
    }

    return false;
}

bool mr_store_lvalue_value(MirUnitBuildContext *context,
                           const MirLValue *lvalue,
                           MirValue value,
                           AstSourceSpan source_span) {
    if (!context || !lvalue) {
        mr_value_free(&value);
        return false;
    }

    switch (lvalue->kind) {
    case MIR_LVALUE_LOCAL:
        return mr_append_store_local_instruction(context,
                                              lvalue->as.local_index,
                                              value,
                                              source_span);
    case MIR_LVALUE_GLOBAL:
        return mr_append_store_global_instruction(context,
                                               lvalue->as.global_name,
                                               value,
                                               source_span);
    case MIR_LVALUE_INDEX:
        return mr_append_store_index_instruction(context,
                                              &lvalue->as.index.target,
                                              &lvalue->as.index.index,
                                              value,
                                              source_span);
    case MIR_LVALUE_MEMBER:
        return mr_append_store_member_instruction(context,
                                               &lvalue->as.member.target,
                                               lvalue->as.member.member,
                                               value,
                                               source_span);
    }

    mr_value_free(&value);
    return false;
}
