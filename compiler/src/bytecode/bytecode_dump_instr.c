#include "bytecode_dump_internal.h"

bool bytecode_dump_instruction(FILE *out,
                               const BytecodeProgram *program,
                               const BytecodeUnit *unit,
                               const BytecodeInstruction *instruction) {
    size_t item_index;

    switch (instruction->kind) {
    case BYTECODE_INSTR_BINARY:
        fprintf(out,
                "BC_BINARY t%zu <- ",
                instruction->as.binary.dest_temp);
        if (!bytecode_dump_value(out, program, unit, instruction->as.binary.left) ||
            fprintf(out,
                    " %s ",
                    bytecode_dump_binary_operator_name_text(
                        instruction->as.binary.operator)) < 0 ||
            !bytecode_dump_value(out, program, unit, instruction->as.binary.right)) {
            return false;
        }
        break;
    case BYTECODE_INSTR_UNARY:
        fprintf(out,
                "BC_UNARY t%zu <- %s ",
                instruction->as.unary.dest_temp,
                bytecode_dump_unary_operator_name_text(instruction->as.unary.operator));
        if (!bytecode_dump_value(out, program, unit, instruction->as.unary.operand)) {
            return false;
        }
        break;
    case BYTECODE_INSTR_CLOSURE:
        fprintf(out,
                "BC_CLOSURE t%zu <- unit#%zu(%s)",
                instruction->as.closure.dest_temp,
                instruction->as.closure.unit_index,
                program->units[instruction->as.closure.unit_index].name);
        fputc('(', out);
        for (item_index = 0;
             item_index < instruction->as.closure.capture_count;
             item_index++) {
            if (item_index > 0 && fputs(", ", out) == EOF) {
                return false;
            }
            if (!bytecode_dump_value(out,
                                     program,
                                     unit,
                                     instruction->as.closure.captures[item_index])) {
                return false;
            }
        }
        fputc(')', out);
        break;
    case BYTECODE_INSTR_CALL:
        if (instruction->as.call.has_result) {
            fprintf(out, "BC_CALL t%zu <- ", instruction->as.call.dest_temp);
        } else {
            fprintf(out, "BC_CALL void <- ");
        }
        if (!bytecode_dump_value(out, program, unit, instruction->as.call.callee)) {
            return false;
        }
        fputc('(', out);
        for (item_index = 0;
             item_index < instruction->as.call.argument_count;
             item_index++) {
            if (item_index > 0 && fputs(", ", out) == EOF) {
                return false;
            }
            if (!bytecode_dump_value(out,
                                     program,
                                     unit,
                                     instruction->as.call.arguments[item_index])) {
                return false;
            }
        }
        fputc(')', out);
        break;
    case BYTECODE_INSTR_CAST:
        fprintf(out, "BC_CAST t%zu <- ", instruction->as.cast.dest_temp);
        if (!bytecode_dump_value(out, program, unit, instruction->as.cast.operand) ||
            fputs(" to ", out) == EOF ||
            !bytecode_dump_write_checked_type(out, instruction->as.cast.target_type)) {
            return false;
        }
        break;
    case BYTECODE_INSTR_MEMBER:
        fprintf(out, "BC_MEMBER t%zu <- ", instruction->as.member.dest_temp);
        if (!bytecode_dump_value(out, program, unit, instruction->as.member.target) ||
            fputs(" . ", out) == EOF ||
            !bytecode_dump_constant_ref(out, program, instruction->as.member.member_index)) {
            return false;
        }
        break;
    case BYTECODE_INSTR_INDEX_LOAD:
        fprintf(out, "BC_INDEX_LOAD t%zu <- ", instruction->as.index_load.dest_temp);
        if (!bytecode_dump_value(out, program, unit, instruction->as.index_load.target) ||
            fputc('[', out) == EOF ||
            !bytecode_dump_value(out, program, unit, instruction->as.index_load.index) ||
            fputc(']', out) == EOF) {
            return false;
        }
        break;
    case BYTECODE_INSTR_ARRAY_LITERAL:
        fprintf(out,
                "BC_ARRAY_LITERAL t%zu <- [",
                instruction->as.array_literal.dest_temp);
        for (item_index = 0;
             item_index < instruction->as.array_literal.element_count;
             item_index++) {
            if (item_index > 0 && fputs(", ", out) == EOF) {
                return false;
            }
            if (!bytecode_dump_value(out,
                                     program,
                                     unit,
                                     instruction->as.array_literal.elements[item_index])) {
                return false;
            }
        }
        fputc(']', out);
        break;
    case BYTECODE_INSTR_TEMPLATE:
        fprintf(out,
                "BC_TEMPLATE t%zu <- [",
                instruction->as.template_literal.dest_temp);
        for (item_index = 0;
             item_index < instruction->as.template_literal.part_count;
             item_index++) {
            if (item_index > 0 && fputs(", ", out) == EOF) {
                return false;
            }
            if (!bytecode_dump_template_part(out,
                                             program,
                                             unit,
                                             &instruction->as.template_literal
                                                  .parts[item_index])) {
                return false;
            }
        }
        fputc(']', out);
        break;
    case BYTECODE_INSTR_STORE_LOCAL:
        fprintf(out,
                "BC_STORE_LOCAL local(%zu:%s) <- ",
                instruction->as.store_local.local_index,
                unit->locals[instruction->as.store_local.local_index].name);
        if (!bytecode_dump_value(out, program, unit, instruction->as.store_local.value)) {
            return false;
        }
        break;
    case BYTECODE_INSTR_STORE_GLOBAL:
        fprintf(out, "BC_STORE_GLOBAL ");
        if (!bytecode_dump_constant_ref(out, program,
                instruction->as.store_global.global_index) ||
            fputs(" <- ", out) == EOF ||
            !bytecode_dump_value(out, program, unit, instruction->as.store_global.value)) {
            return false;
        }
        break;
    case BYTECODE_INSTR_STORE_INDEX:
        fprintf(out, "BC_STORE_INDEX ");
        if (!bytecode_dump_value(out, program, unit, instruction->as.store_index.target) ||
            fputc('[', out) == EOF ||
            !bytecode_dump_value(out, program, unit, instruction->as.store_index.index) ||
            fputs("] <- ", out) == EOF ||
            !bytecode_dump_value(out, program, unit, instruction->as.store_index.value)) {
            return false;
        }
        break;
    case BYTECODE_INSTR_STORE_MEMBER:
        fprintf(out, "BC_STORE_MEMBER ");
        if (!bytecode_dump_value(out, program, unit, instruction->as.store_member.target) ||
            fputs(" . ", out) == EOF ||
            !bytecode_dump_constant_ref(out, program,
                instruction->as.store_member.member_index) ||
            fputs(" <- ", out) == EOF ||
            !bytecode_dump_value(out, program, unit, instruction->as.store_member.value)) {
            return false;
        }
        break;
    case BYTECODE_INSTR_UNION_NEW:
        fprintf(out, "BC_UNION_NEW t%zu <- type_desc(%zu) tag=%u payload=",
                instruction->as.union_new.dest_temp,
                instruction->as.union_new.type_desc_index,
                instruction->as.union_new.variant_tag);
        if (!bytecode_dump_value(out, program, unit, instruction->as.union_new.payload)) {
            return false;
        }
        break;
    case BYTECODE_INSTR_UNION_GET_TAG:
        fprintf(out, "BC_UNION_GET_TAG t%zu <- ",
                instruction->as.union_get_tag.dest_temp);
        if (!bytecode_dump_value(out, program, unit, instruction->as.union_get_tag.target)) {
            return false;
        }
        break;
    case BYTECODE_INSTR_UNION_GET_PAYLOAD:
        fprintf(out, "BC_UNION_GET_PAYLOAD t%zu <- ",
                instruction->as.union_get_payload.dest_temp);
        if (!bytecode_dump_value(out, program, unit,
                instruction->as.union_get_payload.target)) {
            return false;
        }
        break;
    case BYTECODE_INSTR_HETERO_ARRAY_NEW:
        fprintf(out, "BC_HETERO_ARRAY_NEW t%zu <- type_desc(%zu) [",
            instruction->as.hetero_array_new.dest_temp,
            instruction->as.hetero_array_new.type_desc_index);
        for (item_index = 0;
             item_index < instruction->as.hetero_array_new.element_count;
             item_index++) {
            if (item_index > 0 && fputs(", ", out) == EOF) {
                return false;
            }
            if (!bytecode_dump_value(out, program, unit,
                    instruction->as.hetero_array_new.elements[item_index])) {
                return false;
            }
        }
        fputc(']', out);
        break;
    case BYTECODE_INSTR_HETERO_ARRAY_GET_TAG:
        fprintf(out, "BC_HETERO_ARRAY_GET_TAG t%zu <- ",
                instruction->as.hetero_array_get_tag.dest_temp);
        if (!bytecode_dump_value(out, program, unit,
                instruction->as.hetero_array_get_tag.target) ||
            fputc('[', out) == EOF ||
            !bytecode_dump_value(out, program, unit,
                instruction->as.hetero_array_get_tag.index) ||
            fputc(']', out) == EOF) {
            return false;
        }
        break;
    case BYTECODE_INSTR_TYPEOF:
    case BYTECODE_INSTR_ISINT:
    case BYTECODE_INSTR_ISFLOAT:
    case BYTECODE_INSTR_ISBOOL:
    case BYTECODE_INSTR_ISSTRING:
    case BYTECODE_INSTR_ISARRAY:
        fprintf(out,
                instruction->kind == BYTECODE_INSTR_TYPEOF ? "BC_TYPEOF t%zu <- " :
                instruction->kind == BYTECODE_INSTR_ISINT ? "BC_ISINT t%zu <- " :
                instruction->kind == BYTECODE_INSTR_ISFLOAT ? "BC_ISFLOAT t%zu <- " :
                instruction->kind == BYTECODE_INSTR_ISBOOL ? "BC_ISBOOL t%zu <- " :
                instruction->kind == BYTECODE_INSTR_ISSTRING ? "BC_ISSTRING t%zu <- " :
                "BC_ISARRAY t%zu <- ",
                instruction->as.builtin_type_query.dest_temp);
        if (!bytecode_dump_value(out,
                                 program,
                                 unit,
                                 instruction->as.builtin_type_query.operand) ||
            fputs(" type=", out) == EOF ||
            !bytecode_dump_value(out,
                                 program,
                                 unit,
                                 instruction->as.builtin_type_query.type_text)) {
            return false;
        }
        break;
    case BYTECODE_INSTR_ISSAMETYPE:
        fprintf(out, "BC_ISSAMETYPE t%zu <- ",
                instruction->as.same_type_query.dest_temp);
        if (!bytecode_dump_value(out, program, unit,
                instruction->as.same_type_query.left) ||
            fputs(" left_type=", out) == EOF ||
            !bytecode_dump_value(out, program, unit,
                instruction->as.same_type_query.left_type_text) ||
            fputs(" right=", out) == EOF ||
            !bytecode_dump_value(out, program, unit,
                instruction->as.same_type_query.right) ||
            fputs(" right_type=", out) == EOF ||
            !bytecode_dump_value(out, program, unit,
                instruction->as.same_type_query.right_type_text)) {
            return false;
        }
        break;
    }

    return true;
}
