#include "mir_internal.h"
#include <string.h>

static bool mr_is_direct_cleanup_callee(const MirValue *callee) {
    return callee && callee->kind == MIR_VALUE_GLOBAL && callee->as.global_name &&
           (strncmp(callee->as.global_name, "__calynda_", 10) == 0 ||
            strcmp(callee->as.global_name, "malloc") == 0 ||
            strcmp(callee->as.global_name, "calloc") == 0 ||
            strcmp(callee->as.global_name, "realloc") == 0 ||
            strcmp(callee->as.global_name, "free") == 0);
}

static void mr_deferred_cleanup_free(MirDeferredCleanup *cleanup) {
    if (!cleanup) {
        return;
    }
    mr_value_free(&cleanup->argument);
    mr_value_free(&cleanup->callee);
    memset(cleanup, 0, sizeof(*cleanup));
}
static bool mr_snapshot_cleanup_value(MirUnitBuildContext *context,
                                      const char *name_prefix,
                                      const MirValue *value,
                                      AstSourceSpan source_span,
                                      MirValue *snapshot) {
    MirValue stored_value;
    size_t local_index;
    if (!mr_value_clone(context->build, value, &stored_value)) {
        return false;
    }
    if (!mr_append_synthetic_local(context,
                                   name_prefix,
                                   value->type,
                                   source_span,
                                   &local_index)) {
        mr_value_free(&stored_value);
        return false;
    }
    if (!mr_append_store_local_instruction(context,
                                           local_index,
                                           stored_value,
                                           source_span)) {
        return false;
    }
    snapshot->kind = MIR_VALUE_LOCAL;
    snapshot->type = value->type;
    snapshot->as.local_index = local_index;
    return true;
}
bool mr_register_manual_cleanup(MirUnitBuildContext *context,
                                const MirValue *argument,
                                const MirValue *callee,
                                AstSourceSpan source_span,
                                MirValue *captured_argument) {
    MirDeferredCleanup cleanup;
    if (!context || context->manual_cleanup_depth == 0) {
        mr_set_error(context->build,
                     source_span,
                     NULL,
                     "cleanup() requires an enclosing manual block.");
        return false;
    }
    memset(&cleanup, 0, sizeof(cleanup));
    if (!mr_snapshot_cleanup_value(context,
                                   "cleanup_arg",
                                   argument,
                                   source_span,
                                   &cleanup.argument) ||
        !(mr_is_direct_cleanup_callee(callee)
              ? mr_value_clone(context->build, callee, &cleanup.callee)
              : mr_snapshot_cleanup_value(context,
                                          "cleanup_fn",
                                          callee,
                                          source_span,
                                          &cleanup.callee))) {
        mr_deferred_cleanup_free(&cleanup);
        return false;
    }
    cleanup.source_span = source_span;
    if (!mr_reserve_items((void **)&context->deferred_cleanups,
                          &context->deferred_cleanup_capacity,
                          context->deferred_cleanup_count + 1,
                          sizeof(*context->deferred_cleanups))) {
        mr_deferred_cleanup_free(&cleanup);
        mr_set_error(context->build,
                     source_span,
                     NULL,
                     "Out of memory while registering manual cleanup.");
        return false;
    }
    context->deferred_cleanups[context->deferred_cleanup_count++] = cleanup;
    if (captured_argument) {
        *captured_argument = mr_invalid_value();
        captured_argument->kind = MIR_VALUE_LOCAL;
        captured_argument->type = cleanup.argument.type;
        captured_argument->as.local_index = cleanup.argument.as.local_index;
    }
    return true;
}
static bool mr_append_cleanup_call(MirUnitBuildContext *context,
                                   MirDeferredCleanup *cleanup) {
    MirInstruction instruction;
    MirBasicBlock *block;
    block = mr_current_block(context);
    if (!block) {
        mr_set_error(context->build,
                     cleanup->source_span,
                     NULL,
                     "Internal error: missing current MIR block for cleanup emission.");
        return false;
    }
    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_CALL;
    instruction.as.call.callee = cleanup->callee;
    instruction.as.call.argument_count = 1;
    instruction.as.call.arguments = calloc(1, sizeof(*instruction.as.call.arguments));
    if (!instruction.as.call.arguments) {
        instruction.as.call.callee = mr_invalid_value();
        mr_set_error(context->build,
                     cleanup->source_span,
                     NULL,
                     "Out of memory while emitting manual cleanup calls.");
        return false;
    }
    instruction.as.call.arguments[0] = cleanup->argument;
    cleanup->callee = mr_invalid_value();
    cleanup->argument = mr_invalid_value();
    if (!mr_append_instruction(block, instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                     cleanup->source_span,
                     NULL,
                     "Out of memory while emitting manual cleanup calls.");
        return false;
    }
    return true;
}
static bool mr_emit_cleanup_range(MirUnitBuildContext *context, size_t start_count) {
    size_t cleanup_index;
    for (cleanup_index = context->deferred_cleanup_count;
         cleanup_index > start_count;
         cleanup_index--) {
        MirDeferredCleanup *cleanup = &context->deferred_cleanups[cleanup_index - 1];
        if (!mr_append_cleanup_call(context, cleanup)) {
            return false;
        }
    }
    return true;
}
bool mr_enter_manual_scope(MirUnitBuildContext *context,
                           AstSourceSpan source_span) {
    if (!mr_reserve_items((void **)&context->manual_cleanup_markers,
                          &context->manual_cleanup_capacity,
                          context->manual_cleanup_depth + 1,
                          sizeof(*context->manual_cleanup_markers))) {
        mr_set_error(context->build,
                     source_span,
                     NULL,
                     "Out of memory while tracking manual cleanup scopes.");
        return false;
    }
    context->manual_cleanup_markers[context->manual_cleanup_depth++] =
        context->deferred_cleanup_count;
    return true;
}
bool mr_leave_manual_scope(MirUnitBuildContext *context,
                           AstSourceSpan source_span) {
    MirBasicBlock *block;
    size_t start_count;
    size_t cleanup_index;
    if (!context || context->manual_cleanup_depth == 0) {
        mr_set_error(context->build,
                     source_span,
                     NULL,
                     "Internal error: missing manual cleanup scope.");
        return false;
    }
    block = mr_current_block(context);
    if (!block) {
        mr_set_error(context->build,
                     source_span,
                     NULL,
                     "Internal error: missing current MIR block at manual scope exit.");
        return false;
    }
    start_count = context->manual_cleanup_markers[context->manual_cleanup_depth - 1];
    if (block->terminator.kind == MIR_TERM_NONE &&
        !mr_emit_cleanup_range(context, start_count)) {
        return false;
    }
    for (cleanup_index = start_count;
         cleanup_index < context->deferred_cleanup_count;
         cleanup_index++) {
        mr_deferred_cleanup_free(&context->deferred_cleanups[cleanup_index]);
    }
    context->deferred_cleanup_count = start_count;
    context->manual_cleanup_depth--;
    return true;
}
bool mr_emit_active_cleanups(MirUnitBuildContext *context,
                             AstSourceSpan source_span) {
    if (!context || context->deferred_cleanup_count == 0) {
        return true;
    }
    if (!mr_current_block(context)) {
        mr_set_error(context->build,
                     source_span,
                     NULL,
                     "Internal error: missing current MIR block for cleanup emission.");
        return false;
    }
    return mr_emit_cleanup_range(context, 0);
}
void mr_free_manual_cleanup_state(MirUnitBuildContext *context) {
    size_t cleanup_index;
    if (!context) {
        return;
    }
    for (cleanup_index = 0;
         cleanup_index < context->deferred_cleanup_count;
         cleanup_index++) {
        mr_deferred_cleanup_free(&context->deferred_cleanups[cleanup_index]);
    }
    free(context->deferred_cleanups);
    free(context->manual_cleanup_markers);
    context->deferred_cleanups = NULL;
    context->deferred_cleanup_count = 0;
    context->deferred_cleanup_capacity = 0;
    context->manual_cleanup_markers = NULL;
    context->manual_cleanup_depth = 0;
    context->manual_cleanup_capacity = 0;
}