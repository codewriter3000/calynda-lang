#include "lir_dump_internal.h"

bool lir_dump_instruction(FILE *out, const LirUnit *unit, const LirInstruction *instruction) {
    switch (instruction->kind) {
    case LIR_INSTR_INCOMING_ARG:
        fprintf(out, "incoming arg%zu -> slot(%zu:%s)",
            instruction->as.incoming_arg.argument_index,
            instruction->as.incoming_arg.slot_index,
            unit->slots[instruction->as.incoming_arg.slot_index].name);
        break;
    case LIR_INSTR_INCOMING_CAPTURE:
        fprintf(out, "incoming capture%zu -> slot(%zu:%s)",
            instruction->as.incoming_capture.capture_index,
            instruction->as.incoming_capture.slot_index,
            unit->slots[instruction->as.incoming_capture.slot_index].name);
        break;
    case LIR_INSTR_OUTGOING_ARG:
        fprintf(out, "out arg%zu <- ", instruction->as.outgoing_arg.argument_index);
        if (!lir_dump_operand(out, unit, instruction->as.outgoing_arg.value)) {
            return false;
        }
        break;
    case LIR_INSTR_BINARY:
        fprintf(out, "v%zu = binary %s ", instruction->as.binary.dest_vreg,
                lir_dump_binary_operator_name_text(instruction->as.binary.operator));
        if (!lir_dump_operand(out, unit, instruction->as.binary.left) ||
            fputs(" ", out) == EOF ||
            !lir_dump_operand(out, unit, instruction->as.binary.right)) {
            return false;
        }
        break;
    case LIR_INSTR_UNARY:
        fprintf(out, "v%zu = unary %s ", instruction->as.unary.dest_vreg,
            lir_dump_unary_operator_name_text(instruction->as.unary.operator));
        if (!lir_dump_operand(out, unit, instruction->as.unary.operand)) {
            return false;
        }
        break;
    case LIR_INSTR_CLOSURE:
        fprintf(out, "v%zu = closure unit=%s(", instruction->as.closure.dest_vreg,
            instruction->as.closure.unit_name);
        for (size_t capture_index = 0;
             capture_index < instruction->as.closure.capture_count;
             capture_index++) {
            if (capture_index > 0 && fputs(", ", out) == EOF) {
                return false;
            }
            if (!lir_dump_operand(out,
                                  unit,
                                  instruction->as.closure.captures[capture_index])) {
                return false;
            }
        }
        if (fputc(')', out) == EOF) {
            return false;
        }
        break;
    case LIR_INSTR_CALL:
        if (instruction->as.call.has_result) {
            fprintf(out, "v%zu = ", instruction->as.call.dest_vreg);
        }
        fprintf(out, "call ");
        if (!lir_dump_operand(out, unit, instruction->as.call.callee) ||
            fprintf(out, " argc=%zu", instruction->as.call.argument_count) < 0) {
            return false;
        }
        break;
    case LIR_INSTR_CAST:
        fprintf(out, "v%zu = cast ", instruction->as.cast.dest_vreg);
        if (!lir_dump_operand(out, unit, instruction->as.cast.operand) ||
            fputs(" to ", out) == EOF ||
            !lir_dump_write_checked_type(out, instruction->as.cast.target_type)) {
            return false;
        }
        break;
    case LIR_INSTR_MEMBER:
        fprintf(out, "v%zu = member ", instruction->as.member.dest_vreg);
        if (!lir_dump_operand(out, unit, instruction->as.member.target) ||
            fprintf(out, ".%s", instruction->as.member.member) < 0) {
            return false;
        }
        break;
    case LIR_INSTR_UNION_GET_TAG:
        fprintf(out, "v%zu = union_get_tag ", instruction->as.union_get_tag.dest_vreg);
        if (!lir_dump_operand(out, unit, instruction->as.union_get_tag.target)) {
            return false;
        }
        break;
    case LIR_INSTR_UNION_GET_PAYLOAD:
        fprintf(out,
                "v%zu = union_get_payload ",
                instruction->as.union_get_payload.dest_vreg);
        if (!lir_dump_operand(out, unit, instruction->as.union_get_payload.target)) {
            return false;
        }
        break;
    case LIR_INSTR_INDEX_LOAD:
        fprintf(out, "v%zu = index ", instruction->as.index_load.dest_vreg);
        if (!lir_dump_operand(out, unit, instruction->as.index_load.target) ||
            fputc('[', out) == EOF ||
            !lir_dump_operand(out, unit, instruction->as.index_load.index) ||
            fputc(']', out) == EOF) {
            return false;
        }
        break;
    case LIR_INSTR_ARRAY_LITERAL:
        fprintf(out, "v%zu = array(", instruction->as.array_literal.dest_vreg);
        for (size_t element_index = 0;
             element_index < instruction->as.array_literal.element_count;
             element_index++) {
            if (element_index > 0 && fputs(", ", out) == EOF) {
                return false;
            }
            if (!lir_dump_operand(out,
                                  unit,
                                  instruction->as.array_literal.elements[element_index])) {
                return false;
            }
        }
        if (fputc(')', out) == EOF) {
            return false;
        }
        break;
    case LIR_INSTR_TEMPLATE:
        fprintf(out, "v%zu = template(", instruction->as.template_literal.dest_vreg);
        for (size_t part_index = 0;
             part_index < instruction->as.template_literal.part_count;
             part_index++) {
            if (part_index > 0 && fputs(", ", out) == EOF) {
                return false;
            }
            if (!lir_dump_template_part(out,
                                        unit,
                                        &instruction->as.template_literal.parts[part_index])) {
                return false;
            }
        }
        if (fputc(')', out) == EOF) {
            return false;
        }
        break;
    case LIR_INSTR_STORE_SLOT:
        fprintf(out,
                "store slot(%zu:%s) <- ",
                instruction->as.store_slot.slot_index,
                unit->slots[instruction->as.store_slot.slot_index].name);
        if (!lir_dump_operand(out, unit, instruction->as.store_slot.value)) {
            return false;
        }
        break;
    case LIR_INSTR_STORE_GLOBAL:
        fprintf(out,
                "store global(%s) <- ",
                instruction->as.store_global.global_name);
        if (!lir_dump_operand(out, unit, instruction->as.store_global.value)) {
            return false;
        }
        break;
    case LIR_INSTR_STORE_INDEX:
        fprintf(out, "store index ");
        if (!lir_dump_operand(out, unit, instruction->as.store_index.target) ||
            fputc('[', out) == EOF ||
            !lir_dump_operand(out, unit, instruction->as.store_index.index) ||
            fputs("] <- ", out) == EOF ||
            !lir_dump_operand(out, unit, instruction->as.store_index.value)) {
            return false;
        }
        break;
    case LIR_INSTR_STORE_MEMBER:
        fprintf(out, "store member ");
        if (!lir_dump_operand(out, unit, instruction->as.store_member.target) ||
            fprintf(out, ".%s <- ", instruction->as.store_member.member) < 0 ||
            !lir_dump_operand(out, unit, instruction->as.store_member.value)) {
            return false;
        }
        break;
    case LIR_INSTR_UNION_NEW:
        fprintf(out, "v%zu = union_new ", instruction->as.union_new.dest_vreg);
        if (!lir_dump_type_descriptor(out, &instruction->as.union_new.type_desc) ||
            fprintf(out, " variant %zu", instruction->as.union_new.variant_index) < 0) {
            return false;
        }
        if (instruction->as.union_new.has_payload) {
            fputs(" payload ", out);
            if (!lir_dump_operand(out, unit, instruction->as.union_new.payload)) {
                return false;
            }
        }
        break;
    case LIR_INSTR_HETERO_ARRAY_NEW:
        fprintf(out, "v%zu = hetero_array_new ", instruction->as.hetero_array_new.dest_vreg);
        if (!lir_dump_type_descriptor(out, &instruction->as.hetero_array_new.type_desc) ||
            fputs(" [", out) == EOF) {
            return false;
        }
        for (size_t ei = 0; ei < instruction->as.hetero_array_new.element_count; ei++) {
            if (ei > 0 && fputs(", ", out) == EOF) {
                return false;
            }
            if (!lir_dump_operand(out, unit, instruction->as.hetero_array_new.elements[ei])) {
                return false;
            }
        }
        fputc(']', out);
        break;
    }

    return true;
}

bool lir_dump_terminator(FILE *out, const LirUnit *unit, const LirBasicBlock *block) {
    switch (block->terminator.kind) {
    case LIR_TERM_RETURN:
        fprintf(out, "return");
        if (block->terminator.as.return_term.has_value) {
            fputc(' ', out);
            if (!lir_dump_operand(out, unit, block->terminator.as.return_term.value)) {
                return false;
            }
        }
        break;
    case LIR_TERM_JUMP:
        fprintf(out, "jump bb%zu", block->terminator.as.jump_term.target_block);
        break;
    case LIR_TERM_BRANCH:
        fprintf(out, "branch ");
        if (!lir_dump_operand(out, unit, block->terminator.as.branch_term.condition)) {
            return false;
        }
        fprintf(out,
                " -> bb%zu, bb%zu",
                block->terminator.as.branch_term.true_block,
                block->terminator.as.branch_term.false_block);
        break;
    case LIR_TERM_THROW:
        fprintf(out, "throw ");
        if (!lir_dump_operand(out, unit, block->terminator.as.throw_term.value)) {
            return false;
        }
        break;
    case LIR_TERM_NONE:
        fprintf(out, "<no terminator>");
        break;
    }

    return true;
}
