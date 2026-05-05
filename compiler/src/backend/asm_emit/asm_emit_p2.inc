    if (strcmp(destination->text, "rax") == 0) {
        byte_reg = "al";
    } else if (strcmp(destination->text, "rcx") == 0) {
        byte_reg = "cl";
    } else if (strcmp(destination->text, "rdx") == 0) {
        byte_reg = "dl";
    } else if (strcmp(destination->text, "rdi") == 0) {
        byte_reg = "dil";
    } else if (strcmp(destination->text, "rsi") == 0) {
        byte_reg = "sil";
    } else if (strcmp(destination->text, "r8") == 0) {
        byte_reg = "r8b";
    } else if (strcmp(destination->text, "r9") == 0) {
        byte_reg = "r9b";
    } else if (strcmp(destination->text, "r10") == 0) {
        byte_reg = "r10b";
    } else if (strcmp(destination->text, "r11") == 0) {
        byte_reg = "r11b";
    } else if (strcmp(destination->text, "r12") == 0) {
        byte_reg = "r12b";
    } else if (strcmp(destination->text, "r13") == 0) {
        byte_reg = "r13b";
    } else if (strcmp(destination->text, "r14") == 0) {
        byte_reg = "r14b";
    } else if (strcmp(destination->text, "r15") == 0) {
        byte_reg = "r15b";
    }

    if (!byte_reg) {
        return false;
    }

    return ae_emit_line(out, "    %s %s\n", mnemonic, byte_reg) &&
           ae_emit_line(out, "    movzx %s, %s\n", destination->text, byte_reg);
}

bool ae_emit_shift(FILE *out,
                   const char *mnemonic,
                   AsmOperand *destination,
                   AsmOperand *source) {
    if (!out || !mnemonic || !destination || !source) {
        return false;
    }
    if (source->kind == ASM_OPERAND_IMMEDIATE) {
        return ae_emit_two_operand(out, mnemonic, destination, source);
    }
    return ae_emit_line(out, "    mov rcx, ") &&
           ae_write_operand(out, source) &&
           ae_emit_line(out, "\n    %s %s, cl\n", mnemonic, destination->text);
}

bool ae_emit_div(FILE *out, const char *mnemonic, AsmOperand *source) {
    if (!out || !mnemonic || !source) {
        return false;
    }
    if (source->kind == ASM_OPERAND_IMMEDIATE || source->kind == ASM_OPERAND_ADDRESS) {
        return ae_emit_line(out, "    mov rcx, ") &&
               ae_write_operand(out, source) &&
               ae_emit_line(out, "\n    %s rcx\n", mnemonic);
    }
    return ae_emit_line(out, "    %s ", mnemonic) && ae_write_operand(out, source) && ae_emit_line(out, "\n");
}
