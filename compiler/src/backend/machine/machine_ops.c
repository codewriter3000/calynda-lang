#include "machine_internal.h"

#include <stdlib.h>
#include <string.h>

bool mc_emit_binary(MachineBuildContext *context,
                    const LirUnit *lir_unit,
                    const CodegenUnit *codegen_unit,
                    const LirInstruction *instruction,
                    MachineBlock *block) {
    char *left = NULL;
    char *right = NULL;
    const CodegenVRegAllocation *allocation;
    const TargetDescriptor *td = mc_target(context);
    const char *work_reg;
    bool is_unsigned;
    bool is_arm64 = mc_is_aarch64(context);
    bool is_riscv64 = mc_is_riscv64(context);
    bool ok = false;

    if (!mc_format_operand(td, lir_unit, codegen_unit, instruction->as.binary.left, &left) ||
        !mc_format_operand(td, lir_unit, codegen_unit, instruction->as.binary.right, &right)) {
        goto cleanup;
    }
    if (instruction->as.binary.dest_vreg >= codegen_unit->vreg_count) {
        mc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Machine emission missing destination virtual register %zu.",
                     instruction->as.binary.dest_vreg);
        goto cleanup;
    }

    allocation = &codegen_unit->vreg_allocations[instruction->as.binary.dest_vreg];
    work_reg = allocation->location.kind == CODEGEN_VREG_REGISTER
        ? target_register_name(td, allocation->location.as.reg)
        : td->work_register.name;
    is_unsigned = mc_checked_type_is_unsigned(instruction->as.binary.left.type);

    if (is_arm64 || is_riscv64) {
        /* ARM64 / RISC-V 64: 3-operand instructions */
        switch (instruction->as.binary.operator) {
        case AST_BINARY_OP_ADD:
            ok = mc_append_line(context, block, "add %s, %s, %s", work_reg, left, right);
            break;
        case AST_BINARY_OP_SUBTRACT:
            ok = mc_append_line(context, block, "sub %s, %s, %s", work_reg, left, right);
            break;
        case AST_BINARY_OP_MULTIPLY:
            ok = mc_append_line(context, block, "mul %s, %s, %s", work_reg, left, right);
            break;
        case AST_BINARY_OP_DIVIDE:
            if (is_riscv64)
                ok = mc_append_line(context, block, "%s %s, %s, %s",
                                    is_unsigned ? "divu" : "div", work_reg, left, right);
            else
                ok = mc_append_line(context, block, "%s %s, %s, %s",
                                    is_unsigned ? "udiv" : "sdiv", work_reg, left, right);
            break;
        case AST_BINARY_OP_MODULO:
            if (is_riscv64) {
                ok = mc_append_line(context, block, "%s %s, %s, %s",
                                    is_unsigned ? "remu" : "rem", work_reg, left, right);
            } else {
                ok = mc_append_line(context, block, "%s %s, %s, %s",
                                    is_unsigned ? "udiv" : "sdiv", td->work_register.name, left, right) &&
                     mc_append_line(context, block, "msub %s, %s, %s, %s",
                                    work_reg, td->work_register.name, right, left);
            }
            break;
        case AST_BINARY_OP_BIT_AND:
            ok = mc_append_line(context, block, "and %s, %s, %s", work_reg, left, right);
            break;
        case AST_BINARY_OP_BIT_OR:
            ok = mc_append_line(context, block, "orr %s, %s, %s", work_reg, left, right);
            break;
        case AST_BINARY_OP_BIT_XOR:
            ok = mc_append_line(context, block, "eor %s, %s, %s", work_reg, left, right);
            break;
        case AST_BINARY_OP_BIT_NAND:
            ok = mc_append_line(context, block, "and %s, %s, %s", work_reg, left, right) &&
                 mc_append_line(context, block, "mvn %s, %s", work_reg, work_reg);
            break;
        case AST_BINARY_OP_BIT_XNOR:
            ok = mc_append_line(context, block, "eor %s, %s, %s", work_reg, left, right) &&
                 mc_append_line(context, block, "mvn %s, %s", work_reg, work_reg);
            break;
        case AST_BINARY_OP_SHIFT_LEFT:
            ok = mc_append_line(context, block, "lsl %s, %s, %s", work_reg, left, right);
            break;
        case AST_BINARY_OP_SHIFT_RIGHT:
            ok = mc_append_line(context, block, "%s %s, %s, %s",
                                is_unsigned ? "lsr" : "asr", work_reg, left, right);
            break;
        case AST_BINARY_OP_EQUAL:
        case AST_BINARY_OP_NOT_EQUAL:
        case AST_BINARY_OP_LESS:
        case AST_BINARY_OP_GREATER:
        case AST_BINARY_OP_LESS_EQUAL:
        case AST_BINARY_OP_GREATER_EQUAL:
            ok = mc_emit_compare_3op(context, block, work_reg, left, right,
                                     instruction->as.binary.operator,
                                     is_unsigned, is_riscv64);
            break;
        case AST_BINARY_OP_LOGICAL_AND:
        case AST_BINARY_OP_LOGICAL_OR:
            mc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Logical short-circuit operators should not reach direct binary machine emission.");
            goto cleanup;
        }
    } else {
        /* x86_64: 2-operand instructions */
        switch (instruction->as.binary.operator) {
    case AST_BINARY_OP_ADD:
        ok = mc_append_line(context, block, "mov %s, %s", work_reg, left) &&
             mc_append_line(context, block, "add %s, %s", work_reg, right);
        break;
    case AST_BINARY_OP_SUBTRACT:
        ok = mc_append_line(context, block, "mov %s, %s", work_reg, left) &&
             mc_append_line(context, block, "sub %s, %s", work_reg, right);
        break;
    case AST_BINARY_OP_MULTIPLY:
        ok = mc_append_line(context, block, "mov %s, %s", work_reg, left) &&
             mc_append_line(context, block, "imul %s, %s", work_reg, right);
        break;
    case AST_BINARY_OP_DIVIDE:
    case AST_BINARY_OP_MODULO:
        ok = mc_append_line(context, block, "mov rax, %s", left) &&
             (is_unsigned
                  ? mc_append_line(context, block, "xor rdx, rdx")
                  : mc_append_line(context, block, "cqo")) &&
             mc_append_line(context,
                            block,
                            "%s %s",
                            is_unsigned ? "div" : "idiv",
                            right) &&
             mc_append_line(context,
                            block,
                            "mov %s, %s",
                            work_reg,
                            instruction->as.binary.operator == AST_BINARY_OP_DIVIDE ? "rax" : "rdx");
        break;
    case AST_BINARY_OP_BIT_AND:
        ok = mc_append_line(context, block, "mov %s, %s", work_reg, left) &&
             mc_append_line(context, block, "and %s, %s", work_reg, right);
        break;
    case AST_BINARY_OP_BIT_OR:
        ok = mc_append_line(context, block, "mov %s, %s", work_reg, left) &&
             mc_append_line(context, block, "or %s, %s", work_reg, right);
        break;
    case AST_BINARY_OP_BIT_XOR:
        ok = mc_append_line(context, block, "mov %s, %s", work_reg, left) &&
             mc_append_line(context, block, "xor %s, %s", work_reg, right);
        break;
    case AST_BINARY_OP_BIT_NAND:
        ok = mc_append_line(context, block, "mov %s, %s", work_reg, left) &&
             mc_append_line(context, block, "and %s, %s", work_reg, right) &&
             mc_append_line(context, block, "not %s", work_reg);
        break;
    case AST_BINARY_OP_BIT_XNOR:
        ok = mc_append_line(context, block, "mov %s, %s", work_reg, left) &&
             mc_append_line(context, block, "xor %s, %s", work_reg, right) &&
             mc_append_line(context, block, "not %s", work_reg);
        break;
    case AST_BINARY_OP_SHIFT_LEFT:
        ok = mc_append_line(context, block, "mov %s, %s", work_reg, left) &&
             mc_append_line(context, block, "shl %s, %s", work_reg, right);
        break;
    case AST_BINARY_OP_SHIFT_RIGHT:
        ok = mc_append_line(context, block, "mov %s, %s", work_reg, left) &&
             mc_append_line(context,
                            block,
                            "%s %s, %s",
                            is_unsigned ? "shr" : "sar",
                            work_reg,
                            right);
        break;
    case AST_BINARY_OP_EQUAL:
        ok = mc_append_line(context, block, "mov %s, %s", work_reg, left) &&
             mc_append_line(context, block, "cmp %s, %s", work_reg, right) &&
             mc_append_line(context, block, "sete %s", work_reg);
        break;
    case AST_BINARY_OP_NOT_EQUAL:
        ok = mc_append_line(context, block, "mov %s, %s", work_reg, left) &&
             mc_append_line(context, block, "cmp %s, %s", work_reg, right) &&
             mc_append_line(context, block, "setne %s", work_reg);
        break;
    case AST_BINARY_OP_LESS:
        ok = mc_append_line(context, block, "mov %s, %s", work_reg, left) &&
             mc_append_line(context, block, "cmp %s, %s", work_reg, right) &&
             mc_append_line(context, block, "%s %s", is_unsigned ? "setb" : "setl", work_reg);
        break;
    case AST_BINARY_OP_GREATER:
        ok = mc_append_line(context, block, "mov %s, %s", work_reg, left) &&
             mc_append_line(context, block, "cmp %s, %s", work_reg, right) &&
             mc_append_line(context, block, "%s %s", is_unsigned ? "seta" : "setg", work_reg);
        break;
    case AST_BINARY_OP_LESS_EQUAL:
        ok = mc_append_line(context, block, "mov %s, %s", work_reg, left) &&
             mc_append_line(context, block, "cmp %s, %s", work_reg, right) &&
             mc_append_line(context, block, "%s %s", is_unsigned ? "setbe" : "setle", work_reg);
        break;
    case AST_BINARY_OP_GREATER_EQUAL:
        ok = mc_append_line(context, block, "mov %s, %s", work_reg, left) &&
             mc_append_line(context, block, "cmp %s, %s", work_reg, right) &&
             mc_append_line(context, block, "%s %s", is_unsigned ? "setae" : "setge", work_reg);
        break;
    case AST_BINARY_OP_LOGICAL_AND:
    case AST_BINARY_OP_LOGICAL_OR:
        mc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Logical short-circuit operators should not reach direct binary machine emission.");
        goto cleanup;
    }
    } /* end else (x86_64) */

    if (ok) {
        ok = mc_emit_store_vreg(context,
                                codegen_unit,
                                instruction->as.binary.dest_vreg,
                                block,
                                work_reg);
    }

cleanup:
    free(left);
    free(right);
    return ok;
}

