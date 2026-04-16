#include "machine_internal.h"

#include <stdlib.h>
#include <string.h>

static const char *mc_union_type_tag_name(CalyndaRtTypeTag tag) {
    switch (tag) {
    case CALYNDA_RT_TYPE_VOID: return "void";
    case CALYNDA_RT_TYPE_BOOL: return "bool";
    case CALYNDA_RT_TYPE_INT32: return "int32";
    case CALYNDA_RT_TYPE_INT64: return "int64";
    case CALYNDA_RT_TYPE_STRING: return "string";
    case CALYNDA_RT_TYPE_ARRAY: return "array";
    case CALYNDA_RT_TYPE_CLOSURE: return "closure";
    case CALYNDA_RT_TYPE_EXTERNAL: return "external";
    case CALYNDA_RT_TYPE_UNION: return "union";
    case CALYNDA_RT_TYPE_HETERO_ARRAY: return "hetero_array";
    default: return "raw_word";
    }
}

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

CalyndaRtTypeTag mc_checked_type_to_runtime_tag(CheckedType type) {
    if (type.kind == CHECKED_TYPE_VALUE && type.array_depth > 0) {
        return CALYNDA_RT_TYPE_ARRAY;
    }
    if (type.kind == CHECKED_TYPE_VALUE) {
        switch (type.primitive) {
        case AST_PRIMITIVE_BOOL: return CALYNDA_RT_TYPE_BOOL;
        case AST_PRIMITIVE_STRING: return CALYNDA_RT_TYPE_STRING;
        case AST_PRIMITIVE_INT64:
        case AST_PRIMITIVE_UINT64: return CALYNDA_RT_TYPE_INT64;
        default: return CALYNDA_RT_TYPE_INT32;
        }
    }
    if (type.kind == CHECKED_TYPE_NAMED) {
        return type.name && strcmp(type.name, "arr") == 0
            ? CALYNDA_RT_TYPE_HETERO_ARRAY : CALYNDA_RT_TYPE_UNION;
    }
    if (type.kind == CHECKED_TYPE_EXTERNAL) {
        return CALYNDA_RT_TYPE_EXTERNAL;
    }
    return CALYNDA_RT_TYPE_RAW_WORD;
}

static bool mc_format_type_descriptor_operand(const CalyndaRtTypeDescriptor *type_desc,
                                              char **text) {
    char *encoded;
    size_t i;

    if (!type_desc || !text) {
        return false;
    }
    encoded = mc_copy_format("typedesc(%s|%zu",
                             type_desc->name ? type_desc->name : "?",
                             type_desc->generic_param_count);
    if (!encoded) {
        return false;
    }
    for (i = 0; i < type_desc->generic_param_count; i++) {
        CalyndaRtTypeTag tag = type_desc->generic_param_tags
            ? type_desc->generic_param_tags[i] : CALYNDA_RT_TYPE_RAW_WORD;
        char *next = mc_copy_format("%s|g%zu:%s", encoded, i, mc_union_type_tag_name(tag));

        free(encoded);
        if (!next) {
            return false;
        }
        encoded = next;
    }
    for (i = 0; i < type_desc->variant_count; i++) {
        const char *variant_name =
            type_desc->variant_names ? type_desc->variant_names[i] : NULL;
        CalyndaRtTypeTag tag = type_desc->variant_payload_tags
            ? type_desc->variant_payload_tags[i]
            : CALYNDA_RT_TYPE_RAW_WORD;
        char *next = variant_name
            ? mc_copy_format("%s|%s:%s",
                             encoded,
                             variant_name,
                             mc_union_type_tag_name(tag))
            : mc_copy_format("%s|%zu:%s",
                             encoded,
                             i,
                             mc_union_type_tag_name(tag));

        free(encoded);
        if (!next) {
            return false;
        }
        encoded = next;
    }

    *text = mc_copy_format("%s)", encoded);
    free(encoded);
    return *text != NULL;
}

bool mc_format_hetero_array_type_descriptor_operand(const LirInstruction *instruction,
                                                    char **text) {
    return instruction &&
        mc_format_type_descriptor_operand(&instruction->as.hetero_array_new.type_desc, text);
}

bool mc_format_union_type_descriptor_operand(const LirInstruction *instruction,
                                             char **text) {
    return instruction &&
        mc_format_type_descriptor_operand(&instruction->as.union_new.type_desc, text);
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

