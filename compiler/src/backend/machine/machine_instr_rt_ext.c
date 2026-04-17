#include "machine_internal.h"

#include <stdlib.h>
#include <string.h>

bool mc_emit_instruction_runtime_ext(MachineBuildContext *context,
                                     const LirUnit *lir_unit,
                                     const CodegenUnit *codegen_unit,
                                     const LirInstruction *instruction,
                                     const CodegenSelectedInstruction *selected,
                                     MachineBlock *block) {
    const TargetDescriptor *td = mc_target(context);
    const char *arg0 = td->arg_registers[0].name;
    const char *arg1 = td->arg_registers[1].name;
    const char *arg2 = td->arg_registers[2].name;
    const char *ret  = td->return_register.name;

    switch (selected->selection.as.runtime_helper) {
    case CODEGEN_RUNTIME_TEMPLATE_BUILD: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        size_t i;
        bool ok = true;

        for (i = 0; i < instruction->as.template_literal.part_count; i++) {
            char *tag_slot = NULL;
            char *payload_slot = NULL;
            char *payload = NULL;

            if (!mc_format_helper_slot_operand(i * 2, &tag_slot) ||
                !mc_format_helper_slot_operand((i * 2) + 1, &payload_slot)) {
                free(tag_slot);
                free(payload_slot);
                return false;
            }
            if (instruction->as.template_literal.parts[i].kind == LIR_TEMPLATE_PART_TEXT) {
                ok = mc_format_template_text_operand(instruction->as.template_literal.parts[i].as.text, &payload) &&
                     mc_emit_move_to_destination(context, block, tag_slot, "tag(text)") &&
                     mc_emit_move_to_destination(context, block, payload_slot, payload);
            } else {
                ok = mc_format_operand(td, lir_unit,
                                       codegen_unit,
                                       instruction->as.template_literal.parts[i].as.value,
                                       &payload) &&
                     mc_emit_move_to_destination(context, block, tag_slot, "tag(value)") &&
                     mc_emit_move_to_destination(context, block, payload_slot, payload);
            }
            free(tag_slot);
            free(payload_slot);
            free(payload);
            if (!ok) {
                return false;
            }
        }

        ok = mc_append_line(context,
                            block,
                            "mov %s, %zu",
                            arg0,
                            instruction->as.template_literal.part_count) &&
             (instruction->as.template_literal.part_count > 0
                  ? mc_append_line(context, block, "lea %s, helper(0)", arg1)
                  : mc_append_line(context, block, "mov %s, null", arg1)) &&
             mc_emit_runtime_helper_call(context, codegen_unit, signature, block, false) &&
             mc_emit_store_vreg(context,
                                codegen_unit,
                                instruction->as.template_literal.dest_vreg,
                                block,
                                ret);
        return ok;
    }
    case CODEGEN_RUNTIME_STORE_INDEX: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        char *target = NULL;
        char *index = NULL;
        char *value = NULL;
        bool ok;

        if (!mc_format_operand(td, lir_unit, codegen_unit, instruction->as.store_index.target, &target) ||
            !mc_format_operand(td, lir_unit, codegen_unit, instruction->as.store_index.index, &index) ||
            !mc_format_operand(td, lir_unit, codegen_unit, instruction->as.store_index.value, &value)) {
            free(target);
            free(index);
            free(value);
            return false;
        }
        ok = mc_append_line(context, block, "mov %s, %s", arg0, target) &&
             mc_append_line(context, block, "mov %s, %s", arg1, index) &&
             mc_append_line(context, block, "mov %s, %s", arg2, value) &&
             mc_emit_runtime_helper_call(context, codegen_unit, signature, block, false);
        free(target);
        free(index);
        free(value);
        return ok;
    }
    case CODEGEN_RUNTIME_STORE_MEMBER: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        char *target = NULL;
        char *member = NULL;
        char *value = NULL;
        bool ok;

        if (!mc_format_operand(td, lir_unit, codegen_unit, instruction->as.store_member.target, &target) ||
            !mc_format_member_symbol_operand(instruction->as.store_member.member, &member) ||
            !mc_format_operand(td, lir_unit, codegen_unit, instruction->as.store_member.value, &value)) {
            free(target);
            free(member);
            free(value);
            return false;
        }
        ok = mc_append_line(context, block, "mov %s, %s", arg0, target) &&
             mc_append_line(context, block, "mov %s, %s", arg1, member) &&
             mc_append_line(context, block, "mov %s, %s", arg2, value) &&
             mc_emit_runtime_helper_call(context, codegen_unit, signature, block, false);
        free(target);
        free(member);
        free(value);
        return ok;
    }
    case CODEGEN_RUNTIME_CAST_VALUE: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        char *value = NULL;
        char type_tag[64];
        bool ok;

        if (!mc_format_operand(td, lir_unit, codegen_unit, instruction->as.cast.operand, &value) ||
            !runtime_abi_format_type_tag(instruction->as.cast.target_type, type_tag, sizeof(type_tag))) {
            free(value);
            return false;
        }
        ok = mc_append_line(context, block, "mov %s, %s", arg0, value) &&
             mc_append_line(context, block, "mov %s, %s", arg1, type_tag) &&
             mc_emit_runtime_helper_call(context, codegen_unit, signature, block, false) &&
             mc_emit_store_vreg(context,
                                codegen_unit,
                                instruction->as.cast.dest_vreg,
                                block,
                                ret);
        free(value);
        return ok;
    }
    case CODEGEN_RUNTIME_UNION_GET_TAG:
    case CODEGEN_RUNTIME_UNION_GET_PAYLOAD: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        char *target = NULL;
        bool ok;

        if (!mc_format_operand(td,
                               lir_unit,
                               codegen_unit,
                               selected->selection.as.runtime_helper == CODEGEN_RUNTIME_UNION_GET_TAG
                                   ? instruction->as.union_get_tag.target
                                   : instruction->as.union_get_payload.target,
                               &target)) {
            return false;
        }
        ok = mc_append_line(context, block, "mov %s, %s", arg0, target) &&
             mc_emit_runtime_helper_call(context, codegen_unit, signature, block, false) &&
             mc_emit_store_vreg(context,
                                codegen_unit,
                                selected->selection.as.runtime_helper == CODEGEN_RUNTIME_UNION_GET_TAG
                                    ? instruction->as.union_get_tag.dest_vreg
                                    : instruction->as.union_get_payload.dest_vreg,
                                block,
                                ret);
        free(target);
        return ok;
    }
    case CODEGEN_RUNTIME_HETERO_ARRAY_GET_TAG:
        mc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Union/hetero get-tag/get-payload helpers not yet wired through machine emit.");
        return false;
    case CODEGEN_RUNTIME_HETERO_ARRAY_NEW: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        size_t count = instruction->as.hetero_array_new.element_count;
        size_t i;
        char *type_desc = NULL;
        bool ok = true;

        for (i = 0; i < count; i++) {
            char *slot = NULL;
            char *value = NULL;

            ok = mc_format_helper_slot_operand(i, &slot) &&
                 mc_format_operand(td, lir_unit, codegen_unit,
                                   instruction->as.hetero_array_new.elements[i], &value) &&
                 mc_emit_move_to_destination(context, block, slot, value);
            free(slot);
            free(value);
            if (!ok) return false;
        }
           if (!mc_format_hetero_array_type_descriptor_operand(instruction, &type_desc)) {
              return false;
        }
           ok = mc_append_line(context, block, "mov %s, %s", arg0, type_desc) &&
               mc_append_line(context, block, "mov %s, %zu", arg1, count) &&
             (count > 0
                   ? mc_append_line(context, block, "lea %s, helper(0)", arg2)
                  : mc_append_line(context, block, "mov %s, null", arg2)) &&
             mc_emit_runtime_helper_call(context, codegen_unit, signature, block, false) &&
             mc_emit_store_vreg(context,
                                codegen_unit,
                                instruction->as.hetero_array_new.dest_vreg,
                                block,
                                ret);
           free(type_desc);
        return ok;
    }
    case CODEGEN_RUNTIME_THROW:
        mc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Throw helper should only be emitted from terminators.");
        return false;
    case CODEGEN_RUNTIME_TYPEOF:
    case CODEGEN_RUNTIME_ISINT:
    case CODEGEN_RUNTIME_ISFLOAT:
    case CODEGEN_RUNTIME_ISBOOL:
    case CODEGEN_RUNTIME_ISSTRING:
    case CODEGEN_RUNTIME_ISARRAY:
    case CODEGEN_RUNTIME_ISSAMETYPE: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);

        return signature &&
               mc_emit_runtime_helper_call(context, codegen_unit, signature, block, false) &&
               mc_emit_store_vreg(context,
                                  codegen_unit,
                                  instruction->as.call.dest_vreg,
                                  block,
                                  ret);
    }
    default:
        return false;
    }
}
