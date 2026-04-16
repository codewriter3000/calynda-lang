#include "asm_emit_internal.h"
#include <stdlib.h>
#include <string.h>

/*
 * RV64 program entry glue:
 *   _start               — freestanding boot entry (for boot programs)
 *   calynda_program_start — calls the start unit after registering statics
 *   main                  — calls calynda_rt_start_process
 *   closure wrappers      — unpack call arguments from boxed array
 */

bool ae_emit_program_entry_glue_riscv64(AsmEmitContext *context, FILE *out) {
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
         *   s0 = 0 (frame pointer), ra = 0, s11 = 0 (closure env).
         */
        if (!ae_emit_line(out,
                       ".globl _start\n"
                       "_start:\n"
                       "    li s0, 0\n"
                       "    li ra, 0\n"
                       "    li s11, 0\n"
                       "    call %s\n"
                       "    li a7, 93\n"
                       "    ecall\n",
                       start_symbol->symbol)) {
            return false;
        }
    } else {
        /* calynda_program_start: save ra/s0/s11, register strings, call start */
        if (!ae_emit_line(out,
                       ".globl calynda_program_start\n"
                       "calynda_program_start:\n"
                       "    addi sp, sp, -32\n"
                       "    sd ra, 24(sp)\n"
                       "    sd s0, 16(sp)\n"
                       "    addi s0, sp, 32\n"
                       "    sd s11, 8(sp)\n"
                       "    li s11, 0\n")) {
            return false;
        }

        for (string_index = 0; string_index < context->string_literal_count;
             string_index++) {
            if (!ae_emit_line(out,
                           "    la a0, %s\n"
                           "    call calynda_rt_register_static_object\n",
                           context->string_literals[string_index].object_label)) {
                return false;
            }
        }

        if (!ae_emit_line(out,
                       "    call %s\n"
                       "    ld s11, 8(sp)\n"
                       "    ld s0, 16(sp)\n"
                       "    ld ra, 24(sp)\n"
                       "    addi sp, sp, 32\n"
                       "    ret\n",
                       start_symbol->symbol)) {
            return false;
        }

        /* main: argc in a0, argv in a1.
         *   a2 = argv, a1 = argc, a0 = &calynda_program_start */
        if (!ae_emit_line(out,
                       ".globl main\n"
                       "main:\n"
                       "    addi sp, sp, -16\n"
                       "    sd ra, 8(sp)\n"
                       "    sd s0, 0(sp)\n"
                       "    addi s0, sp, 16\n"
                       "    mv a2, a1\n"
                       "    mv a1, a0\n"
                       "    la a0, calynda_program_start\n"
                       "    call calynda_rt_start_process\n"
                       "    ld s0, 0(sp)\n"
                       "    ld ra, 8(sp)\n"
                       "    addi sp, sp, 16\n"
                       "    ret\n")) {
            return false;
        }
    }

    /* Lambda closure wrappers */
    for (unit_index = 0; unit_index < context->program->unit_count;
         unit_index++) {
        if (!ae_emit_closure_wrapper_riscv64(context, out,
                    &context->program->units[unit_index])) {
            return false;
        }
    }
    return true;
}

/* ------------------------------------------------------------------ */
/*  RV64 closure wrapper emission (split out for line budget)          */
/* ------------------------------------------------------------------ */

bool ae_emit_closure_wrapper_riscv64(AsmEmitContext *context, FILE *out,
                                     const MachineUnit *unit) {
    AsmUnitSymbol *unit_symbol;
    char *wrapper_symbol;
    size_t register_arg_count;
    size_t extra_arg_count;
    size_t argument_index;
    size_t cleanup_bytes;
    size_t stack_pad;

    if (unit->kind != LIR_UNIT_LAMBDA) {
        return true;
    }

    unit_symbol = ae_ensure_unit_symbol(context, unit->name);
    wrapper_symbol = ae_closure_wrapper_symbol_name(unit->name);
    if (!unit_symbol || !wrapper_symbol) {
        free(wrapper_symbol);
        return false;
    }

    extra_arg_count = unit->parameter_count > 8 ? unit->parameter_count - 8 : 0;
    register_arg_count = unit->parameter_count < 8 ? unit->parameter_count : 8;
    {
        size_t total_stack = extra_arg_count * 8;

        stack_pad = (total_stack % 16 != 0) ? (16 - (total_stack % 16)) : 0;
    }

    /* Prologue: save ra, s0, s11; set closure env = a0 */
    if (!ae_emit_line(out,
                   ".globl %s\n"
                   "%s:\n"
                   "    addi sp, sp, -32\n"
                   "    sd ra, 24(sp)\n"
                   "    sd s0, 16(sp)\n"
                   "    addi s0, sp, 32\n"
                   "    sd s11, 8(sp)\n"
                   "    mv s11, a0\n"
                   "    mv t0, a2\n",
                   wrapper_symbol,
                   wrapper_symbol)) {
        free(wrapper_symbol);
        return false;
    }

    /* Push extra arguments (>8) to the stack */
    if (extra_arg_count > 0) {
        cleanup_bytes = (extra_arg_count * 8) + stack_pad;
        if (!ae_emit_line(out, "    addi sp, sp, -%zu\n", cleanup_bytes)) {
            free(wrapper_symbol);
            return false;
        }
        for (argument_index = 0; argument_index < extra_arg_count; argument_index++) {
            size_t src_index = 8 + argument_index;

            if (!ae_emit_line(out,
                           "    ld t1, %zu(t0)\n"
                           "    sd t1, %zu(sp)\n",
                           src_index * 8,
                           argument_index * 8)) {
                free(wrapper_symbol);
                return false;
            }
        }
    } else {
        cleanup_bytes = 0;
    }

    /* Load register arguments from packed array (t0 = args ptr) */
    for (argument_index = 0; argument_index < register_arg_count; argument_index++) {
        if (!ae_emit_line(out, "    ld a%zu, %zu(t0)\n",
                       argument_index, argument_index * 8)) {
            free(wrapper_symbol);
            return false;
        }
    }

    if (!ae_emit_line(out, "    call %s\n", unit_symbol->symbol)) {
        free(wrapper_symbol);
        return false;
    }

    if (cleanup_bytes > 0) {
        if (!ae_emit_line(out, "    addi sp, sp, %zu\n", cleanup_bytes)) {
            free(wrapper_symbol);
            return false;
        }
    }

    if (!ae_emit_line(out,
                   "    ld s11, 8(sp)\n"
                   "    ld s0, 16(sp)\n"
                   "    ld ra, 24(sp)\n"
                   "    addi sp, sp, 32\n"
                   "    ret\n")) {
        free(wrapper_symbol);
        return false;
    }

    free(wrapper_symbol);
    return true;
}
