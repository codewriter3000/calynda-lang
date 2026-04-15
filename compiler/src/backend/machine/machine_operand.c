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

