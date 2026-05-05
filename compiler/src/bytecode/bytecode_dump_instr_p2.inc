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
