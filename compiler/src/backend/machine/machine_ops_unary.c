#include "machine_internal.h"

bool mc_emit_unary(MachineBuildContext *context,
                   const LirUnit *lir_unit,
                   const CodegenUnit *codegen_unit,
                   const LirInstruction *instruction,
                   MachineBlock *block) {
    char *operand = NULL;
    const CodegenVRegAllocation *allocation;
    const TargetDescriptor *td = mc_target(context);
    const char *work_reg;
    bool is_arm64 = mc_is_aarch64(context);
    bool is_riscv64 = mc_is_riscv64(context);
    bool ok = false;

    if (!mc_format_operand(td, lir_unit, codegen_unit, instruction->as.unary.operand, &operand)) {
        goto cleanup;
    }
    if (instruction->as.unary.dest_vreg >= codegen_unit->vreg_count) {
        mc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Machine emission missing unary destination virtual register %zu.",
                     instruction->as.unary.dest_vreg);
        goto cleanup;
    }

    allocation = &codegen_unit->vreg_allocations[instruction->as.unary.dest_vreg];
    work_reg = allocation->location.kind == CODEGEN_VREG_REGISTER
        ? target_register_name(td, allocation->location.as.reg)
        : td->work_register.name;

    ok = mc_append_line(context, block, "mov %s, %s", work_reg, operand);
    if (!ok) {
        goto cleanup;
    }

    if (is_arm64 || is_riscv64) {
        switch (instruction->as.unary.operator) {
        case AST_UNARY_OP_PLUS:
            break;
        case AST_UNARY_OP_NEGATE:
            ok = mc_append_line(context, block, "neg %s, %s", work_reg, work_reg);
            break;
        case AST_UNARY_OP_BITWISE_NOT:
            ok = mc_append_line(context, block, "mvn %s, %s", work_reg, work_reg);
            break;
        case AST_UNARY_OP_LOGICAL_NOT:
            if (is_riscv64)
                ok = mc_append_line(context, block, "seqz %s, %s", work_reg, work_reg);
            else
                ok = mc_append_line(context, block, "cmp %s, bool(false)", work_reg) &&
                     mc_append_line(context, block, "cset %s, eq", work_reg);
            break;
        case AST_UNARY_OP_PRE_INCREMENT:
            if (is_riscv64)
                ok = mc_append_line(context, block, "addi %s, %s, 1", work_reg, work_reg);
            else
                ok = mc_append_line(context, block, "add %s, %s, i64(1)", work_reg, work_reg);
            break;
        case AST_UNARY_OP_PRE_DECREMENT:
            if (is_riscv64)
                ok = mc_append_line(context, block, "addi %s, %s, -1", work_reg, work_reg);
            else
                ok = mc_append_line(context, block, "sub %s, %s, i64(1)", work_reg, work_reg);
            break;
        case AST_UNARY_OP_DEREF:
        case AST_UNARY_OP_ADDRESS_OF:
            break;
        }
    } else {
        switch (instruction->as.unary.operator) {
        case AST_UNARY_OP_PLUS:
            break;
        case AST_UNARY_OP_NEGATE:
            ok = mc_append_line(context, block, "neg %s", work_reg);
            break;
        case AST_UNARY_OP_BITWISE_NOT:
            ok = mc_append_line(context, block, "not %s", work_reg);
            break;
        case AST_UNARY_OP_LOGICAL_NOT:
            ok = mc_append_line(context, block, "cmp %s, bool(false)", work_reg) &&
                 mc_append_line(context, block, "sete %s", work_reg);
            break;
        case AST_UNARY_OP_PRE_INCREMENT:
            ok = mc_append_line(context, block, "inc %s", work_reg);
            break;
        case AST_UNARY_OP_PRE_DECREMENT:
            ok = mc_append_line(context, block, "dec %s", work_reg);
            break;
        case AST_UNARY_OP_DEREF:
        case AST_UNARY_OP_ADDRESS_OF:
            break;
        }
    }

    if (ok) {
        ok = mc_emit_store_vreg(context,
                                codegen_unit,
                                instruction->as.unary.dest_vreg,
                                block,
                                work_reg);
    }

cleanup:
    free(operand);
    return ok;
}

bool mc_emit_cast(MachineBuildContext *context,
                  const LirUnit *lir_unit,
                  const CodegenUnit *codegen_unit,
                  const LirInstruction *instruction,
                  MachineBlock *block) {
    const TargetDescriptor *td = mc_target(context);
    char *operand = NULL;
    bool ok = false;

    if (!mc_format_operand(td, lir_unit, codegen_unit, instruction->as.cast.operand, &operand)) {
        goto cleanup;
    }
    ok = mc_emit_store_vreg(context,
                            codegen_unit,
                            instruction->as.cast.dest_vreg,
                            block,
                            operand);

cleanup:
    free(operand);
    return ok;
}
