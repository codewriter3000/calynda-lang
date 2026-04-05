#include "asm_emit_internal.h"
#include <stdlib.h>
#include <string.h>

bool ae_emit_program_entry_glue(AsmEmitContext *context, FILE *out) {
    static const char *const argument_registers[] = {
        "rdi", "rsi", "rdx", "rcx", "r8", "r9"
    };
    const MachineUnit *start_unit = NULL;
    AsmUnitSymbol *start_symbol = NULL;
    size_t string_index;
    size_t unit_index;

    if (!context || !context->program || !out) {
        return false;
    }

    for (unit_index = 0; unit_index < context->program->unit_count; unit_index++) {
        const MachineUnit *unit = &context->program->units[unit_index];

        if (unit->kind == LIR_UNIT_START) {
            start_unit = unit;
            break;
        }
    }

    if (!start_unit) {
        return true;
    }

    start_symbol = ae_ensure_unit_symbol(context, start_unit->name);
    if (!start_symbol) {
        return false;
    }

    if (start_unit->is_boot) {
        /*
         * Boot entry: freestanding _start that calls the boot unit directly
         * and exits via Linux syscall (sys_exit = 60).
         */
        if (!ae_emit_line(out,
                       ".globl _start\n"
                       "_start:\n"
                       "    xor rbp, rbp\n"
                       "    xor r15, r15\n"
                       "    call %s\n"
                       "    mov edi, eax\n"
                       "    mov eax, 60\n"
                       "    syscall\n",
                       start_symbol->symbol)) {
            return false;
        }
    } else {
        if (!ae_emit_line(out,
                       ".globl calynda_program_start\n"
                       "calynda_program_start:\n"
                       "    push rbp\n"
                       "    mov rbp, rsp\n"
                       "    push r15\n"
                       "    sub rsp, 8\n"
                       "    xor r15, r15\n")) {
            return false;
        }

        for (string_index = 0; string_index < context->string_literal_count; string_index++) {
            if (!ae_emit_line(out,
                           "    mov rdi, OFFSET FLAT:%s\n"
                           "    call calynda_rt_register_static_object\n",
                           context->string_literals[string_index].object_label)) {
                return false;
            }
        }

        if (!ae_emit_line(out,
                       "    call %s\n"
                       "    add rsp, 8\n"
                       "    pop r15\n"
                       "    pop rbp\n"
                       "    ret\n",
                       start_symbol->symbol) ||
            !ae_emit_line(out,
                       ".globl main\n"
                       "main:\n"
                       "    push rbp\n"
                       "    mov rbp, rsp\n"
                       "    mov rdx, rsi\n"
                       "    mov esi, edi\n"
                       "    mov rdi, OFFSET FLAT:calynda_program_start\n"
                       "    call calynda_rt_start_process\n"
                       "    pop rbp\n"
                       "    ret\n")) {
            return false;
        }
    }

    for (unit_index = 0; unit_index < context->program->unit_count; unit_index++) {
        const MachineUnit *unit = &context->program->units[unit_index];
        AsmUnitSymbol *unit_symbol;
        char *wrapper_symbol;
        size_t extra_argument_count;
        size_t stack_pad;
        size_t register_argument_count;
        size_t argument_index;
        size_t cleanup_bytes;

        if (unit->kind != LIR_UNIT_LAMBDA) {
            continue;
        }

        unit_symbol = ae_ensure_unit_symbol(context, unit->name);
        wrapper_symbol = ae_closure_wrapper_symbol_name(unit->name);
        if (!unit_symbol || !wrapper_symbol) {
            free(wrapper_symbol);
            return false;
        }

        extra_argument_count = unit->parameter_count > 6 ? unit->parameter_count - 6 : 0;
        stack_pad = (extra_argument_count % 2 == 0) ? 8 : 0;
        register_argument_count = unit->parameter_count < 6 ? unit->parameter_count : 6;

        if (!ae_emit_line(out,
                       ".globl %s\n"
                       "%s:\n"
                       "    push rbp\n"
                       "    mov rbp, rsp\n"
                       "    push r15\n"
                       "    mov r15, rdi\n"
                       "    mov r11, rdx\n",
                       wrapper_symbol,
                       wrapper_symbol)) {
            free(wrapper_symbol);
            return false;
        }

        if (stack_pad > 0 && !ae_emit_line(out, "    sub rsp, %zu\n", stack_pad)) {
            free(wrapper_symbol);
            return false;
        }

        for (argument_index = unit->parameter_count; argument_index > 6; argument_index--) {
            if (!ae_emit_line(out,
                           "    push QWORD PTR [r11 + %zu]\n",
                           (argument_index - 1) * 8)) {
                free(wrapper_symbol);
                return false;
            }
        }

        for (argument_index = 0; argument_index < register_argument_count; argument_index++) {
            if (!ae_emit_line(out,
                           "    mov %s, QWORD PTR [r11 + %zu]\n",
                           argument_registers[argument_index],
                           argument_index * 8)) {
                free(wrapper_symbol);
                return false;
            }
        }

        if (!ae_emit_line(out, "    call %s\n", unit_symbol->symbol)) {
            free(wrapper_symbol);
            return false;
        }

        cleanup_bytes = stack_pad + (extra_argument_count * 8);
        if (cleanup_bytes > 0 && !ae_emit_line(out, "    add rsp, %zu\n", cleanup_bytes)) {
            free(wrapper_symbol);
            return false;
        }

        if (!ae_emit_line(out,
                       "    pop r15\n"
                       "    pop rbp\n"
                       "    ret\n")) {
            free(wrapper_symbol);
            return false;
        }

        free(wrapper_symbol);
    }

    return true;
}
