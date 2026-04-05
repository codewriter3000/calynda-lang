#include "machine_internal.h"

#include <stdlib.h>
#include <string.h>

bool mc_format_capture_operand(size_t capture_index, char **text) {
    if (!text) {
        return false;
    }

    *text = mc_copy_format("env(%zu)", capture_index);
    return *text != NULL;
}

bool mc_format_helper_slot_operand(size_t helper_slot_index, char **text) {
    if (!text) {
        return false;
    }

    *text = mc_copy_format("helper(%zu)", helper_slot_index);
    return *text != NULL;
}

bool mc_format_incoming_arg_stack_operand(size_t argument_index, char **text) {
    if (!text) {
        return false;
    }

    *text = mc_copy_format("argin(%zu)", argument_index);
    return *text != NULL;
}

bool mc_format_outgoing_arg_stack_operand(size_t argument_index, char **text) {
    if (!text) {
        return false;
    }

    *text = mc_copy_format("argout(%zu)", argument_index);
    return *text != NULL;
}

bool mc_format_member_symbol_operand(const char *member, char **text) {
    if (!text) {
        return false;
    }

    *text = mc_copy_format("symbol(%s)", member ? member : "");
    return *text != NULL;
}

bool mc_format_code_label_operand(const char *unit_name, char **text) {
    if (!text) {
        return false;
    }

    *text = mc_copy_format("code(%s)", unit_name ? unit_name : "");
    return *text != NULL;
}

bool mc_format_template_text_operand(const char *text, char **formatted) {
    if (!formatted) {
        return false;
    }

    *formatted = mc_copy_format("text(\"%s\")", text ? text : "");
    return *formatted != NULL;
}

bool mc_emit_move_to_destination(MachineBuildContext *context,
                                 MachineBlock *block,
                                 const char *destination,
                                 const char *source) {
    return mc_append_line(context, block, "mov %s, %s", destination, source);
}

bool mc_emit_store_vreg(MachineBuildContext *context,
                        const CodegenUnit *codegen_unit,
                        size_t vreg_index,
                        MachineBlock *block,
                        const char *source) {
    const CodegenVRegAllocation *allocation;
    const TargetDescriptor *td = mc_target(context);
    const char *work_reg = td->work_register.name;

    if (!codegen_unit || vreg_index >= codegen_unit->vreg_count) {
        mc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Machine emission missing virtual-register allocation %zu.",
                     vreg_index);
        return false;
    }

    allocation = &codegen_unit->vreg_allocations[vreg_index];
    if (allocation->location.kind == CODEGEN_VREG_REGISTER) {
        const char *reg_name = target_register_name(td, allocation->location.as.reg);
        if (strcmp(reg_name, source) == 0) {
            return true;
        }
        return mc_append_line(context,
                              block,
                              "mov %s, %s",
                              reg_name,
                              source);
    }

    if (strcmp(work_reg, source) == 0) {
        return mc_append_line(context,
                              block,
                              "mov spill(%zu), %s",
                              allocation->location.as.spill_slot_index,
                              work_reg);
    }

    return mc_append_line(context, block, "mov %s, %s", work_reg, source) &&
           mc_append_line(context,
                          block,
                          "mov spill(%zu), %s",
                          allocation->location.as.spill_slot_index,
                          work_reg);
}

bool mc_emit_preserve_before_call(MachineBuildContext *context,
                                  const CodegenUnit *codegen_unit,
                                  MachineBlock *block) {
    const TargetDescriptor *td = mc_target(context);
    size_t i;

    /*
     * On x86_64, caller-saved allocatable registers (r10, r11) need to be
     * preserved around calls.  On ARM64, the caller-saved temporaries
     * (x9-x15) used for allocation are also caller-saved and need the
     * same treatment.  We iterate the allocatable set and push any that
     * are actually in use and NOT callee-saved.
     */
    for (i = 0; i < td->allocatable_register_count; i++) {
        int reg_id = td->allocatable_registers[i].id;

        if (target_register_is_callee_saved(td, reg_id)) {
            continue;
        }
        if (mc_unit_uses_register(codegen_unit, reg_id) &&
            !mc_append_line(context, block, "push %s",
                            td->allocatable_registers[i].name)) {
            return false;
        }
    }
    return true;
}

bool mc_emit_preserve_after_call(MachineBuildContext *context,
                                 const CodegenUnit *codegen_unit,
                                 MachineBlock *block) {
    const TargetDescriptor *td = mc_target(context);
    size_t i;

    /* Restore in reverse order */
    for (i = td->allocatable_register_count; i > 0; i--) {
        int reg_id = td->allocatable_registers[i - 1].id;

        if (target_register_is_callee_saved(td, reg_id)) {
            continue;
        }
        if (mc_unit_uses_register(codegen_unit, reg_id) &&
            !mc_append_line(context, block, "pop %s",
                            td->allocatable_registers[i - 1].name)) {
            return false;
        }
    }
    return true;
}

bool mc_emit_entry_prologue(MachineBuildContext *context,
                            const CodegenUnit *codegen_unit,
                            MachineUnit *machine_unit,
                            MachineBlock *block) {
    const TargetDescriptor *td = mc_target(context);
    size_t stack_words;
    size_t i;

    stack_words = mc_unit_stack_word_count(machine_unit);

    /* Push frame pointer and set up frame */
    if (!mc_append_line(context, block, "push %s", td->frame_pointer.name) ||
        !mc_append_line(context, block, "mov %s, %s",
                        td->frame_pointer.name, td->stack_pointer.name)) {
        return false;
    }

    /* Push the work register (always saved) */
    if (!mc_append_line(context, block, "push %s", td->work_register.name)) {
        return false;
    }

    /* Push any callee-saved allocatable registers that are actually used */
    for (i = 0; i < td->allocatable_register_count; i++) {
        int reg_id = td->allocatable_registers[i].id;

        if (!target_register_is_callee_saved(td, reg_id)) {
            continue;
        }
        if (mc_unit_uses_register(codegen_unit, reg_id) &&
            !mc_append_line(context, block, "push %s",
                            td->allocatable_registers[i].name)) {
            return false;
        }
    }

    /* Allocate stack space for locals, spills, helpers, outgoing args */
    if (stack_words > 0 &&
        !mc_append_line(context, block, "sub %s, %zu",
                        td->stack_pointer.name, stack_words * td->word_size)) {
        return false;
    }
    return true;
}

bool mc_emit_return_epilogue(MachineBuildContext *context,
                             const CodegenUnit *codegen_unit,
                             const MachineUnit *machine_unit,
                             MachineBlock *block) {
    const TargetDescriptor *td = mc_target(context);
    size_t stack_words;
    size_t i;

    stack_words = mc_unit_stack_word_count(machine_unit);

    /* Deallocate stack space */
    if (stack_words > 0 &&
        !mc_append_line(context, block, "add %s, %zu",
                        td->stack_pointer.name, stack_words * td->word_size)) {
        return false;
    }

    /* Pop callee-saved allocatable registers in reverse order */
    for (i = td->allocatable_register_count; i > 0; i--) {
        int reg_id = td->allocatable_registers[i - 1].id;

        if (!target_register_is_callee_saved(td, reg_id)) {
            continue;
        }
        if (mc_unit_uses_register(codegen_unit, reg_id) &&
            !mc_append_line(context, block, "pop %s",
                            td->allocatable_registers[i - 1].name)) {
            return false;
        }
    }

    /* Pop work register, frame pointer, and return */
    if (!mc_append_line(context, block, "pop %s", td->work_register.name) ||
        !mc_append_line(context, block, "pop %s", td->frame_pointer.name) ||
        !mc_append_line(context, block, "ret")) {
        return false;
    }
    return true;
}
