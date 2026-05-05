#include "codegen_internal.h"

static bool cg_runtime_helper_for_global_name(const char *name,
                                              CodegenRuntimeHelper *helper_out) {
    if (!name || !helper_out) {
        return false;
    }

    if (strcmp(name, CALYNDA_TYPEOF) == 0) {
        *helper_out = CODEGEN_RUNTIME_TYPEOF;
        return true;
    }
    if (strcmp(name, CALYNDA_ISINT) == 0) {
        *helper_out = CODEGEN_RUNTIME_ISINT;
        return true;
    }
    if (strcmp(name, CALYNDA_ISFLOAT) == 0) {
        *helper_out = CODEGEN_RUNTIME_ISFLOAT;
        return true;
    }
    if (strcmp(name, CALYNDA_ISBOOL) == 0) {
        *helper_out = CODEGEN_RUNTIME_ISBOOL;
        return true;
    }
    if (strcmp(name, CALYNDA_ISSTRING) == 0) {
        *helper_out = CODEGEN_RUNTIME_ISSTRING;
        return true;
    }
    if (strcmp(name, CALYNDA_ISARRAY) == 0) {
        *helper_out = CODEGEN_RUNTIME_ISARRAY;
        return true;
    }
    if (strcmp(name, CALYNDA_ISSAMETYPE) == 0) {
        *helper_out = CODEGEN_RUNTIME_ISSAMETYPE;
        return true;
    }

    return false;
}

bool cg_source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

void cg_set_error(CodegenBuildContext *context,
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
    if (related_span && cg_source_span_is_valid(*related_span)) {
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

void cg_block_free(CodegenBlock *block) {
    if (!block) {
        return;
    }

    free(block->label);
    free(block->instructions);
    memset(block, 0, sizeof(*block));
}

void cg_unit_free(CodegenUnit *unit) {
    size_t i;

    if (!unit) {
        return;
    }

    free(unit->name);
    for (i = 0; i < unit->frame_slot_count; i++) {
        free(unit->frame_slots[i].name);
    }
    free(unit->frame_slots);
    free(unit->vreg_allocations);
    for (i = 0; i < unit->block_count; i++) {
        cg_block_free(&unit->blocks[i]);
    }
    free(unit->blocks);
    free(unit->asm_body);
    memset(unit, 0, sizeof(*unit));
}

CheckedType cg_checked_type_invalid_value(void) {
    CheckedType type;

    memset(&type, 0, sizeof(type));
    type.kind = CHECKED_TYPE_INVALID;
    return type;
}

bool cg_is_direct_scalar_type(CheckedType type) {
    return type.kind == CHECKED_TYPE_VALUE && type.array_depth == 0;
}

bool cg_select_instruction(CodegenBuildContext *context,
                           const LirInstruction *instruction,
                           CodegenSelectedInstruction *selected) {
    if (!instruction || !selected) {
        return false;
    }

    memset(selected, 0, sizeof(*selected));
    selected->kind = instruction->kind;

    switch (instruction->kind) {
    case LIR_INSTR_INCOMING_ARG:
        selected->selection.kind = CODEGEN_SELECTION_DIRECT;
        selected->selection.as.direct_pattern = CODEGEN_DIRECT_ABI_ARG_MOVE;
        return true;

    case LIR_INSTR_INCOMING_CAPTURE:
        selected->selection.kind = CODEGEN_SELECTION_DIRECT;
        selected->selection.as.direct_pattern = CODEGEN_DIRECT_ABI_CAPTURE_LOAD;
        return true;

    case LIR_INSTR_OUTGOING_ARG:
        selected->selection.kind = CODEGEN_SELECTION_DIRECT;
        selected->selection.as.direct_pattern = CODEGEN_DIRECT_ABI_OUTGOING_ARG;
        return true;

    case LIR_INSTR_BINARY:
        selected->selection.kind = CODEGEN_SELECTION_DIRECT;
        selected->selection.as.direct_pattern = CODEGEN_DIRECT_SCALAR_BINARY;
        return true;

    case LIR_INSTR_UNARY:
        selected->selection.kind = CODEGEN_SELECTION_DIRECT;
        selected->selection.as.direct_pattern = CODEGEN_DIRECT_SCALAR_UNARY;
        return true;

    case LIR_INSTR_CLOSURE:
        selected->selection.kind = CODEGEN_SELECTION_RUNTIME;
        selected->selection.as.runtime_helper = CODEGEN_RUNTIME_CLOSURE_NEW;
        return true;

    case LIR_INSTR_CALL:
        if (instruction->as.call.callee.kind == LIR_OPERAND_GLOBAL) {
            CodegenRuntimeHelper runtime_helper;

            if (cg_runtime_helper_for_global_name(instruction->as.call.callee.as.global_name,
                                                  &runtime_helper)) {
                selected->selection.kind = CODEGEN_SELECTION_RUNTIME;
                selected->selection.as.runtime_helper = runtime_helper;
            } else {
                selected->selection.kind = CODEGEN_SELECTION_DIRECT;
                selected->selection.as.direct_pattern = CODEGEN_DIRECT_CALL_GLOBAL;
            }
        } else {
            selected->selection.kind = CODEGEN_SELECTION_RUNTIME;
            selected->selection.as.runtime_helper = CODEGEN_RUNTIME_CALL_CALLABLE;
        }
        return true;

    case LIR_INSTR_CAST:
        if (cg_is_direct_scalar_type(instruction->as.cast.target_type) &&
            cg_is_direct_scalar_type(instruction->as.cast.operand.type)) {
            selected->selection.kind = CODEGEN_SELECTION_DIRECT;
            selected->selection.as.direct_pattern = CODEGEN_DIRECT_SCALAR_CAST;
        } else {
            selected->selection.kind = CODEGEN_SELECTION_RUNTIME;
            selected->selection.as.runtime_helper = CODEGEN_RUNTIME_CAST_VALUE;
        }
        return true;

    case LIR_INSTR_MEMBER:
        selected->selection.kind = CODEGEN_SELECTION_RUNTIME;
        selected->selection.as.runtime_helper = CODEGEN_RUNTIME_MEMBER_LOAD;
        return true;

    case LIR_INSTR_INDEX_LOAD:
        selected->selection.kind = CODEGEN_SELECTION_RUNTIME;
        selected->selection.as.runtime_helper = CODEGEN_RUNTIME_INDEX_LOAD;
        return true;

    case LIR_INSTR_ARRAY_LITERAL:
        selected->selection.kind = CODEGEN_SELECTION_RUNTIME;
        selected->selection.as.runtime_helper = CODEGEN_RUNTIME_ARRAY_LITERAL;
        return true;

    case LIR_INSTR_TEMPLATE:
        selected->selection.kind = CODEGEN_SELECTION_RUNTIME;
        selected->selection.as.runtime_helper = CODEGEN_RUNTIME_TEMPLATE_BUILD;
        return true;

    case LIR_INSTR_STORE_SLOT:
        selected->selection.kind = CODEGEN_SELECTION_DIRECT;
        selected->selection.as.direct_pattern = CODEGEN_DIRECT_STORE_SLOT;
        return true;

    case LIR_INSTR_STORE_GLOBAL:
        selected->selection.kind = CODEGEN_SELECTION_DIRECT;
        selected->selection.as.direct_pattern = CODEGEN_DIRECT_STORE_GLOBAL;
        return true;

    case LIR_INSTR_STORE_INDEX:
        selected->selection.kind = CODEGEN_SELECTION_RUNTIME;
        selected->selection.as.runtime_helper = CODEGEN_RUNTIME_STORE_INDEX;
        return true;

    case LIR_INSTR_STORE_MEMBER:
        selected->selection.kind = CODEGEN_SELECTION_RUNTIME;
        selected->selection.as.runtime_helper = CODEGEN_RUNTIME_STORE_MEMBER;
        return true;

    case LIR_INSTR_UNION_NEW:
        selected->selection.kind = CODEGEN_SELECTION_RUNTIME;
        selected->selection.as.runtime_helper = CODEGEN_RUNTIME_UNION_NEW;
        return true;

    case LIR_INSTR_UNION_GET_TAG:
        selected->selection.kind = CODEGEN_SELECTION_RUNTIME;
        selected->selection.as.runtime_helper = CODEGEN_RUNTIME_UNION_GET_TAG;
        return true;

    case LIR_INSTR_UNION_GET_PAYLOAD:
        selected->selection.kind = CODEGEN_SELECTION_RUNTIME;
        selected->selection.as.runtime_helper = CODEGEN_RUNTIME_UNION_GET_PAYLOAD;
        return true;

    case LIR_INSTR_HETERO_ARRAY_NEW:
        selected->selection.kind = CODEGEN_SELECTION_RUNTIME;
        selected->selection.as.runtime_helper = CODEGEN_RUNTIME_HETERO_ARRAY_NEW;
        return true;
    }

#include "codegen_helpers_p2.inc"
