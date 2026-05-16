#include "mir_internal.h"

#include <stdlib.h>
#include <string.h>

static bool mr_specialize_union_generic_param(MirBuildContext *build,
                                              const char *union_name,
                                              size_t variant_index,
                                              CheckedType payload_type,
                                              CalyndaRtTypeDescriptor *type_desc,
                                              AstSourceSpan source_span) {
    size_t d;

    if (!build || !build->hir_program || !type_desc || !type_desc->generic_param_tags) {
        return true;
    }
    for (d = 0; d < build->hir_program->top_level_count; d++) {
        const HirTopLevelDecl *decl = build->hir_program->top_level_decls[d];
        const CheckedType *variant_type;
        size_t g;

        if (decl->kind != HIR_TOP_LEVEL_UNION || !decl->as.union_decl.name ||
            strcmp(decl->as.union_decl.name, union_name) != 0) {
            continue;
        }
        if (variant_index >= decl->as.union_decl.variant_count) {
            break;
        }
        variant_type = &decl->as.union_decl.variants[variant_index].payload_type;
        if ((variant_type->kind != CHECKED_TYPE_TYPE_PARAM &&
             variant_type->kind != CHECKED_TYPE_NAMED) ||
            !variant_type->name) {
            if (type_desc->generic_param_count == 1) {
                ((CalyndaRtTypeTag *)type_desc->generic_param_tags)[0] =
                    mr_checked_type_to_runtime_tag(payload_type);
            }
            return true;
        }
        for (g = 0; g < decl->as.union_decl.generic_param_count; g++) {
            if (decl->as.union_decl.generic_params[g] &&
                strcmp(decl->as.union_decl.generic_params[g], variant_type->name) == 0) {
                ((CalyndaRtTypeTag *)type_desc->generic_param_tags)[g] =
                    mr_checked_type_to_runtime_tag(payload_type);
                return true;
            }
        }
        if (type_desc->generic_param_count == 1) {
            ((CalyndaRtTypeTag *)type_desc->generic_param_tags)[0] =
                mr_checked_type_to_runtime_tag(payload_type);
            return true;
        }
        break;
    }

    mr_set_error(build, source_span, NULL,
                 "Internal error: union generic payload metadata '%s' could not be specialized.",
                 union_name ? union_name : "?");
    return false;
}

bool mr_lower_call_expression(MirUnitBuildContext *context,
                              const HirExpression *expression,
                              MirValue *value) {
    MirInstruction instruction;
    size_t i;

    /* Payload union variant constructor: Option.Some(42) → union_new */
    {
        const char *call_union_name = NULL;
        const char *call_variant_name = NULL;

        if (mr_is_union_variant_call(expression, &call_union_name, &call_variant_name)) {
            size_t call_variant_index = 0;
            size_t call_variant_count = 0;

            if (!mr_find_hir_union_variant(context->build, call_union_name, call_variant_name,
                                        &call_variant_index, &call_variant_count)) {
                mr_set_error(context->build,
                              expression->source_span,
                              NULL,
                              "Internal error: union variant '%s.%s' not found in HIR.",
                              call_union_name, call_variant_name);
                return false;
            }

            memset(&instruction, 0, sizeof(instruction));
            instruction.as.union_new.dest_temp = context->unit->next_temp_index++;
            instruction.as.union_new.variant_index = call_variant_index;
            instruction.as.union_new.has_payload = (expression->as.call.argument_count > 0);

            if (!mr_init_union_new_instruction(context->build,
                                               &instruction,
                                               call_union_name,
                                               expression->source_span)) {
                mr_instruction_free(&instruction);
                return false;
            }

            if (instruction.as.union_new.has_payload) {
                if (!mr_lower_expression(context,
                                      expression->as.call.arguments[0],
                                      &instruction.as.union_new.payload)) {
                    mr_instruction_free(&instruction);
                    return false;
                }
                if (instruction.as.union_new.variant_index <
                        instruction.as.union_new.type_desc.variant_count &&
                    instruction.as.union_new.type_desc.variant_payload_tags) {
                    ((CalyndaRtTypeTag *)instruction.as.union_new.type_desc.variant_payload_tags)
                        [instruction.as.union_new.variant_index] =
                            mr_checked_type_to_runtime_tag(
                                instruction.as.union_new.payload.type);
                    if (!mr_specialize_union_generic_param(context->build,
                                                           call_union_name,
                                                           instruction.as.union_new.variant_index,
                                                           instruction.as.union_new.payload.type,
                                                           &instruction.as.union_new.type_desc,
                                                           expression->source_span)) {
                        mr_instruction_free(&instruction);
                        return false;
                    }
                }
            } else {
                memset(&instruction.as.union_new.payload, 0, sizeof(MirValue));
            }

            if (!mr_current_block(context) ||
                !mr_append_instruction(mr_current_block(context), instruction)) {
                mr_instruction_free(&instruction);
                mr_set_error(context->build,
                              expression->source_span,
                              NULL,
                              "Out of memory while lowering MIR union variant call.");
                return false;
            }

            value->kind = MIR_VALUE_TEMP;
            value->type = expression->type;
            value->as.temp_index = instruction.as.union_new.dest_temp;
            return true;
        }
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_CALL;
    {
        /* Detect NLR arguments (|var closures or return-as-arg) */
        bool has_nlr = false;
        size_t n_args = expression->as.call.argument_count;
        size_t *nlr_retval_locals = NULL;
        size_t nlr_slot_local = 0;

        for (i = 0; i < n_args && !has_nlr; i++) {
            const HirExpression *a = expression->as.call.arguments[i];
            if (a->kind == HIR_EXPR_NONLOCAL_RETURN ||
                (a->kind == HIR_EXPR_LAMBDA && a->as.lambda.is_nlr_block)) {
                has_nlr = true;
            }
        }

        if (has_nlr) {
            MirInstruction push_call;
            MirValue slot_ptr_temp;
            CheckedType ext_type = mr_nlr_external_type();

            /* Allocate per-arg retval local index array (sentinel = (size_t)-1) */
            nlr_retval_locals = calloc(n_args, sizeof(*nlr_retval_locals));
            if (!nlr_retval_locals) {
                mr_set_error(context->build, expression->source_span, NULL,
                             "Out of memory while lowering NLR call.");
                return false;
            }
            memset(nlr_retval_locals, 0xFF, n_args * sizeof(*nlr_retval_locals));

            /* Pre-evaluate NONLOCAL_RETURN values into synthetic locals */
            for (i = 0; i < n_args; i++) {
                const HirExpression *a = expression->as.call.arguments[i];
                if (a->kind == HIR_EXPR_NONLOCAL_RETURN && a->as.nonlocal_return_value) {
                    MirValue retval;
                    size_t retval_local;
                    if (!mr_lower_expression(context, a->as.nonlocal_return_value, &retval) ||
                        !mr_append_synthetic_local(context, "nlrv", retval.type,
                                                   a->source_span, &retval_local) ||
                        !mr_append_store_local_instruction(context, retval_local,
                                                           retval, a->source_span)) {
                        free(nlr_retval_locals);
                        return false;
                    }
                    nlr_retval_locals[i] = retval_local;
                }
            }

            /* Emit: temp_slot_ptr = call __calynda_rt_nlr_push() */
            memset(&push_call, 0, sizeof(push_call));
            push_call.kind = MIR_INSTR_CALL;
            if (!mr_value_from_global(context->build, "__calynda_rt_nlr_push",
                                      ext_type, &push_call.as.call.callee)) {
                free(nlr_retval_locals);
                return false;
            }
            push_call.as.call.has_result = true;
            push_call.as.call.dest_temp = context->unit->next_temp_index++;
            if (!mr_current_block(context) ||
                !mr_append_instruction(mr_current_block(context), push_call)) {
                mr_instruction_free(&push_call);
                free(nlr_retval_locals);
                mr_set_error(context->build, expression->source_span, NULL,
                             "Out of memory while lowering NLR push call.");
                return false;
            }
            memset(&slot_ptr_temp, 0, sizeof(slot_ptr_temp));
            slot_ptr_temp.kind = MIR_VALUE_TEMP;
            slot_ptr_temp.type = ext_type;
            slot_ptr_temp.as.temp_index = push_call.as.call.dest_temp;
            if (!mr_append_synthetic_local(context, "nlrs", ext_type,
                                           expression->source_span, &nlr_slot_local) ||
                !mr_append_store_local_instruction(context, nlr_slot_local,
                                                   slot_ptr_temp, expression->source_span)) {
                free(nlr_retval_locals);
                return false;
            }
        }

        if (!mr_lower_expression(context, expression->as.call.callee, &instruction.as.call.callee)) {
            free(nlr_retval_locals);
            return false;
        }
        instruction.as.call.has_result = (expression->type.kind != CHECKED_TYPE_VOID);
        if (instruction.as.call.has_result) {
            instruction.as.call.dest_temp = context->unit->next_temp_index++;
        }
        if (n_args > 0) {
            instruction.as.call.arguments = calloc(n_args,
                                                    sizeof(*instruction.as.call.arguments));
            if (!instruction.as.call.arguments) {
                free(nlr_retval_locals);
                mr_instruction_free(&instruction);
                mr_set_error(context->build, expression->source_span, NULL,
                             "Out of memory while lowering MIR calls.");
                return false;
            }
        }
        instruction.as.call.argument_count = n_args;

#include "mir_expr_call_p2.inc"
