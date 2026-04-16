#include "bytecode.h"
#include "bytecode_internal.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

bool bc_source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

void bc_set_error(BytecodeBuildContext *context,
                  AstSourceSpan primary_span,
                  const AstSourceSpan *related_span,
                  const char *format,
                  ...) {
    va_list args;

    if (!context || !context->program || context->program->has_error) {
        return;
    }

    context->program->has_error = true;
    context->program->error.primary_span = primary_span;
    if (related_span && bc_source_span_is_valid(*related_span)) {
        context->program->error.related_span = *related_span;
        context->program->error.has_related_span = true;
    }

    va_start(args, format);
    vsnprintf(context->program->error.message,
              sizeof(context->program->error.message),
              format,
              args);
    va_end(args);
}

BytecodeValue bc_invalid_value(void) {
    BytecodeValue value;

    memset(&value, 0, sizeof(value));
    value.kind = BYTECODE_VALUE_INVALID;
    return value;
}

BytecodeLocalKind bc_local_kind_from_mir(MirLocalKind kind) {
    switch (kind) {
    case MIR_LOCAL_PARAMETER:
        return BYTECODE_LOCAL_PARAMETER;
    case MIR_LOCAL_LOCAL:
        return BYTECODE_LOCAL_LOCAL;
    case MIR_LOCAL_CAPTURE:
        return BYTECODE_LOCAL_CAPTURE;
    case MIR_LOCAL_SYNTHETIC:
        return BYTECODE_LOCAL_SYNTHETIC;
    }

    return BYTECODE_LOCAL_LOCAL;
}

BytecodeUnitKind bc_unit_kind_from_mir(MirUnitKind kind) {
    switch (kind) {
    case MIR_UNIT_START:
        return BYTECODE_UNIT_START;
    case MIR_UNIT_BINDING:
        return BYTECODE_UNIT_BINDING;
    case MIR_UNIT_INIT:
        return BYTECODE_UNIT_INIT;
    case MIR_UNIT_LAMBDA:
        return BYTECODE_UNIT_LAMBDA;
    case MIR_UNIT_ASM:
        break;
    }

    return BYTECODE_UNIT_BINDING;
}

size_t bc_find_unit_index(const MirProgram *program, const char *name) {
    size_t i;

    if (!program || !name) {
        return (size_t)-1;
    }

    for (i = 0; i < program->unit_count; i++) {
        if (strcmp(program->units[i].name, name) == 0) {
            return i;
        }
    }

    return (size_t)-1;
}

CalyndaRtTypeTag bc_checked_type_to_runtime_tag(CheckedType type) {
    if (type.kind == CHECKED_TYPE_VALUE && type.array_depth > 0) {
        return CALYNDA_RT_TYPE_ARRAY;
    }
    if (type.kind == CHECKED_TYPE_VALUE && type.array_depth == 0) {
        switch (type.primitive) {
        case AST_PRIMITIVE_BOOL:    return CALYNDA_RT_TYPE_BOOL;
        case AST_PRIMITIVE_STRING:  return CALYNDA_RT_TYPE_STRING;
        case AST_PRIMITIVE_INT64:
        case AST_PRIMITIVE_UINT64:  return CALYNDA_RT_TYPE_INT64;
        default:                    return CALYNDA_RT_TYPE_INT32;
        }
    }
    if (type.kind == CHECKED_TYPE_NAMED) {
        if (type.name && strcmp(type.name, "arr") == 0) {
            return CALYNDA_RT_TYPE_HETERO_ARRAY;
        }
        return CALYNDA_RT_TYPE_UNION;
    }
    if (type.kind == CHECKED_TYPE_EXTERNAL) {
        return CALYNDA_RT_TYPE_EXTERNAL;
    }
    return CALYNDA_RT_TYPE_INT32;
}

size_t bc_intern_union_type_descriptor(BytecodeBuildContext *context,
                                       const char *name,
                                       size_t generic_param_count,
                                       const CalyndaRtTypeTag *generic_param_tags,
                                       const char *const *variant_names,
                                       const CalyndaRtTypeTag *variant_payload_tags,
                                       size_t variant_count) {
    size_t i;
    size_t g;
    size_t v;
    if (!context || !context->program || !name) {
        return (size_t)-1;
    }

    for (i = 0; i < context->program->constant_count; i++) {
        BytecodeConstant *constant = &context->program->constants[i];

        if (constant->kind != BYTECODE_CONSTANT_TYPE_DESCRIPTOR ||
            constant->as.type_descriptor.generic_param_count != generic_param_count ||
            constant->as.type_descriptor.variant_count != variant_count ||
            strcmp(constant->as.type_descriptor.name, name) != 0) {
            continue;
        }
        for (g = 0; g < generic_param_count; g++) {
            CalyndaRtTypeTag requested_tag = generic_param_tags
                ? generic_param_tags[g] : CALYNDA_RT_TYPE_RAW_WORD;

            if (constant->as.type_descriptor.generic_param_tags[g] == requested_tag) {
                continue;
            }
            if (constant->as.type_descriptor.generic_param_tags[g] == CALYNDA_RT_TYPE_RAW_WORD &&
                requested_tag != CALYNDA_RT_TYPE_RAW_WORD) {
                constant->as.type_descriptor.generic_param_tags[g] = requested_tag;
                continue;
            }
            if (requested_tag == CALYNDA_RT_TYPE_RAW_WORD) {
                continue;
            }
            break;
        }
        if (g != generic_param_count) {
            continue;
        }
        for (v = 0; v < variant_count; v++) {
            const char *existing_name = constant->as.type_descriptor.variant_names[v];
            const char *requested_name = variant_names ? variant_names[v] : NULL;

            if ((existing_name == NULL) != (requested_name == NULL)) {
                break;
            }
            if (existing_name && strcmp(existing_name, requested_name) != 0) {
                break;
            }
            if (constant->as.type_descriptor.variant_payload_tags[v] ==
                variant_payload_tags[v]) {
                continue;
            }
            if (constant->as.type_descriptor.variant_payload_tags[v] == CALYNDA_RT_TYPE_RAW_WORD &&
                variant_payload_tags[v] != CALYNDA_RT_TYPE_RAW_WORD) {
                constant->as.type_descriptor.variant_payload_tags[v] =
                    variant_payload_tags[v];
                continue;
            }
            if (variant_payload_tags[v] == CALYNDA_RT_TYPE_RAW_WORD) {
                continue;
            }
            break;
        }
        if (v == variant_count) {
            return i;
        }
    }

    if (!bc_reserve_items((void **)&context->program->constants,
                          &context->program->constant_capacity,
                          context->program->constant_count + 1,
                          sizeof(*context->program->constants))) {
        return (size_t)-1;
    }

    i = context->program->constant_count++;
    memset(&context->program->constants[i], 0, sizeof(context->program->constants[i]));
    context->program->constants[i].kind = BYTECODE_CONSTANT_TYPE_DESCRIPTOR;
    context->program->constants[i].as.type_descriptor.name = ast_copy_text(name);
    context->program->constants[i].as.type_descriptor.generic_param_count = generic_param_count;
    context->program->constants[i].as.type_descriptor.variant_count = variant_count;
    if (!context->program->constants[i].as.type_descriptor.name) {
        return (size_t)-1;
    }
    if (generic_param_count > 0) {
        context->program->constants[i].as.type_descriptor.generic_param_tags = calloc(
            generic_param_count,
            sizeof(*context->program->constants[i].as.type_descriptor.generic_param_tags));
        if (!context->program->constants[i].as.type_descriptor.generic_param_tags) {
            return (size_t)-1;
        }
        for (g = 0; g < generic_param_count; g++) {
            context->program->constants[i].as.type_descriptor.generic_param_tags[g] =
                generic_param_tags ? generic_param_tags[g] : CALYNDA_RT_TYPE_RAW_WORD;
        }
    }
    if (variant_count == 0) {
        return i;
    }

    context->program->constants[i].as.type_descriptor.variant_names = calloc(
        variant_count,
        sizeof(*context->program->constants[i].as.type_descriptor.variant_names));
    context->program->constants[i].as.type_descriptor.variant_payload_tags = calloc(
        variant_count,
        sizeof(*context->program->constants[i].as.type_descriptor.variant_payload_tags));
    if (!context->program->constants[i].as.type_descriptor.variant_names ||
        !context->program->constants[i].as.type_descriptor.variant_payload_tags) {
        return (size_t)-1;
    }

    for (v = 0; v < variant_count; v++) {
        if (variant_names && variant_names[v]) {
            context->program->constants[i].as.type_descriptor.variant_names[v] =
                ast_copy_text(variant_names[v]);
            if (!context->program->constants[i].as.type_descriptor.variant_names[v]) {
                return (size_t)-1;
            }
        }
        context->program->constants[i].as.type_descriptor.variant_payload_tags[v] =
            variant_payload_tags[v];
    }

    return i;
}
