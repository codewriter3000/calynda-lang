#include "machine_internal.h"

#include <stdlib.h>
#include <string.h>

const LirInstruction *mc_find_pending_call_instruction(const LirBasicBlock *block,
                                                       size_t instruction_index,
                                                       size_t *call_index_out) {
    size_t index;

    if (!block) {
        return NULL;
    }

    for (index = instruction_index + 1; index < block->instruction_count; index++) {
        if (block->instructions[index].kind == LIR_INSTR_OUTGOING_ARG) {
            continue;
        }
        if (block->instructions[index].kind == LIR_INSTR_CALL) {
            if (call_index_out) {
                *call_index_out = index;
            }
            return &block->instructions[index];
        }
        break;
    }

    return NULL;
}

const CodegenSelectedInstruction *mc_find_pending_call_selection(const CodegenBlock *block,
                                                                 size_t instruction_index,
                                                                 size_t *call_index_out) {
    size_t index;

    if (!block) {
        return NULL;
    }

    for (index = instruction_index + 1; index < block->instruction_count; index++) {
        if (block->instructions[index].kind == LIR_INSTR_OUTGOING_ARG) {
            continue;
        }
        if (block->instructions[index].kind == LIR_INSTR_CALL) {
            if (call_index_out) {
                *call_index_out = index;
            }
            return &block->instructions[index];
        }
        break;
    }

    return NULL;
}

bool mc_emit_call_global(MachineBuildContext *context,
                         const LirUnit *lir_unit,
                         const CodegenUnit *codegen_unit,
                         const LirInstruction *instruction,
                         MachineBlock *block) {
    const TargetDescriptor *td = mc_target(context);
    const char *call_mnemonic = mc_is_aarch64(context) ? "bl" : "call";
    bool ok;

    (void)lir_unit;

    ok = mc_emit_preserve_before_call(context, codegen_unit, block) &&
         mc_append_line(context,
                        block,
                        "%s %s",
                        call_mnemonic,
                        instruction->as.call.callee.as.global_name) &&
         mc_emit_preserve_after_call(context, codegen_unit, block);
    if (!ok) {
        return false;
    }
    if (instruction->as.call.has_result) {
        return mc_emit_store_vreg(context,
                                  codegen_unit,
                                  instruction->as.call.dest_vreg,
                                  block,
                                  td->return_register.name);
    }

    return true;
}

bool mc_emit_runtime_helper_call(MachineBuildContext *context,
                                 const CodegenUnit *codegen_unit,
                                 const RuntimeAbiHelperSignature *signature,
                                 MachineBlock *block,
                                 bool is_noreturn) {
    const char *call_mnemonic = mc_is_aarch64(context) ? "bl" : "call";

    if (!signature) {
        mc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Machine emission missing runtime helper signature.");
        return false;
    }

    if (!mc_emit_preserve_before_call(context, codegen_unit, block) ||
        !mc_append_line(context, block, "%s %s", call_mnemonic, signature->name)) {
        return false;
    }
    if (!is_noreturn && !mc_emit_preserve_after_call(context, codegen_unit, block)) {
        return false;
    }
    return true;
}

bool mc_emit_terminator(MachineBuildContext *context,
                        const LirUnit *lir_unit,
                        const CodegenUnit *codegen_unit,
                        const LirBasicBlock *lir_block,
                        const MachineUnit *machine_unit,
                        MachineBlock *block) {
    const LirTerminator *terminator;
    const CodegenSelectedTerminator *selected;
    const TargetDescriptor *td = mc_target(context);
    bool is_arm64 = mc_is_aarch64(context);
    const char *jmp_mnemonic = is_arm64 ? "b" : "jmp";
    const char *jne_mnemonic = is_arm64 ? "b.ne" : "jne";

    if (!lir_block || !machine_unit || !block) {
        return false;
    }

    terminator = &lir_block->terminator;
    selected = &codegen_unit->blocks[block - machine_unit->blocks].terminator;

    if (selected->selection.kind == CODEGEN_SELECTION_DIRECT) {
        switch (selected->selection.as.direct_pattern) {
        case CODEGEN_DIRECT_RETURN: {
            if (terminator->kind == LIR_TERM_RETURN && terminator->as.return_term.has_value) {
                char *value = NULL;
                bool ok;

                if (!mc_format_operand(td, lir_unit,
                                       codegen_unit,
                                       terminator->as.return_term.value,
                                       &value)) {
                    return false;
                }
                ok = mc_append_line(context, block, "mov %s, %s",
                                    td->return_register.name, value);
                free(value);
                if (!ok) {
                    return false;
                }
            }
            return mc_emit_return_epilogue(context, codegen_unit, machine_unit, block);
        }
        case CODEGEN_DIRECT_JUMP:
            return mc_append_line(context,
                                  block,
                                  "%s %s",
                                  jmp_mnemonic,
                                  codegen_unit->blocks[terminator->as.jump_term.target_block].label);
        case CODEGEN_DIRECT_BRANCH: {
            char *condition = NULL;
            bool ok;

            if (!mc_format_operand(td, lir_unit,
                                   codegen_unit,
                                   terminator->as.branch_term.condition,
                                   &condition)) {
                return false;
            }
            ok = mc_append_line(context, block, "cmp %s, bool(false)", condition) &&
                 mc_append_line(context,
                                block,
                                "%s %s",
                                jne_mnemonic,
                                codegen_unit->blocks[terminator->as.branch_term.true_block].label) &&
                 mc_append_line(context,
                                block,
                                "%s %s",
                                jmp_mnemonic,
                                codegen_unit->blocks[terminator->as.branch_term.false_block].label);
            free(condition);
            return ok;
        }
        case CODEGEN_DIRECT_ABI_ARG_MOVE:
        case CODEGEN_DIRECT_ABI_CAPTURE_LOAD:
        case CODEGEN_DIRECT_ABI_OUTGOING_ARG:
        case CODEGEN_DIRECT_SCALAR_BINARY:
        case CODEGEN_DIRECT_SCALAR_UNARY:
        case CODEGEN_DIRECT_SCALAR_CAST:
        case CODEGEN_DIRECT_CALL_GLOBAL:
        case CODEGEN_DIRECT_STORE_SLOT:
        case CODEGEN_DIRECT_STORE_GLOBAL:
            mc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Non-terminator direct pattern reached machine terminator emission.");
            return false;
        }
    }

    if (selected->selection.as.runtime_helper == CODEGEN_RUNTIME_THROW &&
        terminator->kind == LIR_TERM_THROW) {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             CODEGEN_RUNTIME_THROW);
        char *value = NULL;
        bool ok;

        if (!mc_format_operand(td, lir_unit, codegen_unit, terminator->as.throw_term.value, &value)) {
            return false;
        }
        ok = mc_append_line(context, block, "mov %s, %s",
                            td->arg_registers[0].name, value) &&
             mc_emit_runtime_helper_call(context, codegen_unit, signature, block, true);
        free(value);
        return ok;
    }

    mc_set_error(context,
                 (AstSourceSpan){0},
                 NULL,
                 "Unsupported machine terminator selection.");
    return false;
}
