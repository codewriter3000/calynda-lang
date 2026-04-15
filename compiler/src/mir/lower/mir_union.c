#include "mir_internal.h"

bool mr_is_union_variant_call(const HirExpression *expression,
                              const char **out_union_name,
                              const char **out_variant_name) {
    const HirExpression *callee;
    const HirExpression *target;

    if (!expression || expression->kind != HIR_EXPR_CALL) {
        return false;
    }

    callee = expression->as.call.callee;
    if (!callee || callee->kind != HIR_EXPR_MEMBER) {
        return false;
    }

    target = callee->as.member.target;
    if (!target || target->kind != HIR_EXPR_SYMBOL) {
        return false;
    }

    if (target->as.symbol.kind != SYMBOL_KIND_UNION) {
        return false;
    }

    if (out_union_name) {
        *out_union_name = target->as.symbol.name;
    }
    if (out_variant_name) {
        *out_variant_name = callee->as.member.member;
    }
    return true;
}

bool mr_is_union_variant_member(const HirExpression *expression,
                                const char **out_union_name,
                                const char **out_variant_name) {
    const HirExpression *target;

    if (!expression || expression->kind != HIR_EXPR_MEMBER) {
        return false;
    }

    target = expression->as.member.target;
    if (!target || target->kind != HIR_EXPR_SYMBOL) {
        return false;
    }

    if (target->as.symbol.kind != SYMBOL_KIND_UNION) {
        return false;
    }

    if (out_union_name) {
        *out_union_name = target->as.symbol.name;
    }
    if (out_variant_name) {
        *out_variant_name = expression->as.member.member;
    }
    return true;
}

bool mr_find_hir_union_variant(const MirBuildContext *build,
                               const char *union_name,
                               const char *variant_name,
                               size_t *out_variant_index,
                               size_t *out_variant_count) {
    size_t d, v;
    const HirProgram *hir = build->hir_program;

    for (d = 0; d < hir->top_level_count; d++) {
        const HirTopLevelDecl *decl = hir->top_level_decls[d];
        if (decl->kind != HIR_TOP_LEVEL_UNION) {
            continue;
        }
        if (!decl->as.union_decl.name || strcmp(decl->as.union_decl.name, union_name) != 0) {
            continue;
        }
        if (out_variant_count) {
            *out_variant_count = decl->as.union_decl.variant_count;
        }
        for (v = 0; v < decl->as.union_decl.variant_count; v++) {
            if (decl->as.union_decl.variants[v].name &&
                strcmp(decl->as.union_decl.variants[v].name, variant_name) == 0) {
                if (out_variant_index) {
                    *out_variant_index = v;
                }
                return true;
            }
        }
        return false;
    }
    return false;
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
        if (expression->as.array_literal.element_count > 0) {
            instruction.as.hetero_array_new.elements = calloc(
                expression->as.array_literal.element_count,
                sizeof(*instruction.as.hetero_array_new.elements));
            instruction.as.hetero_array_new.element_types = calloc(
                expression->as.array_literal.element_count,
                sizeof(*instruction.as.hetero_array_new.element_types));
            if (!instruction.as.hetero_array_new.elements ||
                !instruction.as.hetero_array_new.element_types) {
                free(instruction.as.hetero_array_new.elements);
                free(instruction.as.hetero_array_new.element_types);
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
            instruction.as.hetero_array_new.element_types[i] =
                expression->as.array_literal.elements[i]->type;
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
