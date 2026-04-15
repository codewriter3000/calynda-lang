#include "asm_emit_internal.h"
#include <stdlib.h>
#include <string.h>

bool ae_emit_machine_instruction(AsmEmitContext *context,
                                     FILE *out,
                                     const MachineUnit *unit,
                                     const AsmUnitLayout *layout,
                                     size_t unit_index,
                                     size_t block_index,
                                     size_t instruction_index,
                                     const char *instruction_text) {
    char *space;
    char *mnemonic = NULL;
    char *operand_text = NULL;
    char *left_text = NULL;
    char *right_text = NULL;
    AsmOperand left;
    AsmOperand right;
    bool ok = false;
    const char *lea_source;

    (void)unit_index;

    memset(&left, 0, sizeof(left));
    memset(&right, 0, sizeof(right));

    if (strcmp(instruction_text, "push rbp") == 0 ||
        strcmp(instruction_text, "mov rbp, rsp") == 0 ||
        strcmp(instruction_text, "push r14") == 0 ||
        strcmp(instruction_text, "push r12") == 0 ||
        strcmp(instruction_text, "push r13") == 0 ||
        ae_starts_with(instruction_text, "sub rsp, ") ||
        ae_starts_with(instruction_text, "add rsp, ") ||
        strcmp(instruction_text, "pop r14") == 0 ||
        strcmp(instruction_text, "pop r12") == 0 ||
        strcmp(instruction_text, "pop r13") == 0 ||
        strcmp(instruction_text, "pop rbp") == 0) {
        return true;
    }
    if (strcmp(instruction_text, "push r10") == 0) {
        if (!layout->preserves_r10) {
            return true;
        }
        return ae_emit_line(out,
                         "    mov QWORD PTR [rbp - %zu], r10\n",
                         ae_call_preserve_offset(layout, unit, 0));
    }
    if (strcmp(instruction_text, "pop r10") == 0) {
        if (!layout->preserves_r10) {
            return true;
        }
        return ae_emit_line(out,
                         "    mov r10, QWORD PTR [rbp - %zu]\n",
                         ae_call_preserve_offset(layout, unit, 0));
    }
    if (strcmp(instruction_text, "push r11") == 0) {
        size_t preserve_index = layout->preserves_r10 ? 1 : 0;

        if (!layout->preserves_r11) {
            return true;
        }
        return ae_emit_line(out,
                         "    mov QWORD PTR [rbp - %zu], r11\n",
                         ae_call_preserve_offset(layout, unit, preserve_index));
    }
    if (strcmp(instruction_text, "pop r11") == 0) {
        size_t preserve_index = layout->preserves_r10 ? 1 : 0;

        if (!layout->preserves_r11) {
            return true;
        }
        return ae_emit_line(out,
                         "    mov r11, QWORD PTR [rbp - %zu]\n",
                         ae_call_preserve_offset(layout, unit, preserve_index));
    }
    if (strcmp(instruction_text, "ret") == 0) {
        if (layout->total_local_words > 0 &&
            !ae_emit_line(out, "    add rsp, %zu\n", layout->total_local_words * 8)) {
            return false;
        }
        if (layout->saves_r13 && !ae_emit_line(out, "    pop r13\n")) {
            return false;
        }
        if (layout->saves_r12 && !ae_emit_line(out, "    pop r12\n")) {
            return false;
        }
        return ae_emit_line(out, "    pop r14\n    pop rbp\n    ret\n");
    }

    if (strcmp(instruction_text, "cqo") == 0) {
        return ae_emit_line(out, "    cqo\n");
    }

    space = strchr(instruction_text, ' ');
    if (!space) {
        return false;
    }
    mnemonic = ae_copy_text_n(instruction_text, (size_t)(space - instruction_text));
    operand_text = ae_trim_copy(space + 1);
    if (!mnemonic || !operand_text) {
        free(mnemonic);
        free(operand_text);
        return false;
    }

    if (strcmp(mnemonic, "call") == 0 || strcmp(mnemonic, "jmp") == 0 || strcmp(mnemonic, "jne") == 0) {
        if (!ae_translate_operand(context,
                               unit,
                               layout,
                               block_index,
                               instruction_index,
                               operand_text,
                               false,
                               &left)) {
            free(mnemonic);
            free(operand_text);
            return false;
        }
        if (strcmp(mnemonic, "call") == 0) {
            bool ok = ae_emit_line(out, "    call %s\n", left.text);

            ae_free_operand(&left);
            free(mnemonic);
            free(operand_text);
            return ok;
        }
        {
            bool ok = ae_emit_line(out, "    %s %s\n", mnemonic, left.text);

            ae_free_operand(&left);
            free(mnemonic);
            free(operand_text);
            return ok;
        }
    }

    if (!strchr(operand_text, ',')) {
        if (!ae_translate_operand(context,
                               unit,
                               layout,
                               block_index,
                               instruction_index,
                               operand_text,
                               true,
                               &left)) {
            free(mnemonic);
            free(operand_text);
            return false;
        }
        if (ae_starts_with(mnemonic, "set")) {
            bool ok = ae_emit_setcc(out, mnemonic, &left);
            ae_free_operand(&left);
            free(mnemonic);
            free(operand_text);
            return ok;
        }
        if (strcmp(mnemonic, "push") == 0 || strcmp(mnemonic, "pop") == 0 ||
            strcmp(mnemonic, "neg") == 0 || strcmp(mnemonic, "not") == 0) {
            bool ok = ae_emit_line(out, "    %s %s\n", mnemonic, left.text);
            ae_free_operand(&left);
            free(mnemonic);
            free(operand_text);
            return ok;
        }
        if (strcmp(mnemonic, "div") == 0 || strcmp(mnemonic, "idiv") == 0) {
            bool ok = ae_emit_div(out, mnemonic, &left);
            ae_free_operand(&left);
            free(mnemonic);
            free(operand_text);
            return ok;
        }
        ae_free_operand(&left);
        free(mnemonic);
        free(operand_text);
        return false;
    }

    if (!ae_split_binary_operands(operand_text, &left_text, &right_text) ||
        !ae_translate_operand(context,
                           unit,
                           layout,
                           block_index,
                           instruction_index,
                           left_text,
                           true,
                           &left) ||
        !ae_translate_operand(context,
                           unit,
                           layout,
                           block_index,
                           instruction_index,
                           right_text,
                           false,
                           &right)) {
        free(mnemonic);
        free(operand_text);
        free(left_text);
        free(right_text);
        ae_free_operand(&left);
        ae_free_operand(&right);
        return false;
    }
    free(operand_text);
    free(left_text);
    free(right_text);

    if (strcmp(mnemonic, "lea") == 0) {
        lea_source = ae_starts_with(right.text, "QWORD PTR ") ? right.text + strlen("QWORD PTR ") : right.text;
        ok = ae_emit_line(out, "    lea %s, %s\n", left.text, lea_source);
        ae_free_operand(&left);
        ae_free_operand(&right);
        free(mnemonic);
        return ok;
    }
    if (strcmp(mnemonic, "shl") == 0 || strcmp(mnemonic, "shr") == 0 || strcmp(mnemonic, "sar") == 0) {
        bool ok = ae_emit_shift(out, mnemonic, &left, &right);
        ae_free_operand(&left);
        ae_free_operand(&right);
        free(mnemonic);
        return ok;
    }

    {
        bool ok = ae_emit_two_operand(out, mnemonic, &left, &right);

        ae_free_operand(&left);
        ae_free_operand(&right);
        free(mnemonic);
        return ok;
    }
}
