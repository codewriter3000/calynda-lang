#include "bytecode.h"
#include "bytecode_internal.h"

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
