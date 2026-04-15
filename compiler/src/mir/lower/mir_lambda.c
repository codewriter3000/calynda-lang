#include "mir_internal.h"

bool mr_build_lambda_unit(MirBuildContext *context,
                          const char *name,
                          const Symbol *symbol,
                          const HirLambdaExpression *lambda,
                          CheckedType return_type,
                          MirUnitKind kind,
                          const MirCaptureList *captures) {
    MirUnit unit;
    MirUnitBuildContext unit_context;
    size_t i;

    memset(&unit, 0, sizeof(unit));
    memset(&unit_context, 0, sizeof(unit_context));
    unit.kind = kind;
    unit.name = ast_copy_text(name);
    unit.symbol = symbol;
    unit.return_type = return_type;
    if (!unit.name) {
        mr_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering MIR units.");
        return false;
    }

    if (captures) {
        for (i = 0; i < captures->count; i++) {
            MirLocal local;
            const char *capture_name = captures->items[i].symbol->name
                                           ? captures->items[i].symbol->name
                                           : "<capture>";

            memset(&local, 0, sizeof(local));
            local.kind = MIR_LOCAL_CAPTURE;
            local.name = ast_copy_text(capture_name);
            local.symbol = captures->items[i].symbol;
            local.type = captures->items[i].type;
            local.is_final = captures->items[i].symbol->is_final;
            local.index = unit.local_count;
            if (!local.name || !mr_append_local(&unit, local)) {
                free(local.name);
                mr_unit_free(&unit);
                mr_set_error(context,
                              captures->items[i].symbol->declaration_span,
                              NULL,
                              "Out of memory while lowering MIR closure captures.");
                return false;
            }
        }
    }

    unit_context.build = context;
    unit_context.unit = &unit;
    if (!mr_create_block(&unit_context, &unit_context.current_block_index) ||
        !mr_lower_parameters(&unit_context, &lambda->parameters) ||
        !mr_lower_block(&unit_context, lambda->body)) {
        mr_unit_free(&unit);
        return false;
    }

    if (!mr_append_unit(context->program, unit)) {
        mr_unit_free(&unit);
        mr_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while assembling MIR units.");
        return false;
    }

    return true;
}

bool mr_lower_lambda_unit(MirBuildContext *context,
                          const char *name,
                          const Symbol *symbol,
                          const HirLambdaExpression *lambda,
                          CheckedType return_type,
                          MirUnitKind kind) {
    MirCaptureList empty_captures;

    memset(&empty_captures, 0, sizeof(empty_captures));
    return mr_build_lambda_unit(context,
                             name,
                             symbol,
                             lambda,
                             return_type,
                             kind,
                             &empty_captures);
}

bool mr_lower_lambda_expression(MirUnitBuildContext *context,
                                const HirExpression *expression,
                                MirValue *value) {
    MirCaptureList captures;
    MirInstruction instruction;
    char unit_name[128];
    const char *parent_name;
    int written;
    size_t i;

    memset(&captures, 0, sizeof(captures));
    if (!context || !expression || !value) {
        return false;
    }

    if (!mr_analyze_lambda_captures(context->build,
                                 &expression->as.lambda,
                                 expression->source_span,
                                 &captures)) {
        return false;
    }

    parent_name = (context->unit && context->unit->name) ? context->unit->name : "lambda";
    written = snprintf(unit_name,
                       sizeof(unit_name),
                       "%s$lambda%zu",
                       parent_name,
                       context->build->next_lambda_unit_index++);
    if (written < 0 || (size_t)written >= sizeof(unit_name)) {
        mr_capture_list_free(&captures);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Failed to name MIR lambda unit.");
        return false;
    }

    if (!mr_build_lambda_unit(context->build,
                           unit_name,
                           NULL,
                           &expression->as.lambda,
                           expression->callable_signature.return_type,
                           MIR_UNIT_LAMBDA,
                           &captures)) {
        mr_capture_list_free(&captures);
        return false;
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_CLOSURE;
    instruction.as.closure.dest_temp = context->unit->next_temp_index++;
    instruction.as.closure.unit_name = ast_copy_text(unit_name);
    if (!instruction.as.closure.unit_name) {
        mr_capture_list_free(&captures);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR closures.");
        return false;
    }

    if (captures.count > 0) {
        instruction.as.closure.captures = calloc(captures.count,
                                                 sizeof(*instruction.as.closure.captures));
        if (!instruction.as.closure.captures) {
            mr_capture_list_free(&captures);
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR closures.");
            return false;
        }
    }
    instruction.as.closure.capture_count = captures.count;

    for (i = 0; i < captures.count; i++) {
        size_t local_index = mr_find_local_index(context->unit, captures.items[i].symbol);

        if (local_index == (size_t)-1) {
            mr_capture_list_free(&captures);
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                          expression->source_span,
                          &captures.items[i].symbol->declaration_span,
                          "Internal error: missing MIR capture for symbol '%s'.",
                          captures.items[i].symbol->name ? captures.items[i].symbol->name : "<anonymous>");
            return false;
        }

        instruction.as.closure.captures[i].kind = MIR_VALUE_LOCAL;
        instruction.as.closure.captures[i].type = captures.items[i].type;
        instruction.as.closure.captures[i].as.local_index = local_index;
    }

    mr_capture_list_free(&captures);

    if (!mr_current_block(context) ||
        !mr_append_instruction(mr_current_block(context), instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR closures.");
        return false;
    }

    value->kind = MIR_VALUE_TEMP;
    value->type = expression->type;
    value->as.temp_index = instruction.as.closure.dest_temp;
    return true;
}

