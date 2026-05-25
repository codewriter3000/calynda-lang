#include "asm_emit_internal.h"
#include "runtime.h"
#include <stdlib.h>
#include <string.h>

static const MachineStaticArrayBinding *ae_find_static_array_binding(
    const AsmEmitContext *context,
    const char *global_name) {
    size_t i;

    if (!context || !context->program || !global_name) {
        return NULL;
    }

    for (i = 0; i < context->program->static_array_binding_count; i++) {
        if (context->program->static_array_bindings[i].global_name &&
            strcmp(context->program->static_array_bindings[i].global_name, global_name) == 0) {
            return &context->program->static_array_bindings[i];
        }
    }

    return NULL;
}

static void ae_mark_static_array_object_reachable(const MachineProgram *program,
                                                  bool *reachable_objects,
                                                  size_t object_index) {
    const MachineStaticArrayObject *object;
    size_t element_index;

    if (!program || !reachable_objects ||
        object_index >= program->static_array_object_count ||
        reachable_objects[object_index]) {
        return;
    }

    reachable_objects[object_index] = true;
    object = &program->static_array_objects[object_index];
    for (element_index = 0; element_index < object->element_count; element_index++) {
        if (object->elements[element_index].kind == MACHINE_STATIC_ARRAY_ELEMENT_OBJECT) {
            ae_mark_static_array_object_reachable(program,
                                                  reachable_objects,
                                                  object->elements[element_index].object_index);
        }
    }
}

bool ae_emit_unit_text(AsmEmitContext *context,
                           FILE *out,
                           size_t unit_index,
                           const MachineUnit *unit) {
    AsmUnitSymbol *symbol;
    AsmUnitLayout layout;
    size_t block_index;
    bool is_arm64 = context->program->target_desc &&
                    context->program->target_desc->kind == TARGET_KIND_AARCH64_AAPCS_ELF;
    bool is_riscv64 = context->program->target_desc &&
                      context->program->target_desc->kind == TARGET_KIND_RISCV64_LP64D_ELF;

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
            !ae_emit_line(out, "    stp x29, x30, [sp, #-16]!\n    mov x29, sp\n"))
            return false;
        if (frame_size > 0 && !ae_emit_line(out, "    sub sp, sp, #%zu\n", frame_size))
            return false;
        if (!ae_emit_line(out, "    str x16, [x29, #-8]\n"))
            return false;
    } else if (is_riscv64) {
        size_t frame_size;

        layout = ae_compute_unit_layout_riscv64(unit);
        frame_size = (layout.saved_reg_words + layout.total_local_words) * 8;
        if (!ae_emit_line(out, ".globl %s\n%s:\n", symbol->symbol, symbol->symbol) ||
            !ae_emit_line(out, "    addi sp, sp, -16\n    sd ra, 8(sp)\n"
                       "    sd s0, 0(sp)\n    addi s0, sp, 16\n"))
            return false;
        if (frame_size > 0 && !ae_rv64_emit_stack_adjust(out, -(long long)frame_size))
            return false;
        if (!ae_emit_line(out, "    sd t0, -24(s0)\n    sd s1, -32(s0)\n"))
            return false;
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
            const char *itext = unit->blocks[block_index].instructions[instruction_index].text;
            bool instr_ok;

            if (is_arm64) {
                instr_ok = ae_emit_machine_instruction_aarch64(
                    context, out, unit, &layout, unit_index,
                    block_index, instruction_index, itext);
            } else if (is_riscv64) {
                instr_ok = ae_emit_machine_instruction_riscv64(
                    context, out, unit, &layout, unit_index,
                    block_index, instruction_index, itext);
            } else {
                instr_ok = ae_emit_machine_instruction(
                    context, out, unit, &layout, unit_index,
                    block_index, instruction_index, itext);
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
    size_t static_array_index;
    bool *reachable_static_array_objects = NULL;
    bool has_reachable_static_arrays = false;

    if (!context) {
        return false;
    }
    if (context->program && context->program->static_array_object_count > 0) {
        reachable_static_array_objects = calloc(context->program->static_array_object_count,
                                                sizeof(*reachable_static_array_objects));
        if (!reachable_static_array_objects) {
            return false;
        }
        for (static_array_index = 0;
             static_array_index < context->program->static_array_binding_count;
             static_array_index++) {
            if (!ae_is_static_array_binding_reachable(
                    context,
                    &context->program->static_array_bindings[static_array_index])) {
                continue;
            }
            ae_mark_static_array_object_reachable(
                context->program,
                reachable_static_array_objects,
                context->program->static_array_bindings[static_array_index].object_index);
        }
        for (static_array_index = 0;
             static_array_index < context->program->static_array_object_count;
             static_array_index++) {
            if (reachable_static_array_objects[static_array_index]) {
                has_reachable_static_arrays = true;
                break;
            }
        }
    }
    if (context->byte_literal_count == 0 &&
        context->string_literal_count == 0 &&
        !has_reachable_static_arrays) {
        free(reachable_static_array_objects);
        return true;
    }
    if (!ae_emit_line(out, ".section .rodata\n")) {
        free(reachable_static_array_objects);
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

    if (context->program) {
        for (static_array_index = 0;
             static_array_index < context->program->static_array_object_count;
             static_array_index++) {
            const MachineStaticArrayObject *object =
                &context->program->static_array_objects[static_array_index];

            if (reachable_static_array_objects &&
                !reachable_static_array_objects[static_array_index]) {
                continue;
            }

            if (object->element_count > 0) {
                if (!ae_emit_line(out,
                                  "    .balign 8\n"
                                  ".Larr_elems_%zu:\n",
                                  static_array_index)) {
                    return false;
                }
                for (j = 0; j < object->element_count; j++) {
                    if (object->elements[j].kind == MACHINE_STATIC_ARRAY_ELEMENT_OBJECT) {
                        if (!ae_emit_line(out,
                                          "    .quad .Larr_obj_%zu\n",
                                          object->elements[j].object_index)) {
                            return false;
                        }
                    } else if (!ae_emit_line(out,
                                             "    .quad %llu\n",
                                             (unsigned long long)object->elements[j].word)) {
                        return false;
                    }
                }
            }

            if (object->element_count > 0) {
                if (!ae_emit_line(out,
                                  "    .balign 8\n"
                                  ".Larr_obj_%zu:\n"
                                  "    .long %u\n"
                                  "    .long %u\n"
                                  "    .quad 0\n"
                                  "    .quad %zu\n"
                                  "    .quad .Larr_elems_%zu\n",
                                  static_array_index,
                                  CALYNDA_RT_OBJECT_MAGIC,
                                  CALYNDA_RT_OBJECT_ARRAY,
                                  object->element_count,
                                  static_array_index)) {
                    return false;
                }
            } else if (!ae_emit_line(out,
                                     "    .balign 8\n"
                                     ".Larr_obj_%zu:\n"
                                     "    .long %u\n"
                                     "    .long %u\n"
                                     "    .quad 0\n"
                                     "    .quad 0\n"
                                     "    .quad 0\n",
                                     static_array_index,
                                     CALYNDA_RT_OBJECT_MAGIC,
                                     CALYNDA_RT_OBJECT_ARRAY)) {
                return false;
            }
        }
    }

    free(reachable_static_array_objects);
    return true;
}

#include "asm_emit_sections_p2.inc"

