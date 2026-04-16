#include "machine_internal.h"

#include <stdlib.h>
#include <string.h>

bool mc_emit_instruction_runtime(MachineBuildContext *context,
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
    case CODEGEN_RUNTIME_CLOSURE_NEW: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        size_t i;
        bool ok = true;

        for (i = 0; i < instruction->as.closure.capture_count; i++) {
            char *slot = NULL;
            char *value = NULL;

            ok = mc_format_helper_slot_operand(i, &slot) &&
                 mc_format_operand(td, lir_unit, codegen_unit, instruction->as.closure.captures[i], &value) &&
                 mc_emit_move_to_destination(context, block, slot, value);
            free(slot);
            free(value);
            if (!ok) {
                return false;
            }
        }

        {
            char *unit_label = NULL;
            bool pointer_ok;

            if (!mc_format_code_label_operand(instruction->as.closure.unit_name, &unit_label)) {
                return false;
            }
            pointer_ok = instruction->as.closure.capture_count > 0
                ? mc_append_line(context, block, "lea %s, helper(0)", arg2)
                : mc_append_line(context, block, "mov %s, null", arg2);
            ok = mc_append_line(context, block, "mov %s, %s", arg0, unit_label) &&
                 mc_append_line(context,
                                block,
                                "mov %s, %zu",
                                arg1,
                                instruction->as.closure.capture_count) &&
                 pointer_ok &&
                 mc_emit_runtime_helper_call(context,
                                             codegen_unit,
                                             signature,
                                             block,
                                             false) &&
                 mc_emit_store_vreg(context,
                                    codegen_unit,
                                    instruction->as.closure.dest_vreg,
                                    block,
                                    ret);
            free(unit_label);
            return ok;
        }
    }
    case CODEGEN_RUNTIME_CALL_CALLABLE: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        char *callee = NULL;
        bool ok;

        if (!mc_format_operand(td, lir_unit, codegen_unit, instruction->as.call.callee, &callee)) {
            return false;
        }
        ok = mc_append_line(context, block, "mov %s, %s", arg0, callee) &&
             mc_append_line(context, block, "mov %s, %zu", arg1, instruction->as.call.argument_count) &&
             (instruction->as.call.argument_count > 0
                  ? mc_append_line(context, block, "lea %s, helper(0)", arg2)
                  : mc_append_line(context, block, "mov %s, null", arg2)) &&
             mc_emit_runtime_helper_call(context, codegen_unit, signature, block, false);
        free(callee);
        if (!ok) {
            return false;
        }
        if (instruction->as.call.has_result) {
            return mc_emit_store_vreg(context,
                                      codegen_unit,
                                      instruction->as.call.dest_vreg,
                                      block,
                                      ret);
        }
        return true;
    }
    case CODEGEN_RUNTIME_MEMBER_LOAD: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        char *target = NULL;
        char *member = NULL;
        bool ok;

        if (!mc_format_operand(td, lir_unit, codegen_unit, instruction->as.member.target, &target) ||
            !mc_format_member_symbol_operand(instruction->as.member.member, &member)) {
            free(target);
            free(member);
            return false;
        }
        ok = mc_append_line(context, block, "mov %s, %s", arg0, target) &&
             mc_append_line(context, block, "mov %s, %s", arg1, member) &&
             mc_emit_runtime_helper_call(context, codegen_unit, signature, block, false) &&
             mc_emit_store_vreg(context,
                                codegen_unit,
                                instruction->as.member.dest_vreg,
                                block,
                                ret);
        free(target);
        free(member);
        return ok;
    }
    case CODEGEN_RUNTIME_INDEX_LOAD: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        char *target = NULL;
        char *index = NULL;
        bool ok;

        if (!mc_format_operand(td, lir_unit, codegen_unit, instruction->as.index_load.target, &target) ||
            !mc_format_operand(td, lir_unit, codegen_unit, instruction->as.index_load.index, &index)) {
            free(target);
            free(index);
            return false;
        }
        ok = mc_append_line(context, block, "mov %s, %s", arg0, target) &&
             mc_append_line(context, block, "mov %s, %s", arg1, index) &&
             mc_emit_runtime_helper_call(context, codegen_unit, signature, block, false) &&
             mc_emit_store_vreg(context,
                                codegen_unit,
                                instruction->as.index_load.dest_vreg,
                                block,
                                ret);
        free(target);
        free(index);
        return ok;
    }
    case CODEGEN_RUNTIME_ARRAY_LITERAL: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        size_t i;
        bool ok = true;

        for (i = 0; i < instruction->as.array_literal.element_count; i++) {
            char *slot = NULL;
            char *value = NULL;

            ok = mc_format_helper_slot_operand(i, &slot) &&
                 mc_format_operand(td, lir_unit, codegen_unit, instruction->as.array_literal.elements[i], &value) &&
                 mc_emit_move_to_destination(context, block, slot, value);
            free(slot);
            free(value);
            if (!ok) {
                return false;
            }
        }
        ok = mc_append_line(context,
                            block,
                            "mov %s, %zu",
                            arg0,
                            instruction->as.array_literal.element_count) &&
             (instruction->as.array_literal.element_count > 0
                  ? mc_append_line(context, block, "lea %s, helper(0)", arg1)
                  : mc_append_line(context, block, "mov %s, null", arg1)) &&
             mc_emit_runtime_helper_call(context, codegen_unit, signature, block, false) &&
             mc_emit_store_vreg(context,
                                codegen_unit,
                                instruction->as.array_literal.dest_vreg,
                                block,
                                ret);
        return ok;
    }
    case CODEGEN_RUNTIME_UNION_NEW: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        char *type_desc = NULL;
        bool ok;

        if (!mc_format_union_type_descriptor_operand(instruction, &type_desc)) {
            return false;
        }
        ok = mc_append_line(context, block, "mov %s, %s", arg0, type_desc) &&
             mc_append_line(context, block, "mov %s, %zu", arg1,
                            instruction->as.union_new.variant_index);
        free(type_desc);
        if (!ok) {
            return false;
        }
        if (instruction->as.union_new.has_payload) {
            char *payload = NULL;

            ok = mc_format_operand(td, lir_unit, codegen_unit,
                                   instruction->as.union_new.payload, &payload) &&
                 mc_append_line(context, block, "mov %s, %s", arg2, payload);
            free(payload);
        } else {
            ok = mc_append_line(context, block, "mov %s, null", arg2);
        }
        return ok &&
               mc_emit_runtime_helper_call(context, codegen_unit, signature, block, false) &&
               mc_emit_store_vreg(context,
                                  codegen_unit,
                                  instruction->as.union_new.dest_vreg,
                                  block,
                                  ret);
    }
    default:
        return mc_emit_instruction_runtime_ext(context, lir_unit, codegen_unit,
                                               instruction, selected, block);
    }
}
