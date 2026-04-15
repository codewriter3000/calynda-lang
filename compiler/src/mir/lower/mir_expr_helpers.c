#include "mir_internal.h"

bool mr_lower_template_part_value(MirUnitBuildContext *context,
                                  const HirExpression *expression,
                                  MirValue *value) {
    MirInstruction instruction;

    if (!context || !expression || !value) {
        return false;
    }

    if (!expression->is_callable || expression->callable_signature.parameter_count != 0) {
        return mr_lower_expression(context, expression, value);
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_CALL;
    if (!mr_lower_expression(context, expression, &instruction.as.call.callee)) {
        return false;
    }
    instruction.as.call.has_result = (expression->type.kind != CHECKED_TYPE_VOID);
    if (instruction.as.call.has_result) {
        instruction.as.call.dest_temp = context->unit->next_temp_index++;
    }

    if (!mr_current_block(context)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Internal error: missing current MIR block after lowering template auto-call.");
        return false;
    }
    if (!mr_append_instruction(mr_current_block(context), instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR template auto-calls.");
        return false;
    }

    if (instruction.as.call.has_result) {
        value->kind = MIR_VALUE_TEMP;
        value->type = expression->type;
        value->as.temp_index = instruction.as.call.dest_temp;
    } else {
        *value = mr_invalid_value();
        value->type = expression->type;
    }
    return true;
}

bool mr_lower_template_expression(MirUnitBuildContext *context,
                                  const HirExpression *expression,
                                  MirValue *value) {
    MirInstruction instruction;
    size_t i;

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_TEMPLATE;
    instruction.as.template_literal.dest_temp = context->unit->next_temp_index++;
    if (expression->as.template_parts.count > 0) {
        instruction.as.template_literal.parts = calloc(expression->as.template_parts.count,
                                                       sizeof(*instruction.as.template_literal.parts));
        if (!instruction.as.template_literal.parts) {
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR templates.");
            return false;
        }
    }
    instruction.as.template_literal.part_count = expression->as.template_parts.count;
    for (i = 0; i < expression->as.template_parts.count; i++) {
        instruction.as.template_literal.parts[i].kind =
            expression->as.template_parts.items[i].kind == AST_TEMPLATE_PART_TEXT
                ? MIR_TEMPLATE_PART_TEXT
                : MIR_TEMPLATE_PART_VALUE;
        if (instruction.as.template_literal.parts[i].kind == MIR_TEMPLATE_PART_TEXT) {
            instruction.as.template_literal.parts[i].as.text =
                ast_copy_text(expression->as.template_parts.items[i].as.text);
            if (!instruction.as.template_literal.parts[i].as.text) {
                mr_instruction_free(&instruction);
                mr_set_error(context->build,
                              expression->source_span,
                              NULL,
                              "Out of memory while lowering MIR templates.");
                return false;
            }
        } else if (!mr_lower_template_part_value(context,
                                              expression->as.template_parts.items[i].as.expression,
                                              &instruction.as.template_literal.parts[i].as.value)) {
            mr_instruction_free(&instruction);
            return false;
        }
    }

    if (!mr_current_block(context) ||
        !mr_append_instruction(mr_current_block(context), instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR templates.");
        return false;
    }

    value->kind = MIR_VALUE_TEMP;
    value->type = expression->type;
    value->as.temp_index = instruction.as.template_literal.dest_temp;
    return true;
}

bool mr_lower_member_expression(MirUnitBuildContext *context,
                                const HirExpression *expression,
                                MirValue *value) {
    MirInstruction instruction;
    const char *union_name = NULL;
    const char *variant_name = NULL;

    /* Non-payload union variant: Option.None → union_new with no payload */
    if (!expression->is_callable &&
        mr_is_union_variant_member(expression, &union_name, &variant_name)) {
        size_t variant_index = 0;
        size_t variant_count = 0;

        if (!mr_find_hir_union_variant(context->build, union_name, variant_name,
                                    &variant_index, &variant_count)) {
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Internal error: union variant '%s.%s' not found in HIR.",
                          union_name, variant_name);
            return false;
        }

        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_UNION_NEW;
        instruction.as.union_new.dest_temp = context->unit->next_temp_index++;
        instruction.as.union_new.union_name = ast_copy_text(union_name);
        instruction.as.union_new.variant_index = variant_index;
        instruction.as.union_new.variant_count = variant_count;
        instruction.as.union_new.has_payload = false;
        memset(&instruction.as.union_new.payload, 0, sizeof(MirValue));

        if (!instruction.as.union_new.union_name) {
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR union variant.");
            return false;
        }

        if (!mr_current_block(context) ||
            !mr_append_instruction(mr_current_block(context), instruction)) {
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR union variant.");
            return false;
        }

        value->kind = MIR_VALUE_TEMP;
        value->type = expression->type;
        value->as.temp_index = instruction.as.union_new.dest_temp;
        return true;
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_MEMBER;
    instruction.as.member.dest_temp = context->unit->next_temp_index++;
    if (!mr_lower_expression(context, expression->as.member.target, &instruction.as.member.target)) {
        return false;
    }
    instruction.as.member.member = ast_copy_text(expression->as.member.member);
    if (!instruction.as.member.member) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR member expressions.");
        return false;
    }
    if (!mr_current_block(context) ||
        !mr_append_instruction(mr_current_block(context), instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR member expressions.");
        return false;
    }

    value->kind = MIR_VALUE_TEMP;
    value->type = expression->type;
    value->as.temp_index = instruction.as.member.dest_temp;
    return true;
}

bool mr_lower_index_expression(MirUnitBuildContext *context,
                               const HirExpression *expression,
                               MirValue *value) {
    MirInstruction instruction;

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_INDEX_LOAD;
    instruction.as.index_load.dest_temp = context->unit->next_temp_index++;
    if (!mr_lower_expression(context, expression->as.index.target, &instruction.as.index_load.target) ||
        !mr_lower_expression(context, expression->as.index.index, &instruction.as.index_load.index)) {
        mr_instruction_free(&instruction);
        return false;
    }
    if (!mr_current_block(context) ||
        !mr_append_instruction(mr_current_block(context), instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR index expressions.");
        return false;
    }

    value->kind = MIR_VALUE_TEMP;
    value->type = expression->type;
    value->as.temp_index = instruction.as.index_load.dest_temp;
    return true;
}

