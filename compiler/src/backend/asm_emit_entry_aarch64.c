#include "asm_emit_internal.h"
#include <stdlib.h>
#include <string.h>

/*
 * ARM64 program entry glue:
 *   calynda_program_start  — calls the start unit after registering statics
 *   main                   — calls calynda_rt_start_process
 *   closure wrappers       — unpack call arguments from boxed array
 */

bool ae_emit_program_entry_glue_aarch64(AsmEmitContext *context, FILE *out) {
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
         * and exits via Linux syscall (sys_exit = 93).
         */
        if (!ae_emit_line(out,
                       ".globl _start\n"
                       "_start:\n"
                       "    mov x29, #0\n"
                       "    mov x30, #0\n"
                       "    mov x28, #0\n"
                       "    bl %s\n"
                       "    mov x8, #93\n"
                       "    svc #0\n",
                       start_symbol->symbol)) {
            return false;
        }
    } else {
        /*
         * calynda_program_start:
         *   Save FP+LR, save x28 (closure env register), set x28 = 0,
         *   register static strings, call start unit, restore, return.
         */
        if (!ae_emit_line(out,
                       ".globl calynda_program_start\n"
                       "calynda_program_start:\n"
                       "    stp x29, x30, [sp, #-32]!\n"
                       "    mov x29, sp\n"
                       "    str x28, [x29, #16]\n"
                       "    mov x28, #0\n")) {
            return false;
        }

        for (string_index = 0; string_index < context->string_literal_count; string_index++) {
            if (!ae_emit_line(out,
                           "    adrp x0, %s\n"
                           "    add x0, x0, :lo12:%s\n"
                           "    bl calynda_rt_register_static_object\n",
                           context->string_literals[string_index].object_label,
                           context->string_literals[string_index].object_label)) {
                return false;
            }
        }

        if (!ae_emit_line(out,
                       "    bl %s\n"
                       "    ldr x28, [x29, #16]\n"
                       "    ldp x29, x30, [sp], #32\n"
                       "    ret\n",
                       start_symbol->symbol)) {
            return false;
        }

        /*
         * main:
         *   ARM64 main receives argc in w0 and argv in x1.
         *   We need: x0 = &calynda_program_start, w1 = argc, x2 = argv.
         */
        if (!ae_emit_line(out,
                       ".globl main\n"
                       "main:\n"
                       "    stp x29, x30, [sp, #-16]!\n"
                       "    mov x29, sp\n"
                       "    mov x2, x1\n"
                       "    mov w1, w0\n"
                       "    adrp x0, calynda_program_start\n"
                       "    add x0, x0, :lo12:calynda_program_start\n"
                       "    bl calynda_rt_start_process\n"
                       "    ldp x29, x30, [sp], #16\n"
                   "    ret\n")) {
        return false;
    }
    }

    /*
     * Lambda closure wrappers.
     *
     * On ARM64 AAPCS64: x0 = closure env ptr, x1 = packed args ptr,
     * x2 = arg count (or args array depending on RT).
     *
     * The wrapper unpacks arguments from the array in x2 (the packed
     * argument buffer) into the correct registers and stack slots,
     * then calls the actual lambda body.
     */
    for (unit_index = 0; unit_index < context->program->unit_count; unit_index++) {
        const MachineUnit *unit = &context->program->units[unit_index];
        AsmUnitSymbol *unit_symbol;
        char *wrapper_symbol;
        size_t register_arg_count;
        size_t extra_arg_count;
        size_t stack_pad;
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

        extra_arg_count = unit->parameter_count > 8 ? unit->parameter_count - 8 : 0;
        stack_pad = ((extra_arg_count % 2 == 0) ? 16 : 8) - (extra_arg_count % 2 == 0 ? 0 : 0);
        register_arg_count = unit->parameter_count < 8 ? unit->parameter_count : 8;

        /* Compute stack padding for 16-byte alignment */
        {
            size_t total_stack = extra_arg_count * 8;

            stack_pad = (total_stack % 16 != 0) ? (16 - (total_stack % 16)) : 0;
        }

        if (!ae_emit_line(out,
                       ".globl %s\n"
                       "%s:\n"
                       "    stp x29, x30, [sp, #-32]!\n"
                       "    mov x29, sp\n"
                       "    str x28, [x29, #16]\n"
                       "    mov x28, x0\n"
                       "    mov x17, x2\n",
                       wrapper_symbol,
                       wrapper_symbol)) {
            free(wrapper_symbol);
            return false;
        }

        /* Push extra arguments (>8) to the stack */
        if (extra_arg_count > 0) {
            cleanup_bytes = (extra_arg_count * 8) + stack_pad;
            if (!ae_emit_line(out, "    sub sp, sp, #%zu\n", cleanup_bytes)) {
                free(wrapper_symbol);
                return false;
            }
            for (argument_index = 0; argument_index < extra_arg_count; argument_index++) {
                size_t src_index = 8 + argument_index;

                if (!ae_emit_line(out,
                               "    ldr x16, [x17, #%zu]\n"
                               "    str x16, [sp, #%zu]\n",
                               src_index * 8,
                               argument_index * 8)) {
                    free(wrapper_symbol);
                    return false;
                }
            }
        } else {
            cleanup_bytes = 0;
        }

        /* Load register arguments from the packed array */
        for (argument_index = 0; argument_index < register_arg_count; argument_index++) {
            if (!ae_emit_line(out,
                           "    ldr x%zu, [x17, #%zu]\n",
                           argument_index,
                           argument_index * 8)) {
                free(wrapper_symbol);
                return false;
            }
        }

        if (!ae_emit_line(out, "    bl %s\n", unit_symbol->symbol)) {
            free(wrapper_symbol);
            return false;
        }

        if (cleanup_bytes > 0 && !ae_emit_line(out, "    add sp, sp, #%zu\n", cleanup_bytes)) {
            free(wrapper_symbol);
            return false;
        }

        if (!ae_emit_line(out,
                       "    ldr x28, [x29, #16]\n"
                       "    ldp x29, x30, [sp], #32\n"
                       "    ret\n")) {
            free(wrapper_symbol);
            return false;
        }

        free(wrapper_symbol);
    }

    return true;
}
