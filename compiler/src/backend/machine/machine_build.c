#include "machine_internal.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const char *mc_named_symbol_global_name(const char *fallback_name,
                                               const Symbol *symbol) {
    if (symbol &&
        (symbol->kind == SYMBOL_KIND_TOP_LEVEL_BINDING ||
         symbol->kind == SYMBOL_KIND_ASM_BINDING) &&
        symbol->qualified_name) {
        return symbol->qualified_name;
    }

    return fallback_name;
}

static AstPrimitiveType mc_static_canonical_primitive(AstPrimitiveType primitive) {
    switch (primitive) {
    case AST_PRIMITIVE_BYTE:   return AST_PRIMITIVE_UINT8;
    case AST_PRIMITIVE_SBYTE:  return AST_PRIMITIVE_INT8;
    case AST_PRIMITIVE_SHORT:  return AST_PRIMITIVE_INT16;
    case AST_PRIMITIVE_INT:    return AST_PRIMITIVE_INT32;
    case AST_PRIMITIVE_UINT:   return AST_PRIMITIVE_UINT32;
    case AST_PRIMITIVE_LONG:   return AST_PRIMITIVE_INT64;
    case AST_PRIMITIVE_ULONG:  return AST_PRIMITIVE_UINT64;
    default:                   return primitive;
    }
}

static bool mc_static_primitive_is_integral(AstPrimitiveType primitive) {
    switch (mc_static_canonical_primitive(primitive)) {
    case AST_PRIMITIVE_INT8:
    case AST_PRIMITIVE_INT16:
    case AST_PRIMITIVE_INT32:
    case AST_PRIMITIVE_INT64:
    case AST_PRIMITIVE_UINT8:
    case AST_PRIMITIVE_UINT16:
    case AST_PRIMITIVE_UINT32:
    case AST_PRIMITIVE_UINT64:
        return true;
    default:
        return false;
    }
}

static bool mc_static_primitive_is_signed(AstPrimitiveType primitive) {
    switch (mc_static_canonical_primitive(primitive)) {
    case AST_PRIMITIVE_INT8:
    case AST_PRIMITIVE_INT16:
    case AST_PRIMITIVE_INT32:
    case AST_PRIMITIVE_INT64:
        return true;
    default:
        return false;
    }
}

static bool mc_parse_integer_literal_word(const char *text,
                                          bool is_signed,
                                          CalyndaRtWord *word_out) {
    char *end = NULL;

    if (!text || text[0] == '\0') {
        return false;
    }

    errno = 0;
    if (is_signed) {
        long long value = strtoll(text, &end, 0);

        if (errno != 0 || !end || *end != '\0') {
            return false;
        }
        if (word_out) {
            *word_out = (CalyndaRtWord)(int64_t)value;
        }
        return true;
    }

    {
        unsigned long long value = strtoull(text, &end, 0);

        if (errno != 0 || !end || *end != '\0') {
            return false;
        }
        if (word_out) {
            *word_out = (CalyndaRtWord)value;
        }
        return true;
    }
}

static bool mc_extract_static_scalar_word(const HirExpression *expression,
                                          CalyndaRtWord *word_out) {
    AstPrimitiveType primitive;

    if (!expression) {
        return false;
    }

    if (expression->kind == HIR_EXPR_UNARY) {
        if (expression->as.unary.operator == AST_UNARY_OP_PLUS) {
            return mc_extract_static_scalar_word(expression->as.unary.operand,
                                                 word_out);
        }
        if (expression->as.unary.operator == AST_UNARY_OP_NEGATE &&
            expression->type.kind == CHECKED_TYPE_VALUE &&
            expression->type.array_depth == 0 &&
            expression->as.unary.operand &&
            expression->as.unary.operand->kind == HIR_EXPR_LITERAL &&
            expression->as.unary.operand->as.literal.kind == AST_LITERAL_INTEGER &&
            mc_static_primitive_is_integral(expression->type.primitive) &&
            mc_static_primitive_is_signed(expression->type.primitive)) {
            CalyndaRtWord operand_word = 0;
            int64_t signed_word;

            if (!mc_parse_integer_literal_word(
                    expression->as.unary.operand->as.literal.as.text,
                    true,
                    &operand_word)) {
                return false;
            }

            signed_word = -(int64_t)operand_word;
            if (word_out) {
                *word_out = (CalyndaRtWord)signed_word;
            }
            return true;
        }
        return false;
    }

    if (expression->kind != HIR_EXPR_LITERAL ||
        expression->type.kind != CHECKED_TYPE_VALUE ||
        expression->type.array_depth != 0) {
        return false;
    }

    primitive = mc_static_canonical_primitive(expression->type.primitive);
    switch (expression->as.literal.kind) {
    case AST_LITERAL_BOOL:
        if (primitive != AST_PRIMITIVE_BOOL) {
            return false;
        }
        if (word_out) {
            *word_out = expression->as.literal.as.bool_value ? 1u : 0u;
        }
        return true;

    case AST_LITERAL_INTEGER:
        if (!mc_static_primitive_is_integral(primitive)) {
            return false;
        }
        return mc_parse_integer_literal_word(expression->as.literal.as.text,
                                             mc_static_primitive_is_signed(primitive),
                                             word_out);

    default:
        return false;
    }
}

static bool mc_is_static_array_literal_expression(const HirExpression *expression) {
    size_t i;

    if (!expression ||
        expression->kind != HIR_EXPR_ARRAY_LITERAL ||
        expression->type.kind != CHECKED_TYPE_VALUE ||
        expression->type.array_depth == 0) {
        return false;
    }

    for (i = 0; i < expression->as.array_literal.element_count; i++) {
        const HirExpression *element = expression->as.array_literal.elements[i];

        if (!element) {
            return false;
        }

        if (element->type.kind == CHECKED_TYPE_VALUE &&
            element->type.array_depth > 0) {
            if (!mc_is_static_array_literal_expression(element)) {
                return false;
            }
        } else if (!mc_extract_static_scalar_word(element, NULL)) {
            return false;
        }
    }

    return true;
}

static bool mc_is_static_top_level_array_binding(const HirTopLevelDecl *decl) {
    return decl != NULL &&
           decl->kind == HIR_TOP_LEVEL_BINDING &&
           decl->as.binding.is_final &&
           decl->as.binding.initializer != NULL &&
           mc_is_static_array_literal_expression(decl->as.binding.initializer);
}

static bool mc_append_static_array_object(MachineBuildContext *context,
                                          MachineStaticArrayObject object,
                                          size_t *object_index_out) {
    size_t object_index;

    if (!context || !context->program) {
        free(object.elements);
        return false;
    }

    if (!mc_reserve_items((void **)&context->program->static_array_objects,
                          &context->program->static_array_object_capacity,
                          context->program->static_array_object_count + 1,
                          sizeof(*context->program->static_array_objects))) {
        free(object.elements);
        mc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Out of memory while collecting static array objects.");
        return false;
    }

    object_index = context->program->static_array_object_count++;
    context->program->static_array_objects[object_index] = object;
    if (object_index_out) {
        *object_index_out = object_index;
    }
    return true;
}

static bool mc_append_static_array_binding(MachineBuildContext *context,
                                           const char *global_name,
                                           size_t object_index,
                                           AstSourceSpan source_span) {
    MachineStaticArrayBinding binding;

    if (!context || !context->program || !global_name) {
        return false;
    }

    if (!mc_reserve_items((void **)&context->program->static_array_bindings,
                          &context->program->static_array_binding_capacity,
                          context->program->static_array_binding_count + 1,
                          sizeof(*context->program->static_array_bindings))) {
        mc_set_error(context,
                     source_span,
                     NULL,
                     "Out of memory while collecting static array bindings.");
        return false;
    }

    memset(&binding, 0, sizeof(binding));
    binding.global_name = ast_copy_text(global_name);
    binding.object_index = object_index;
    if (!binding.global_name) {
        mc_set_error(context,
                     source_span,
                     NULL,
                     "Out of memory while naming static array binding '%s'.",
                     global_name);
        return false;
    }

    context->program->static_array_bindings[context->program->static_array_binding_count++] =
        binding;
    return true;
}

static bool mc_collect_static_array_object(MachineBuildContext *context,
                                           const HirExpression *expression,
                                           size_t *object_index_out) {
    MachineStaticArrayObject object;
    size_t i;

    if (!context || !expression || expression->kind != HIR_EXPR_ARRAY_LITERAL) {
        return false;
    }

    memset(&object, 0, sizeof(object));
    object.element_count = expression->as.array_literal.element_count;
    if (object.element_count > 0) {
        object.elements = calloc(object.element_count, sizeof(*object.elements));
        if (!object.elements) {
            mc_set_error(context,
                         expression->source_span,
                         NULL,
                         "Out of memory while collecting static array literal data.");
            return false;
        }
    }

    for (i = 0; i < object.element_count; i++) {
        const HirExpression *element = expression->as.array_literal.elements[i];

        if (element->type.kind == CHECKED_TYPE_VALUE && element->type.array_depth > 0) {
            object.elements[i].kind = MACHINE_STATIC_ARRAY_ELEMENT_OBJECT;
            if (!mc_collect_static_array_object(context,
                                                element,
                                                &object.elements[i].object_index)) {
                free(object.elements);
                return false;
            }
        } else {
            object.elements[i].kind = MACHINE_STATIC_ARRAY_ELEMENT_WORD;
            if (!mc_extract_static_scalar_word(element, &object.elements[i].word)) {
                free(object.elements);
                mc_set_error(context,
                             element->source_span,
                             NULL,
                             "Unsupported literal in static final array initializer.");
                return false;
            }
        }
    }

    return mc_append_static_array_object(context, object, object_index_out);
}

bool mc_collect_static_arrays(MachineBuildContext *context) {
    size_t i;

    if (!context || !context->hir_program) {
        return true;
    }

    for (i = 0; i < context->hir_program->top_level_count; i++) {
        const HirTopLevelDecl *decl = context->hir_program->top_level_decls[i];
        const char *global_name;
        size_t object_index;

        if (!mc_is_static_top_level_array_binding(decl)) {
            continue;
        }

        global_name = mc_named_symbol_global_name(decl->as.binding.name,
                                                  decl->as.binding.symbol);
        if (!global_name) {
            mc_set_error(context,
                         decl->as.binding.source_span,
                         NULL,
                         "Internal error: missing global name for static array binding.");
            return false;
        }

        if (!mc_collect_static_array_object(context,
                                            decl->as.binding.initializer,
                                            &object_index) ||
            !mc_append_static_array_binding(context,
                                            global_name,
                                            object_index,
                                            decl->as.binding.source_span)) {
            return false;
        }
    }

    return true;
}

bool mc_build_unit(MachineBuildContext *context,
                   const LirUnit *lir_unit,
                   const CodegenUnit *codegen_unit,
                   MachineUnit *machine_unit) {
    size_t block_index;

    if (!lir_unit || !codegen_unit || !machine_unit) {
        return false;
    }
    if (lir_unit->block_count != codegen_unit->block_count) {
        mc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Machine emission requires matching LIR/codegen block counts for unit %s.",
                     lir_unit->name);
        return false;
    }

    memset(machine_unit, 0, sizeof(*machine_unit));
    machine_unit->kind = lir_unit->kind;
    machine_unit->name = ast_copy_text(lir_unit->name);
    machine_unit->return_type = lir_unit->return_type;
    machine_unit->is_exported = lir_unit->symbol && lir_unit->symbol->is_exported;
    machine_unit->is_static = lir_unit->symbol && lir_unit->symbol->is_static;
    machine_unit->parameter_count = lir_unit->parameter_count;
    machine_unit->frame_slot_count = codegen_unit->frame_slot_count;
    machine_unit->spill_slot_count = codegen_unit->spill_slot_count;
    machine_unit->is_boot = lir_unit->is_boot;
    machine_unit->helper_slot_count = mc_compute_helper_slot_count(lir_unit, codegen_unit);
    machine_unit->outgoing_stack_slot_count = mc_compute_outgoing_stack_slot_count(lir_unit, codegen_unit, context->program->target_desc);
    machine_unit->block_count = lir_unit->block_count;
    if (!machine_unit->name) {
        mc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Out of memory while naming machine unit.");
        return false;
    }

    if (lir_unit->kind == LIR_UNIT_ASM) {
        machine_unit->asm_body = ast_copy_text_n(codegen_unit->asm_body,
                                                 codegen_unit->asm_body_length);
        machine_unit->asm_body_length = codegen_unit->asm_body_length;
        if (codegen_unit->asm_body && !machine_unit->asm_body) {
            mc_unit_free(machine_unit);
            mc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while building machine asm body.");
            return false;
        }
        return true;
    }

    if (machine_unit->frame_slot_count > 0) {
        size_t slot_index;

        machine_unit->frame_slots = calloc(machine_unit->frame_slot_count,
                                           sizeof(*machine_unit->frame_slots));
        if (!machine_unit->frame_slots) {
            mc_unit_free(machine_unit);
            mc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while allocating machine frame slots.");
            return false;
        }
        for (slot_index = 0; slot_index < machine_unit->frame_slot_count; slot_index++) {
            machine_unit->frame_slots[slot_index].index = slot_index;
            machine_unit->frame_slots[slot_index].kind = codegen_unit->frame_slots[slot_index].kind;
            machine_unit->frame_slots[slot_index].name = ast_copy_text(codegen_unit->frame_slots[slot_index].name);
            machine_unit->frame_slots[slot_index].type = codegen_unit->frame_slots[slot_index].type;
            machine_unit->frame_slots[slot_index].is_final = codegen_unit->frame_slots[slot_index].is_final;
            if (!machine_unit->frame_slots[slot_index].name) {
                mc_unit_free(machine_unit);
                mc_set_error(context,
                             (AstSourceSpan){0},
                             NULL,
                             "Out of memory while naming machine frame slots.");
                return false;
            }
        }
    }
    if (machine_unit->block_count > 0) {
        machine_unit->blocks = calloc(machine_unit->block_count, sizeof(*machine_unit->blocks));
        if (!machine_unit->blocks) {
            mc_unit_free(machine_unit);
            mc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while allocating machine blocks.");
            return false;
        }
    }

    for (block_index = 0; block_index < machine_unit->block_count; block_index++) {
        MachineBlock *block = &machine_unit->blocks[block_index];
        const LirBasicBlock *lir_block = &lir_unit->blocks[block_index];
        const CodegenBlock *codegen_block = &codegen_unit->blocks[block_index];
        size_t instruction_index;

        if (lir_block->instruction_count != codegen_block->instruction_count) {
            mc_unit_free(machine_unit);
            mc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Machine emission requires matching LIR/codegen instruction counts for block %s.",
                         lir_block->label);
            return false;
        }

        block->label = ast_copy_text(lir_block->label);
        if (!block->label) {
            mc_unit_free(machine_unit);
            mc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while naming machine blocks.");
            return false;
        }

        if (block_index == 0 && !mc_emit_entry_prologue(context, codegen_unit, machine_unit, block)) {
            mc_unit_free(machine_unit);
            return false;
        }

        for (instruction_index = 0;
             instruction_index < lir_block->instruction_count;
             instruction_index++) {
            if (!mc_emit_instruction(context,
                                     lir_unit,
                                     codegen_unit,
                                     lir_block,
                                     codegen_block,
                                     instruction_index,
                                     block)) {
                mc_unit_free(machine_unit);
                return false;
            }
        }
        if (!mc_emit_terminator(context,
                                lir_unit,
                                codegen_unit,
                                lir_block,
                                machine_unit,
                                block)) {
            mc_unit_free(machine_unit);
            return false;
        }
    }

    return true;
}
