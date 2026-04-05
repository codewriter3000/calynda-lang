#include "asm_emit_internal.h"
#include "runtime.h"
#include <stdlib.h>
#include <string.h>

bool ae_emit_unit_text(AsmEmitContext *context,
                           FILE *out,
                           size_t unit_index,
                           const MachineUnit *unit) {
    AsmUnitSymbol *symbol;
    AsmUnitLayout layout;
    size_t block_index;
    bool is_arm64 = context->program->target_desc &&
                    context->program->target_desc->kind == TARGET_KIND_AARCH64_AAPCS_ELF;

    symbol = ae_ensure_unit_symbol(context, unit->name);
    if (!symbol) {
        return false;
    }

    if (unit->kind == LIR_UNIT_ASM) {
        if (!ae_emit_line(out, ".globl %s\n%s:\n", symbol->symbol, symbol->symbol)) {
            return false;
        }
        if (unit->asm_body && unit->asm_body_length > 0) {
            if (fwrite(unit->asm_body, 1, unit->asm_body_length, out) != unit->asm_body_length) {
                return false;
            }
            if (unit->asm_body[unit->asm_body_length - 1] != '\n') {
                if (fputc('\n', out) == EOF) {
                    return false;
                }
            }
        }
        return true;
    }

    if (is_arm64) {
        size_t frame_size;

        layout = ae_compute_unit_layout_aarch64(unit);
        frame_size = (layout.saved_reg_words + layout.total_local_words) * 8;

        if (!ae_emit_line(out, ".globl %s\n%s:\n", symbol->symbol, symbol->symbol) ||
            !ae_emit_line(out, "    stp x29, x30, [sp, #-16]!\n") ||
            !ae_emit_line(out, "    mov x29, sp\n")) {
            return false;
        }
        if (frame_size > 0 &&
            !ae_emit_line(out, "    sub sp, sp, #%zu\n", frame_size)) {
            return false;
        }
        /* Save work register (x16) at first slot below FP */
        if (!ae_emit_line(out, "    str x16, [x29, #-8]\n")) {
            return false;
        }
    } else {
        layout = ae_compute_unit_layout(unit);

        if (!ae_emit_line(out, ".globl %s\n%s:\n", symbol->symbol, symbol->symbol) ||
            !ae_emit_line(out, "    push rbp\n    mov rbp, rsp\n    push r14\n")) {
            return false;
        }
        if (layout.saves_r12 && !ae_emit_line(out, "    push r12\n")) {
            return false;
        }
        if (layout.saves_r13 && !ae_emit_line(out, "    push r13\n")) {
            return false;
        }
        if (layout.total_local_words > 0 &&
            !ae_emit_line(out, "    sub rsp, %zu\n", layout.total_local_words * 8)) {
            return false;
        }
    }

    for (block_index = 0; block_index < unit->block_count; block_index++) {
        size_t instruction_index;

        if (block_index > 0 &&
            !ae_emit_line(out, ".L%s_%s:\n", symbol->symbol, unit->blocks[block_index].label)) {
            return false;
        }
        for (instruction_index = 0;
             instruction_index < unit->blocks[block_index].instruction_count;
             instruction_index++) {
            bool instr_ok;

            if (is_arm64) {
                instr_ok = ae_emit_machine_instruction_aarch64(context,
                                              out,
                                              unit,
                                              &layout,
                                              unit_index,
                                              block_index,
                                              instruction_index,
                                              unit->blocks[block_index].instructions[instruction_index].text);
            } else {
                instr_ok = ae_emit_machine_instruction(context,
                                              out,
                                              unit,
                                              &layout,
                                              unit_index,
                                              block_index,
                                              instruction_index,
                                              unit->blocks[block_index].instructions[instruction_index].text);
            }
            if (!instr_ok) {
                return false;
            }
        }
    }

    return true;
}

bool ae_emit_rodata(FILE *out, const AsmEmitContext *context) {
    size_t i;
    size_t j;

    if (!context || (context->byte_literal_count == 0 && context->string_literal_count == 0)) {
        return true;
    }
    if (!ae_emit_line(out, ".section .rodata\n")) {
        return false;
    }
    for (i = 0; i < context->byte_literal_count; i++) {
        if (!ae_emit_line(out, "%s:\n", context->byte_literals[i].label)) {
            return false;
        }
        if (context->byte_literals[i].length == 0) {
            if (!ae_emit_line(out, "    .byte 0\n")) {
                return false;
            }
        } else {
            if (!ae_emit_line(out, "    .byte ")) {
                return false;
            }
            for (j = 0; j < context->byte_literals[i].length; j++) {
                if (j > 0 && !ae_emit_line(out, ", ")) {
                    return false;
                }
                if (!ae_emit_line(out, "%u", (unsigned int)(unsigned char)context->byte_literals[i].text[j])) {
                    return false;
                }
            }
            if (!ae_emit_line(out, ", 0\n")) {
                return false;
            }
        }
    }
    return true;
}

bool ae_emit_data(FILE *out, const AsmEmitContext *context) {
    size_t i;

    if (!context) {
        return false;
    }
    if (context->global_symbol_count == 0 && context->string_literal_count == 0) {
        return true;
    }
    if (!ae_emit_line(out, ".data\n")) {
        return false;
    }
    for (i = 0; i < context->string_literal_count; i++) {
        if (!ae_emit_line(out,
                       "%s:\n"
                       "    .long %u\n"
                       "    .long %u\n"
                       "    .quad %zu\n"
                       "    .quad %s\n",
                       context->string_literals[i].object_label,
                       CALYNDA_RT_OBJECT_MAGIC,
                       CALYNDA_RT_OBJECT_STRING,
                       context->string_literals[i].length,
                       context->string_literals[i].bytes_label)) {
            return false;
        }
        if (!ae_emit_line(out, "%s:\n", context->string_literals[i].bytes_label)) {
            return false;
        }
        if (context->string_literals[i].length == 0) {
            if (!ae_emit_line(out, "    .byte 0\n")) {
                return false;
            }
        } else {
            size_t j;

            if (!ae_emit_line(out, "    .byte ")) {
                return false;
            }
            for (j = 0; j < context->string_literals[i].length; j++) {
                if (j > 0 && !ae_emit_line(out, ", ")) {
                    return false;
                }
                if (!ae_emit_line(out,
                               "%u",
                               (unsigned int)(unsigned char)context->string_literals[i].text[j])) {
                    return false;
                }
            }
            if (!ae_emit_line(out, ", 0\n")) {
                return false;
            }
        }
    }
    for (i = 0; i < context->global_symbol_count; i++) {
        char *sanitized = ae_sanitize_symbol(context->global_symbols[i].name);

        if (!sanitized) {
            return false;
        }
        if (!ae_emit_line(out, ".globl %s\n%s:\n", context->global_symbols[i].symbol, context->global_symbols[i].symbol)) {
            free(sanitized);
            return false;
        }
        if (context->global_symbols[i].has_store) {
            if (!ae_emit_line(out, "    .quad 0\n")) {
                free(sanitized);
                return false;
            }
        } else {
            if (!ae_emit_line(out, "    .quad __calynda_pkg_%s\n", sanitized)) {
                free(sanitized);
                return false;
            }
        }
        free(sanitized);
    }
    return true;
}
