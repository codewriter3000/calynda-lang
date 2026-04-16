#include "lir.h"
#include "lir_internal.h"
#include <stdlib.h>
#include <string.h>

bool lr_lower_mir_instr_stores(LirBuildContext *context,
                               const MirUnit *mir_unit,
                               const LirUnit *unit,
                               LirBasicBlock *block,
                               const MirInstruction *instruction) {
    LirInstruction lowered;
    size_t i;

    (void)mir_unit;

    switch (instruction->kind) {
    case MIR_INSTR_STORE_LOCAL:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_STORE_SLOT;
        lowered.as.store_slot.slot_index = instruction->as.store_local.local_index;
        if (!lr_operand_from_mir_value(context, unit,
                                       instruction->as.store_local.value,
                                       &lowered.as.store_slot.value)) {
            lr_instruction_free(&lowered);
            return false;
        }
        if (!lr_append_instruction(block, lowered)) {
            lr_instruction_free(&lowered);
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR slot stores.");
            return false;
        }
        return true;

    case MIR_INSTR_STORE_GLOBAL:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_STORE_GLOBAL;
        lowered.as.store_global.global_name = ast_copy_text(instruction->as.store_global.global_name);
        if (!lowered.as.store_global.global_name ||
            !lr_operand_from_mir_value(context, unit,
                                       instruction->as.store_global.value,
                                       &lowered.as.store_global.value)) {
            lr_instruction_free(&lowered);
            if (!context->program->has_error) {
                lr_set_error(context, (AstSourceSpan){0}, NULL,
                             "Out of memory while lowering LIR global stores.");
            }
            return false;
        }
        if (!lr_append_instruction(block, lowered)) {
            lr_instruction_free(&lowered);
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR global stores.");
            return false;
        }
        return true;

    case MIR_INSTR_STORE_INDEX:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_STORE_INDEX;
        if (!lr_operand_from_mir_value(context, unit,
                                       instruction->as.store_index.target,
                                       &lowered.as.store_index.target) ||
            !lr_operand_from_mir_value(context, unit,
                                       instruction->as.store_index.index,
                                       &lowered.as.store_index.index) ||
            !lr_operand_from_mir_value(context, unit,
                                       instruction->as.store_index.value,
                                       &lowered.as.store_index.value)) {
            lr_instruction_free(&lowered);
            return false;
        }
        if (!lr_append_instruction(block, lowered)) {
            lr_instruction_free(&lowered);
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR index stores.");
            return false;
        }
        return true;

    case MIR_INSTR_STORE_MEMBER:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_STORE_MEMBER;
        lowered.as.store_member.member = ast_copy_text(instruction->as.store_member.member);
        if (!lowered.as.store_member.member ||
            !lr_operand_from_mir_value(context, unit,
                                       instruction->as.store_member.target,
                                       &lowered.as.store_member.target) ||
            !lr_operand_from_mir_value(context, unit,
                                       instruction->as.store_member.value,
                                       &lowered.as.store_member.value)) {
            lr_instruction_free(&lowered);
            if (!context->program->has_error) {
                lr_set_error(context, (AstSourceSpan){0}, NULL,
                             "Out of memory while lowering LIR member stores.");
            }
            return false;
        }
        if (!lr_append_instruction(block, lowered)) {
            lr_instruction_free(&lowered);
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR member stores.");
            return false;
        }
        return true;

    case MIR_INSTR_HETERO_ARRAY_NEW:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_HETERO_ARRAY_NEW;
        lowered.as.hetero_array_new.dest_vreg = instruction->as.hetero_array_new.dest_temp;
        lowered.as.hetero_array_new.type_desc.name = ast_copy_text(
            instruction->as.hetero_array_new.type_desc.name);
        lowered.as.hetero_array_new.type_desc.generic_param_count =
            instruction->as.hetero_array_new.type_desc.generic_param_count;
        lowered.as.hetero_array_new.type_desc.variant_count =
            instruction->as.hetero_array_new.type_desc.variant_count;
        lowered.as.hetero_array_new.element_count = instruction->as.hetero_array_new.element_count;
        if (!lowered.as.hetero_array_new.type_desc.name) {
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR hetero array.");
            return false;
        }
        if (instruction->as.hetero_array_new.type_desc.generic_param_count > 0) {
            lowered.as.hetero_array_new.type_desc.generic_param_tags = calloc(
                instruction->as.hetero_array_new.type_desc.generic_param_count,
                sizeof(*lowered.as.hetero_array_new.type_desc.generic_param_tags));
            if (!lowered.as.hetero_array_new.type_desc.generic_param_tags) {
                lr_instruction_free(&lowered);
                lr_set_error(context, (AstSourceSpan){0}, NULL,
                             "Out of memory while lowering LIR hetero array.");
                return false;
            }
            for (i = 0; i < instruction->as.hetero_array_new.type_desc.generic_param_count; i++) {
                ((CalyndaRtTypeTag *)lowered.as.hetero_array_new.type_desc.generic_param_tags)[i] =
                    instruction->as.hetero_array_new.type_desc.generic_param_tags[i];
            }
        }
        if (instruction->as.hetero_array_new.element_count > 0) {
            lowered.as.hetero_array_new.elements = calloc(
                instruction->as.hetero_array_new.element_count,
                sizeof(*lowered.as.hetero_array_new.elements));
            lowered.as.hetero_array_new.type_desc.variant_payload_tags = calloc(
                instruction->as.hetero_array_new.element_count,
                sizeof(*lowered.as.hetero_array_new.type_desc.variant_payload_tags));
            if (!lowered.as.hetero_array_new.elements ||
                !lowered.as.hetero_array_new.type_desc.variant_payload_tags) {
                lr_instruction_free(&lowered);
                lr_set_error(context, (AstSourceSpan){0}, NULL,
                             "Out of memory while lowering LIR hetero array.");
                return false;
            }
            for (i = 0; i < instruction->as.hetero_array_new.element_count; i++) {
                if (!lr_operand_from_mir_value(context, unit,
                                               instruction->as.hetero_array_new.elements[i],
                                               &lowered.as.hetero_array_new.elements[i])) {
                    lr_instruction_free(&lowered);
                    return false;
                }
                ((CalyndaRtTypeTag *)lowered.as.hetero_array_new.type_desc.variant_payload_tags)[i] =
                    instruction->as.hetero_array_new.type_desc.variant_payload_tags[i];
            }
        }
        if (!lr_append_instruction(block, lowered)) {
            lr_instruction_free(&lowered);
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR hetero array.");
            return false;
        }
        return true;

    case MIR_INSTR_UNION_NEW:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_UNION_NEW;
        lowered.as.union_new.dest_vreg = instruction->as.union_new.dest_temp;
        lowered.as.union_new.type_desc.name = ast_copy_text(
            instruction->as.union_new.type_desc.name);
        lowered.as.union_new.type_desc.generic_param_count = instruction->as.union_new.type_desc.generic_param_count;
        lowered.as.union_new.type_desc.variant_count = instruction->as.union_new.type_desc.variant_count;
        lowered.as.union_new.variant_index = instruction->as.union_new.variant_index;
        lowered.as.union_new.has_payload = instruction->as.union_new.has_payload;
        if (!lowered.as.union_new.type_desc.name) {
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR union new.");
            return false;
        }
        if (instruction->as.union_new.type_desc.generic_param_count > 0) {
            lowered.as.union_new.type_desc.generic_param_tags = calloc(
                instruction->as.union_new.type_desc.generic_param_count,
                sizeof(*lowered.as.union_new.type_desc.generic_param_tags));
            if (!lowered.as.union_new.type_desc.generic_param_tags) {
                lr_instruction_free(&lowered);
                lr_set_error(context, (AstSourceSpan){0}, NULL,
                             "Out of memory while lowering LIR union metadata.");
                return false;
            }
            for (i = 0; i < instruction->as.union_new.type_desc.generic_param_count; i++) {
                ((CalyndaRtTypeTag *)lowered.as.union_new.type_desc.generic_param_tags)[i] =
                    instruction->as.union_new.type_desc.generic_param_tags[i];
            }
        }
        if (instruction->as.union_new.type_desc.variant_count > 0) {
            char **variant_names = calloc(instruction->as.union_new.type_desc.variant_count,
                                          sizeof(*variant_names));
            lowered.as.union_new.type_desc.variant_names = (const char *const *)variant_names;
            lowered.as.union_new.type_desc.variant_payload_tags = calloc(
                instruction->as.union_new.type_desc.variant_count,
                sizeof(*lowered.as.union_new.type_desc.variant_payload_tags));
            if (!variant_names || !lowered.as.union_new.type_desc.variant_payload_tags) {
                lr_instruction_free(&lowered);
                lr_set_error(context, (AstSourceSpan){0}, NULL,
                             "Out of memory while lowering LIR union metadata.");
                return false;
            }
            for (i = 0; i < instruction->as.union_new.type_desc.variant_count; i++) {
                variant_names[i] =
                    ast_copy_text(instruction->as.union_new.type_desc.variant_names[i]);
                ((CalyndaRtTypeTag *)lowered.as.union_new.type_desc.variant_payload_tags)[i] =
                    instruction->as.union_new.type_desc.variant_payload_tags[i];
                if (!variant_names[i]) {
                    lr_instruction_free(&lowered);
                    lr_set_error(context, (AstSourceSpan){0}, NULL,
                                 "Out of memory while lowering LIR union metadata.");
                    return false;
                }
            }
        }
        if (instruction->as.union_new.has_payload) {
            if (!lr_operand_from_mir_value(context, unit,
                                           instruction->as.union_new.payload,
                                           &lowered.as.union_new.payload)) {
                lr_instruction_free(&lowered);
                return false;
            }
        } else {
            memset(&lowered.as.union_new.payload, 0, sizeof(LirOperand));
        }
        if (!lr_append_instruction(block, lowered)) {
            lr_instruction_free(&lowered);
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR union new.");
            return false;
        }
        return true;

    default:
        return false;
    }
}
