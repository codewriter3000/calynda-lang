#include "lir.h"
#include "lir_internal.h"

#include <stdlib.h>
#include <string.h>

bool lr_lower_mir_instr_ext(LirBuildContext *context,
                            const MirUnit *mir_unit,
                            const LirUnit *unit,
                            LirBasicBlock *block,
                            const MirInstruction *instruction) {
    LirInstruction lowered;
    size_t i;

    (void)mir_unit;

    switch (instruction->kind) {
    case MIR_INSTR_CAST:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_CAST;
        lowered.as.cast.dest_vreg = instruction->as.cast.dest_temp;
        lowered.as.cast.target_type = instruction->as.cast.target_type;
        if (!lr_operand_from_mir_value(context, unit,
                                       instruction->as.cast.operand,
                                       &lowered.as.cast.operand)) {
            lr_instruction_free(&lowered);
            return false;
        }
        if (!lr_append_instruction(block, lowered)) {
            lr_instruction_free(&lowered);
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR casts.");
            return false;
        }
        return true;

    case MIR_INSTR_MEMBER:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_MEMBER;
        lowered.as.member.dest_vreg = instruction->as.member.dest_temp;
        lowered.as.member.member = ast_copy_text(instruction->as.member.member);
        if (!lowered.as.member.member ||
            !lr_operand_from_mir_value(context, unit,
                                       instruction->as.member.target,
                                       &lowered.as.member.target)) {
            lr_instruction_free(&lowered);
            if (!context->program->has_error) {
                lr_set_error(context, (AstSourceSpan){0}, NULL,
                             "Out of memory while lowering LIR member instructions.");
            }
            return false;
        }
        if (!lr_append_instruction(block, lowered)) {
            lr_instruction_free(&lowered);
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR member instructions.");
            return false;
        }
        return true;

    case MIR_INSTR_UNION_GET_TAG:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_UNION_GET_TAG;
        lowered.as.union_get_tag.dest_vreg = instruction->as.union_get_tag.dest_temp;
        if (!lr_operand_from_mir_value(context, unit,
                                       instruction->as.union_get_tag.target,
                                       &lowered.as.union_get_tag.target) ||
            !lr_append_instruction(block, lowered)) {
            lr_instruction_free(&lowered);
            return false;
        }
        return true;

    case MIR_INSTR_UNION_GET_PAYLOAD:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_UNION_GET_PAYLOAD;
        lowered.as.union_get_payload.dest_vreg = instruction->as.union_get_payload.dest_temp;
        if (!lr_operand_from_mir_value(context, unit,
                                       instruction->as.union_get_payload.target,
                                       &lowered.as.union_get_payload.target) ||
            !lr_append_instruction(block, lowered)) {
            lr_instruction_free(&lowered);
            return false;
        }
        return true;

    case MIR_INSTR_INDEX_LOAD:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_INDEX_LOAD;
        lowered.as.index_load.dest_vreg = instruction->as.index_load.dest_temp;
        if (!lr_operand_from_mir_value(context, unit,
                                       instruction->as.index_load.target,
                                       &lowered.as.index_load.target) ||
            !lr_operand_from_mir_value(context, unit,
                                       instruction->as.index_load.index,
                                       &lowered.as.index_load.index)) {
            lr_instruction_free(&lowered);
            return false;
        }
        if (!lr_append_instruction(block, lowered)) {
            lr_instruction_free(&lowered);
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR index loads.");
            return false;
        }
        return true;

    case MIR_INSTR_ARRAY_LITERAL:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_ARRAY_LITERAL;
        lowered.as.array_literal.dest_vreg = instruction->as.array_literal.dest_temp;
        if (instruction->as.array_literal.element_count > 0) {
            lowered.as.array_literal.elements = calloc(instruction->as.array_literal.element_count,
                                                       sizeof(*lowered.as.array_literal.elements));
            if (!lowered.as.array_literal.elements) {
                lr_set_error(context, (AstSourceSpan){0}, NULL,
                             "Out of memory while lowering LIR array literals.");
                return false;
            }
        }
        lowered.as.array_literal.element_count = instruction->as.array_literal.element_count;
        for (i = 0; i < instruction->as.array_literal.element_count; i++) {
            if (!lr_operand_from_mir_value(context, unit,
                                           instruction->as.array_literal.elements[i],
                                           &lowered.as.array_literal.elements[i])) {
                lr_instruction_free(&lowered);
                return false;
            }
        }
        if (!lr_append_instruction(block, lowered)) {
            lr_instruction_free(&lowered);
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR array literals.");
            return false;
        }
        return true;

    case MIR_INSTR_TEMPLATE:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_TEMPLATE;
        lowered.as.template_literal.dest_vreg = instruction->as.template_literal.dest_temp;
        if (instruction->as.template_literal.part_count > 0) {
            lowered.as.template_literal.parts = calloc(instruction->as.template_literal.part_count,
                                                       sizeof(*lowered.as.template_literal.parts));
            if (!lowered.as.template_literal.parts) {
                lr_set_error(context, (AstSourceSpan){0}, NULL,
                             "Out of memory while lowering LIR templates.");
                return false;
            }
        }
        lowered.as.template_literal.part_count = instruction->as.template_literal.part_count;
        for (i = 0; i < instruction->as.template_literal.part_count; i++) {
            lowered.as.template_literal.parts[i].kind =
                instruction->as.template_literal.parts[i].kind == MIR_TEMPLATE_PART_TEXT
                    ? LIR_TEMPLATE_PART_TEXT
                    : LIR_TEMPLATE_PART_VALUE;
            if (lowered.as.template_literal.parts[i].kind == LIR_TEMPLATE_PART_TEXT) {
                lowered.as.template_literal.parts[i].as.text =
                    ast_copy_text(instruction->as.template_literal.parts[i].as.text);
                if (!lowered.as.template_literal.parts[i].as.text) {
                    lr_instruction_free(&lowered);
                    lr_set_error(context, (AstSourceSpan){0}, NULL,
                                 "Out of memory while lowering LIR templates.");
                    return false;
                }
            } else if (!lr_operand_from_mir_value(context, unit,
                                                   instruction->as.template_literal.parts[i].as.value,
                                                   &lowered.as.template_literal.parts[i].as.value)) {
                lr_instruction_free(&lowered);
                return false;
            }
        }
        if (!lr_append_instruction(block, lowered)) {
            lr_instruction_free(&lowered);
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR templates.");
            return false;
        }
        return true;

    default:
        return false;
    }
}
