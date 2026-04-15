#include "mir_dump_internal.h"

bool mir_dump_instruction(FILE *out, const MirUnit *unit, const MirInstruction *instruction) {
    switch (instruction->kind) {
    case MIR_INSTR_BINARY:
        fprintf(out, "t%zu = binary %s ",
                instruction->as.binary.dest_temp,
                mir_dump_binary_operator_name_text(instruction->as.binary.operator));
        if (!mir_dump_value(out, unit, instruction->as.binary.left) ||
            fputs(" ", out) == EOF ||
            !mir_dump_value(out, unit, instruction->as.binary.right)) {
            return false;
        }
        break;
    case MIR_INSTR_UNARY:
        fprintf(out, "t%zu = unary %s ",
                instruction->as.unary.dest_temp,
                mir_dump_unary_operator_name_text(instruction->as.unary.operator));
        if (!mir_dump_value(out, unit, instruction->as.unary.operand)) {
            return false;
        }
        break;
    case MIR_INSTR_CLOSURE:
        fprintf(out, "t%zu = closure unit=%s(",
                instruction->as.closure.dest_temp,
                instruction->as.closure.unit_name);
        for (size_t capture_index = 0;
             capture_index < instruction->as.closure.capture_count;
             capture_index++) {
            if (capture_index > 0 && fputs(", ", out) == EOF) {
                return false;
            }
            if (!mir_dump_value(out,
                            unit,
                            instruction->as.closure.captures[capture_index])) {
                return false;
            }
        }
        if (fputc(')', out) == EOF) {
            return false;
        }
        break;
    case MIR_INSTR_CALL:
        if (instruction->as.call.has_result) {
            fprintf(out, "t%zu = ", instruction->as.call.dest_temp);
        }
        fprintf(out, "call ");
        if (!mir_dump_value(out, unit, instruction->as.call.callee)) {
            return false;
        }
        fputc('(', out);
        for (size_t arg_index = 0; arg_index < instruction->as.call.argument_count; arg_index++) {
            if (arg_index > 0) {
                fputs(", ", out);
            }
            if (!mir_dump_value(out, unit, instruction->as.call.arguments[arg_index])) {
                return false;
            }
        }
        fputc(')', out);
        break;
    case MIR_INSTR_CAST:
        fprintf(out, "t%zu = cast ", instruction->as.cast.dest_temp);
        if (!mir_dump_value(out, unit, instruction->as.cast.operand) ||
            fputs(" to ", out) == EOF ||
            !mir_dump_write_checked_type(out, instruction->as.cast.target_type)) {
            return false;
        }
        break;
    case MIR_INSTR_MEMBER:
        fprintf(out, "t%zu = member ", instruction->as.member.dest_temp);
        if (!mir_dump_value(out, unit, instruction->as.member.target) ||
            fprintf(out, ".%s", instruction->as.member.member) < 0) {
            return false;
        }
        break;
    case MIR_INSTR_INDEX_LOAD:
        fprintf(out, "t%zu = index ", instruction->as.index_load.dest_temp);
        if (!mir_dump_value(out, unit, instruction->as.index_load.target) ||
            fputc('[', out) == EOF ||
            !mir_dump_value(out, unit, instruction->as.index_load.index) ||
            fputc(']', out) == EOF) {
            return false;
        }
        break;
    case MIR_INSTR_ARRAY_LITERAL:
        fprintf(out, "t%zu = array(", instruction->as.array_literal.dest_temp);
        for (size_t element_index = 0;
             element_index < instruction->as.array_literal.element_count;
             element_index++) {
            if (element_index > 0 && fputs(", ", out) == EOF) {
                return false;
            }
            if (!mir_dump_value(out,
                            unit,
                            instruction->as.array_literal.elements[element_index])) {
                return false;
            }
        }
        if (fputc(')', out) == EOF) {
            return false;
        }
        break;
    case MIR_INSTR_TEMPLATE:
        fprintf(out, "t%zu = template(", instruction->as.template_literal.dest_temp);
        for (size_t part_index = 0;
             part_index < instruction->as.template_literal.part_count;
             part_index++) {
            if (part_index > 0 && fputs(", ", out) == EOF) {
                return false;
            }
            if (!mir_dump_template_part(out,
                                        unit,
                                        &instruction->as.template_literal.parts[part_index])) {
                return false;
            }
        }
        if (fputc(')', out) == EOF) {
            return false;
        }
        break;
    case MIR_INSTR_STORE_LOCAL:
        fprintf(out, "store local(%zu:%s) <- ",
                instruction->as.store_local.local_index,
                unit->locals[instruction->as.store_local.local_index].name);
        if (!mir_dump_value(out, unit, instruction->as.store_local.value)) {
            return false;
        }
        break;
    case MIR_INSTR_STORE_GLOBAL:
        fprintf(out, "store global(%s) <- ", instruction->as.store_global.global_name);
        if (!mir_dump_value(out, unit, instruction->as.store_global.value)) {
            return false;
        }
        break;
    case MIR_INSTR_STORE_INDEX:
        fprintf(out, "store index ");
        if (!mir_dump_value(out, unit, instruction->as.store_index.target) ||
            fputc('[', out) == EOF ||
            !mir_dump_value(out, unit, instruction->as.store_index.index) ||
            fputs("] <- ", out) == EOF ||
            !mir_dump_value(out, unit, instruction->as.store_index.value)) {
            return false;
        }
        break;
    case MIR_INSTR_STORE_MEMBER:
        fprintf(out, "store member ");
        if (!mir_dump_value(out, unit, instruction->as.store_member.target) ||
            fprintf(out, ".%s <- ", instruction->as.store_member.member) < 0 ||
            !mir_dump_value(out, unit, instruction->as.store_member.value)) {
            return false;
        }
        break;
    case MIR_INSTR_HETERO_ARRAY_NEW:
        fprintf(out, "t%zu = hetero_array_new [",
                instruction->as.hetero_array_new.dest_temp);
        for (size_t elem_index = 0;
             elem_index < instruction->as.hetero_array_new.element_count;
             elem_index++) {
            if (elem_index > 0 && fputs(", ", out) == EOF) {
                return false;
            }
            if (!mir_dump_value(out, unit,
                            instruction->as.hetero_array_new.elements[elem_index])) {
                return false;
            }
        }
        fputc(']', out);
        break;
    case MIR_INSTR_UNION_NEW:
        fprintf(out, "t%zu = union_new %s variant %zu",
                instruction->as.union_new.dest_temp,
                instruction->as.union_new.union_name ?
                    instruction->as.union_new.union_name : "?",
                instruction->as.union_new.variant_index);
        if (instruction->as.union_new.has_payload) {
            fputs(" payload ", out);
            if (!mir_dump_value(out, unit, instruction->as.union_new.payload)) {
                return false;
            }
        }
        break;
    }

    return true;
}

bool mir_dump_terminator(FILE *out, const MirUnit *unit, const MirBasicBlock *block) {
    switch (block->terminator.kind) {
    case MIR_TERM_RETURN:
        fprintf(out, "return");
        if (block->terminator.as.return_term.has_value) {
            fputc(' ', out);
            if (!mir_dump_value(out, unit, block->terminator.as.return_term.value)) {
                return false;
            }
        }
        break;
    case MIR_TERM_GOTO:
        fprintf(out, "goto bb%zu", block->terminator.as.goto_term.target_block);
        break;
    case MIR_TERM_BRANCH:
        fprintf(out, "branch ");
        if (!mir_dump_value(out, unit, block->terminator.as.branch_term.condition)) {
            return false;
        }
        fprintf(out, " -> bb%zu, bb%zu",
                block->terminator.as.branch_term.true_block,
                block->terminator.as.branch_term.false_block);
        break;
    case MIR_TERM_THROW:
        fprintf(out, "throw ");
        if (!mir_dump_value(out, unit, block->terminator.as.throw_term.value)) {
            return false;
        }
        break;
    case MIR_TERM_NONE:
        fprintf(out, "<no terminator>");
        break;
    }

    return true;
}
