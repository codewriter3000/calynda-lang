#include "mir_internal.h"

bool mr_is_union_variant_call(const HirExpression *expression,
                              const char **out_union_name,
                              const char **out_variant_name) {
    const HirExpression *callee;
    const HirExpression *target;

    callee = expression ? expression->as.call.callee : NULL;
    target = callee ? callee->as.member.target : NULL;
    if (!expression || expression->kind != HIR_EXPR_CALL ||
        !callee || callee->kind != HIR_EXPR_MEMBER ||
        !target || target->kind != HIR_EXPR_SYMBOL ||
        target->as.symbol.kind != SYMBOL_KIND_UNION) {
        return false;
    }

    if (out_union_name) *out_union_name = target->as.symbol.name;
    if (out_variant_name) *out_variant_name = callee->as.member.member;
    return true;
}

bool mr_is_union_variant_member(const HirExpression *expression,
                                const char **out_union_name,
                                const char **out_variant_name) {
    const HirExpression *target;

    target = expression ? expression->as.member.target : NULL;
    if (!expression || expression->kind != HIR_EXPR_MEMBER ||
        !target || target->kind != HIR_EXPR_SYMBOL ||
        target->as.symbol.kind != SYMBOL_KIND_UNION) {
        return false;
    }

    if (out_union_name) *out_union_name = target->as.symbol.name;
    if (out_variant_name) *out_variant_name = expression->as.member.member;
    return true;
}

bool mr_init_union_new_instruction(MirBuildContext *build,
                                   MirInstruction *instruction,
                                   const char *union_name,
                                   AstSourceSpan source_span) {
    const HirUnionDecl *decl = mr_find_hir_union_decl(build, union_name);
    CalyndaRtTypeTag *generic_param_tags = NULL;
    char **variant_names = NULL;
    CalyndaRtTypeTag *variant_payload_tags = NULL;
    size_t v;
    if (!instruction || !decl) {
        mr_set_error(build, source_span, NULL,
                     "Internal error: union '%s' not found in HIR.",
                     union_name ? union_name : "?");
        return false;
    }
    instruction->kind = MIR_INSTR_UNION_NEW;
    instruction->as.union_new.type_desc.name = ast_copy_text(union_name);
    instruction->as.union_new.type_desc.generic_param_count = decl->generic_param_count;
    instruction->as.union_new.type_desc.variant_count = decl->variant_count;
    if (!instruction->as.union_new.type_desc.name) {
        mr_set_error(build, source_span, NULL,
                     "Out of memory while lowering MIR union metadata.");
        return false;
    }
    if (decl->generic_param_count > 0) {
        generic_param_tags = calloc(decl->generic_param_count, sizeof(*generic_param_tags));
        instruction->as.union_new.type_desc.generic_param_tags = generic_param_tags;
        if (!generic_param_tags) {
            mr_set_error(build, source_span, NULL,
                         "Out of memory while lowering MIR union metadata.");
            return false;
        }
        for (v = 0; v < decl->generic_param_count; v++) {
            generic_param_tags[v] = CALYNDA_RT_TYPE_RAW_WORD;
        }
    }
    if (decl->variant_count == 0) {
        return true;
    }
    variant_names = calloc(decl->variant_count, sizeof(*variant_names));
    variant_payload_tags = calloc(decl->variant_count, sizeof(*variant_payload_tags));
    instruction->as.union_new.type_desc.variant_names =
        (const char *const *)variant_names;
    instruction->as.union_new.type_desc.variant_payload_tags = variant_payload_tags;
    if (!variant_names || !variant_payload_tags) {
        mr_set_error(build, source_span, NULL,
                     "Out of memory while lowering MIR union descriptor.");
        return false;
    }
    for (v = 0; v < decl->variant_count; v++) {
        variant_names[v] =
            ast_copy_text(decl->variants[v].name ? decl->variants[v].name : "");
        if (!variant_names[v]) {
            mr_set_error(build, source_span, NULL,
                         "Out of memory while lowering MIR union variant names.");
            return false;
        }
        variant_payload_tags[v] = decl->variants[v].has_payload
            ? mr_checked_type_to_runtime_tag(decl->variants[v].payload_type)
            : CALYNDA_RT_TYPE_VOID;
    }

    return true;
}

bool mr_lower_array_literal(MirUnitBuildContext *context,
                            const HirExpression *expression,
                            MirValue *value) {
    MirInstruction instruction;
    size_t i;
    bool is_hetero = (expression->type.kind == CHECKED_TYPE_NAMED &&
                      expression->type.name != NULL &&
                      strcmp(expression->type.name, "arr") == 0);

    memset(&instruction, 0, sizeof(instruction));

    if (is_hetero) {
        instruction.kind = MIR_INSTR_HETERO_ARRAY_NEW;
        instruction.as.hetero_array_new.dest_temp = context->unit->next_temp_index++;
        instruction.as.hetero_array_new.type_desc.name = ast_copy_text("arr");
        instruction.as.hetero_array_new.type_desc.generic_param_count = 1;
        instruction.as.hetero_array_new.type_desc.variant_count =
            expression->as.array_literal.element_count;
        if (!instruction.as.hetero_array_new.type_desc.name) {
            mr_set_error(context->build,
                         expression->source_span,
                         NULL,
                         "Out of memory while lowering MIR hetero array.");
            return false;
        }
        instruction.as.hetero_array_new.type_desc.generic_param_tags = calloc(
            1,
            sizeof(*instruction.as.hetero_array_new.type_desc.generic_param_tags));
        if (!instruction.as.hetero_array_new.type_desc.generic_param_tags) {
            mr_instruction_free(&instruction);
            mr_set_error(context->build, expression->source_span, NULL,
                         "Out of memory while lowering MIR hetero array.");
            return false;
        }
        ((CalyndaRtTypeTag *)instruction.as.hetero_array_new.type_desc.generic_param_tags)[0] =
            CALYNDA_RT_TYPE_RAW_WORD;
        if (expression->as.array_literal.element_count > 0) {
            instruction.as.hetero_array_new.elements = calloc(
                expression->as.array_literal.element_count,
                sizeof(*instruction.as.hetero_array_new.elements));
            instruction.as.hetero_array_new.type_desc.variant_payload_tags = calloc(
                expression->as.array_literal.element_count,
                sizeof(*instruction.as.hetero_array_new.type_desc.variant_payload_tags));
            if (!instruction.as.hetero_array_new.elements ||
                !instruction.as.hetero_array_new.type_desc.variant_payload_tags) {
                mr_instruction_free(&instruction);
                mr_set_error(context->build,
                              expression->source_span,
                              NULL,
                              "Out of memory while lowering MIR hetero array.");
                return false;
            }
        }
        instruction.as.hetero_array_new.element_count = expression->as.array_literal.element_count;
        for (i = 0; i < expression->as.array_literal.element_count; i++) {
            if (!mr_lower_expression(context,
                                  expression->as.array_literal.elements[i],
                                  &instruction.as.hetero_array_new.elements[i])) {
                mr_instruction_free(&instruction);
                return false;
            }
            ((CalyndaRtTypeTag *)instruction.as.hetero_array_new.type_desc.variant_payload_tags)[i] =
                mr_checked_type_to_runtime_tag(expression->as.array_literal.elements[i]->type);
        }
    } else {
        instruction.kind = MIR_INSTR_ARRAY_LITERAL;
        instruction.as.array_literal.dest_temp = context->unit->next_temp_index++;
        if (expression->as.array_literal.element_count > 0) {
            instruction.as.array_literal.elements = calloc(
                expression->as.array_literal.element_count,
                sizeof(*instruction.as.array_literal.elements));
            if (!instruction.as.array_literal.elements) {
                mr_set_error(context->build,
                              expression->source_span,
                              NULL,
                              "Out of memory while lowering MIR array literals.");
                return false;
            }
        }
        instruction.as.array_literal.element_count = expression->as.array_literal.element_count;
        for (i = 0; i < expression->as.array_literal.element_count; i++) {
            if (!mr_lower_expression(context,
                                  expression->as.array_literal.elements[i],
                                  &instruction.as.array_literal.elements[i])) {
                mr_instruction_free(&instruction);
                return false;
            }
        }
    }

    if (!mr_current_block(context) ||
        !mr_append_instruction(mr_current_block(context), instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR array literals.");
        return false;
    }

    value->kind = MIR_VALUE_TEMP;
    value->type = expression->type;
    value->as.temp_index = is_hetero
        ? instruction.as.hetero_array_new.dest_temp
        : instruction.as.array_literal.dest_temp;
    return true;
}
