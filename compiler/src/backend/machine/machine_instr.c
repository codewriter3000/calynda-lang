#include "machine_internal.h"

#include <stdlib.h>
#include <string.h>

bool mc_emit_instruction(MachineBuildContext *context,
                         const LirUnit *lir_unit,
                         const CodegenUnit *codegen_unit,
                         const LirBasicBlock *lir_block,
                         const CodegenBlock *codegen_block,
                         size_t instruction_index,
                         MachineBlock *block) {
    const LirInstruction *instruction;
    const CodegenSelectedInstruction *selected;
    const TargetDescriptor *td = mc_target(context);

    if (!lir_unit || !codegen_unit || !lir_block || !codegen_block || !block) {
        return false;
    }
    if (instruction_index >= lir_block->instruction_count ||
        instruction_index >= codegen_block->instruction_count) {
        mc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Machine emission block shape mismatch.");
        return false;
    }

    instruction = &lir_block->instructions[instruction_index];
    selected = &codegen_block->instructions[instruction_index];

    if (selected->selection.kind == CODEGEN_SELECTION_DIRECT) {
        switch (selected->selection.as.direct_pattern) {
        case CODEGEN_DIRECT_ABI_ARG_MOVE: {
            char *destination = NULL;
            char *source = NULL;
            const TargetDescriptor *td = mc_target(context);
            bool ok;

            if (!mc_format_slot_operand(codegen_unit,
                                        instruction->as.incoming_arg.slot_index,
                                        &destination)) {
                return false;
            }
            if (instruction->as.incoming_arg.argument_index < td->arg_register_count) {
                source = ast_copy_text(target_register_name(td,
                    runtime_abi_get_helper_argument_register(context->program->target,
                                                             instruction->as.incoming_arg.argument_index)));
            } else {
                mc_format_incoming_arg_stack_operand(
                    instruction->as.incoming_arg.argument_index - td->arg_register_count,
                    &source);
            }
            ok = source != NULL && mc_emit_move_to_destination(context, block, destination, source);
            free(destination);
            free(source);
            return ok;
        }
        case CODEGEN_DIRECT_ABI_CAPTURE_LOAD: {
            char *destination = NULL;
            char *source = NULL;
            bool ok;

            if (!mc_format_slot_operand(codegen_unit,
                                        instruction->as.incoming_capture.slot_index,
                                        &destination) ||
                !mc_format_capture_operand(instruction->as.incoming_capture.capture_index, &source)) {
                free(destination);
                free(source);
                return false;
            }
            ok = mc_emit_move_to_destination(context, block, destination, source);
            free(destination);
            free(source);
            return ok;
        }
        case CODEGEN_DIRECT_ABI_OUTGOING_ARG: {
            const LirInstruction *call_instruction;
            const CodegenSelectedInstruction *call_selected;
            const TargetDescriptor *td = mc_target(context);
            char *source = NULL;
            char *destination = NULL;
            bool ok;

            call_instruction = mc_find_pending_call_instruction(lir_block, instruction_index, NULL);
            call_selected = mc_find_pending_call_selection(codegen_block, instruction_index, NULL);
            if (!call_instruction || !call_selected) {
                mc_set_error(context,
                             (AstSourceSpan){0},
                             NULL,
                             "Outgoing argument at index %zu is not followed by a call.",
                             instruction_index);
                return false;
            }
            if (!mc_format_operand(td, lir_unit,
                                   codegen_unit,
                                   instruction->as.outgoing_arg.value,
                                   &source)) {
                return false;
            }

            if ((call_selected->selection.kind == CODEGEN_SELECTION_DIRECT &&
                 call_selected->selection.as.direct_pattern == CODEGEN_DIRECT_CALL_GLOBAL) ||
                (call_selected->selection.kind == CODEGEN_SELECTION_RUNTIME &&
                 call_selected->selection.as.runtime_helper != CODEGEN_RUNTIME_CALL_CALLABLE)) {

                if (instruction->as.outgoing_arg.argument_index < td->arg_register_count) {
                    destination = ast_copy_text(target_register_name(td,
                        runtime_abi_get_helper_argument_register(context->program->target,
                                                                 instruction->as.outgoing_arg.argument_index)));
                } else {
                    destination = NULL;
                    mc_format_outgoing_arg_stack_operand(
                        instruction->as.outgoing_arg.argument_index - td->arg_register_count,
                        &destination);
                }
            } else if (call_selected->selection.kind == CODEGEN_SELECTION_RUNTIME &&
                       call_selected->selection.as.runtime_helper == CODEGEN_RUNTIME_CALL_CALLABLE) {
                destination = mc_copy_format("helper(%zu)", instruction->as.outgoing_arg.argument_index);
            } else {
                free(source);
                mc_set_error(context,
                             (AstSourceSpan){0},
                             NULL,
                             "Outgoing argument lowering only supports direct/global-style calls or runtime callable dispatch.");
                return false;
            }

            ok = destination != NULL && mc_emit_move_to_destination(context, block, destination, source);
            free(destination);
            free(source);
            return ok;
        }
        case CODEGEN_DIRECT_SCALAR_BINARY:
            return mc_emit_binary(context, lir_unit, codegen_unit, instruction, block);
        case CODEGEN_DIRECT_SCALAR_UNARY:
            return mc_emit_unary(context, lir_unit, codegen_unit, instruction, block);
        case CODEGEN_DIRECT_SCALAR_CAST:
            return mc_emit_cast(context, lir_unit, codegen_unit, instruction, block);
        case CODEGEN_DIRECT_CALL_GLOBAL:
            return mc_emit_call_global(context, lir_unit, codegen_unit, instruction, block);
        case CODEGEN_DIRECT_STORE_SLOT: {
            char *destination = NULL;
            char *source = NULL;
            bool ok;

            if (!mc_format_slot_operand(codegen_unit, instruction->as.store_slot.slot_index, &destination) ||
                !mc_format_operand(td, lir_unit, codegen_unit, instruction->as.store_slot.value, &source)) {
                free(destination);
                free(source);
                return false;
            }
            ok = mc_emit_move_to_destination(context, block, destination, source);
            free(destination);
            free(source);
            return ok;
        }
        case CODEGEN_DIRECT_STORE_GLOBAL: {
            char *source = NULL;
            char *destination = mc_copy_format("global(%s)", instruction->as.store_global.global_name);
            bool ok;

            if (!destination ||
                !mc_format_operand(td, lir_unit, codegen_unit, instruction->as.store_global.value, &source)) {
                free(destination);
                free(source);
                return false;
            }
            ok = mc_emit_move_to_destination(context, block, destination, source);
            free(destination);
            free(source);
            return ok;
        }
        case CODEGEN_DIRECT_RETURN:
        case CODEGEN_DIRECT_JUMP:
        case CODEGEN_DIRECT_BRANCH:
            mc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Direct terminator pattern reached machine instruction emission.");
            return false;
        }
    }

    return mc_emit_instruction_runtime(context, lir_unit, codegen_unit,
                                       instruction, selected, block);
}
