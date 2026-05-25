#include "mir_internal.h"

static AstPrimitiveType mr_static_array_canonical_primitive(AstPrimitiveType primitive) {
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

static bool mr_static_array_primitive_is_integral(AstPrimitiveType primitive) {
    switch (mr_static_array_canonical_primitive(primitive)) {
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

static bool mr_static_array_primitive_is_signed(AstPrimitiveType primitive) {
    switch (mr_static_array_canonical_primitive(primitive)) {
    case AST_PRIMITIVE_INT8:
    case AST_PRIMITIVE_INT16:
    case AST_PRIMITIVE_INT32:
    case AST_PRIMITIVE_INT64:
        return true;
    default:
        return false;
    }
}

static bool mr_is_static_scalar_expression(const HirExpression *expression) {
    AstPrimitiveType primitive;

    if (!expression) {
        return false;
    }

    if (expression->kind == HIR_EXPR_UNARY) {
        if (expression->as.unary.operator == AST_UNARY_OP_PLUS) {
            return mr_is_static_scalar_expression(expression->as.unary.operand);
        }
        if (expression->as.unary.operator == AST_UNARY_OP_NEGATE) {
            return expression->type.kind == CHECKED_TYPE_VALUE &&
                   expression->type.array_depth == 0 &&
                   expression->as.unary.operand != NULL &&
                   expression->as.unary.operand->kind == HIR_EXPR_LITERAL &&
                   expression->as.unary.operand->as.literal.kind == AST_LITERAL_INTEGER &&
                   mr_static_array_primitive_is_integral(expression->type.primitive) &&
                   mr_static_array_primitive_is_signed(expression->type.primitive);
        }
        return false;
    }

    if (expression->kind != HIR_EXPR_LITERAL ||
        expression->type.kind != CHECKED_TYPE_VALUE ||
        expression->type.array_depth != 0) {
        return false;
    }

    primitive = mr_static_array_canonical_primitive(expression->type.primitive);
    switch (expression->as.literal.kind) {
    case AST_LITERAL_BOOL:
        return primitive == AST_PRIMITIVE_BOOL;
    case AST_LITERAL_INTEGER:
        return mr_static_array_primitive_is_integral(primitive);
    default:
        return false;
    }
}

static bool mr_is_static_array_literal_expression(const HirExpression *expression) {
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
            if (!mr_is_static_array_literal_expression(element)) {
                return false;
            }
        } else if (!mr_is_static_scalar_expression(element)) {
            return false;
        }
    }

    return true;
}

static bool mr_top_level_binding_uses_static_array_data(const HirTopLevelDecl *decl) {
    return decl != NULL &&
           decl->kind == HIR_TOP_LEVEL_BINDING &&
           decl->as.binding.is_final &&
           decl->as.binding.initializer != NULL &&
           mr_is_static_array_literal_expression(decl->as.binding.initializer);
}

typedef struct MirModuleInitSegment {
    size_t      start_block_index;
    size_t      start_instruction_index;
    size_t      end_block_index;
    size_t      end_instruction_index;
    const char *global_name;
    bool        is_pure;
} MirModuleInitSegment;

static bool mr_is_pure_module_init_instruction(const MirInstruction *instruction) {
    if (!instruction) {
        return false;
    }

    switch (instruction->kind) {
    case MIR_INSTR_BINARY:
    case MIR_INSTR_UNARY:
    case MIR_INSTR_CLOSURE:
    case MIR_INSTR_CAST:
    case MIR_INSTR_MEMBER:
    case MIR_INSTR_INDEX_LOAD:
    case MIR_INSTR_ARRAY_LITERAL:
    case MIR_INSTR_TEMPLATE:
    case MIR_INSTR_STORE_LOCAL:
    case MIR_INSTR_UNION_NEW:
    case MIR_INSTR_UNION_GET_TAG:
    case MIR_INSTR_UNION_GET_PAYLOAD:
    case MIR_INSTR_HETERO_ARRAY_NEW:
        return true;
    case MIR_INSTR_CALL:
    case MIR_INSTR_STORE_GLOBAL:
    case MIR_INSTR_STORE_INDEX:
    case MIR_INSTR_STORE_MEMBER:
        return false;
    }

    return false;
}

static bool mr_is_pure_module_init_terminator(const MirTerminator *terminator) {
    if (!terminator) {
        return false;
    }

    switch (terminator->kind) {
    case MIR_TERM_NONE:
    case MIR_TERM_GOTO:
    case MIR_TERM_BRANCH:
        return true;
    case MIR_TERM_RETURN:
    case MIR_TERM_THROW:
        return false;
    }

    return false;
}

static bool mr_is_pure_module_init_segment(const MirUnit *unit,
                                           const MirModuleInitSegment *segment) {
    size_t block_index;

    if (!unit || !segment || segment->end_block_index >= unit->block_count) {
        return false;
    }

    for (block_index = segment->start_block_index;
         block_index <= segment->end_block_index;
         block_index++) {
        const MirBasicBlock *block = &unit->blocks[block_index];
        size_t instruction_start = 0;
        size_t instruction_end = block->instruction_count;
        size_t instruction_index;

        if (block_index == segment->start_block_index) {
            instruction_start = segment->start_instruction_index;
        }
        if (block_index == segment->end_block_index) {
            instruction_end = segment->end_instruction_index;
        }

        if (instruction_start > instruction_end || instruction_end > block->instruction_count) {
            return false;
        }

        for (instruction_index = instruction_start;
             instruction_index < instruction_end;
             instruction_index++) {
            if (!mr_is_pure_module_init_instruction(&block->instructions[instruction_index])) {
                return false;
            }
        }

        if (block_index < segment->end_block_index &&
            !mr_is_pure_module_init_terminator(&block->terminator)) {
            return false;
        }
    }

    return true;
}

bool mr_lower_module_init_unit(MirBuildContext *context,
                               bool *created_module_init_unit,
                               MirModuleInitSegment **segments_out,
                               size_t *segment_count_out) {
    MirUnit unit;
    MirUnitBuildContext unit_context;
    MirBasicBlock *block;
    MirModuleInitSegment *segments = NULL;
    size_t segment_count = 0;
    size_t segment_capacity = 0;
    size_t i;
    bool has_initializers = false;
    bool ok = false;

    if (!context) {
        return false;
    }

    if (created_module_init_unit) {
        *created_module_init_unit = false;
    }
    if (segments_out) {
        *segments_out = NULL;
    }
    if (segment_count_out) {
        *segment_count_out = 0;
    }

    for (i = 0; i < context->hir_program->top_level_count; i++) {
        const HirTopLevelDecl *decl = context->hir_program->top_level_decls[i];

        if (decl->kind == HIR_TOP_LEVEL_BINDING &&
            !mr_top_level_binding_uses_lambda_unit(decl) &&
            !mr_top_level_binding_uses_static_array_data(decl)) {
            has_initializers = true;
            break;
        }
    }

    if (!has_initializers) {
        return true;
    }

    memset(&unit, 0, sizeof(unit));
    memset(&unit_context, 0, sizeof(unit_context));
    unit.kind = MIR_UNIT_INIT;
    unit.name = ast_copy_text(MIR_MODULE_INIT_NAME);
    unit.return_type = mr_checked_type_void_value();
    if (!unit.name) {
        mr_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering MIR module init unit.");
        return false;
    }

    unit_context.build = context;
    unit_context.unit = &unit;
    unit_context.in_checked_manual = context->global_bounds_check;
    if (!mr_create_block(&unit_context, &unit_context.current_block_index)) {
        goto cleanup;
    }

    for (i = 0; i < context->hir_program->top_level_count; i++) {
        const HirTopLevelDecl *decl = context->hir_program->top_level_decls[i];
        MirValue value;
        MirBasicBlock *start_block;
        MirBasicBlock *end_block;
        const char *global_name;
        size_t start_block_index;
        size_t start_instruction_index;

        if (decl->kind != HIR_TOP_LEVEL_BINDING ||
            mr_top_level_binding_uses_lambda_unit(decl) ||
            mr_top_level_binding_uses_static_array_data(decl)) {
            continue;
        }

        start_block = mr_current_block(&unit_context);
        if (!start_block) {
            mr_set_error(context,
                         decl->as.binding.source_span,
                         NULL,
                         "Internal error: missing MIR block at module init binding start.");
            goto cleanup;
        }
        start_block_index = unit_context.current_block_index;
        start_instruction_index = start_block->instruction_count;
        global_name = mr_named_symbol_global_name(decl->as.binding.name,
                                                  decl->as.binding.symbol);

        if (!mr_lower_expression(&unit_context, decl->as.binding.initializer, &value) ||
            !mr_append_store_global_instruction(&unit_context,
                                             global_name,
                                             value,
                                             decl->as.binding.source_span)) {
            goto cleanup;
        }

        end_block = mr_current_block(&unit_context);
        if (!end_block ||
            end_block->instruction_count == 0 ||
            end_block->instructions[end_block->instruction_count - 1].kind != MIR_INSTR_STORE_GLOBAL) {
            mr_set_error(context,
                         decl->as.binding.source_span,
                         NULL,
                         "Internal error: malformed MIR module init segment for '%s'.",
                         decl->as.binding.name);
            goto cleanup;
        }

        if (!mr_reserve_items((void **)&segments,
                              &segment_capacity,
                              segment_count + 1,
                              sizeof(*segments))) {
            mr_set_error(context,
                         decl->as.binding.source_span,
                         NULL,
                         "Out of memory while recording MIR module init segments.");
            goto cleanup;
        }

        segments[segment_count].start_block_index = start_block_index;
        segments[segment_count].start_instruction_index = start_instruction_index;
        segments[segment_count].end_block_index = unit_context.current_block_index;
        segments[segment_count].end_instruction_index = end_block->instruction_count - 1;
        segments[segment_count].global_name = global_name;
        segments[segment_count].is_pure =
            mr_is_pure_module_init_segment(&unit, &segments[segment_count]);
        segment_count++;
    }

    block = mr_current_block(&unit_context);
    if (!block) {
        mr_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Internal error: missing MIR block at module init exit.");
        goto cleanup;
    }
    if (block->terminator.kind == MIR_TERM_NONE) {
        block->terminator.kind = MIR_TERM_RETURN;
        block->terminator.as.return_term.has_value = false;
    }

    if (!mr_append_unit(context->program, unit)) {
        mr_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while assembling MIR module init unit.");
        goto cleanup;
    }

    ok = true;

    if (created_module_init_unit) {
        *created_module_init_unit = true;
    }
    if (segments_out) {
        *segments_out = segments;
    }
    if (segment_count_out) {
        *segment_count_out = segment_count;
    }

cleanup:
    mr_free_manual_cleanup_state(&unit_context);
    if (!ok) {
        free(segments);
        mr_unit_free(&unit);
    }
    return ok;
}

static size_t mr_find_program_unit_index(const MirProgram *program,
                                         const char *unit_name) {
    size_t i;

    if (!program || !unit_name) {
        return SIZE_MAX;
    }

    for (i = 0; i < program->unit_count; i++) {
        if (program->units[i].name && strcmp(program->units[i].name, unit_name) == 0) {
            return i;
        }
    }

    return SIZE_MAX;
}

static bool mr_is_module_init_name(const char *unit_name) {
    return unit_name != NULL && strcmp(unit_name, MIR_MODULE_INIT_NAME) == 0;
}

static bool mr_is_module_init_call(const MirInstruction *instruction) {
    return instruction &&
           instruction->kind == MIR_INSTR_CALL &&
           instruction->as.call.callee.kind == MIR_VALUE_GLOBAL &&
           mr_is_module_init_name(instruction->as.call.callee.as.global_name);
}

static bool mr_is_unit_name_char(char ch) {
    return (ch >= '0' && ch <= '9') ||
           (ch >= 'A' && ch <= 'Z') ||
           (ch >= 'a' && ch <= 'z') ||
           ch == '_' ||
           ch == '$';
}

static void mr_mark_unit_name_reachable(const MirProgram *program,
                                        const char *unit_name,
                                        bool *reachable,
                                        bool *changed,
                                        bool ignore_module_init) {
    size_t unit_index;

    if (!program || !unit_name || !reachable || !changed) {
        return;
    }

    if (ignore_module_init && mr_is_module_init_name(unit_name)) {
        return;
    }

    unit_index = mr_find_program_unit_index(program, unit_name);
    if (unit_index == SIZE_MAX || reachable[unit_index]) {
        return;
    }

    reachable[unit_index] = true;
    *changed = true;
}

static void mr_mark_value_reachable_units(const MirProgram *program,
                                          const MirValue *value,
                                          bool *reachable,
                                          bool *changed,
                                          bool ignore_module_init) {
    if (!value || value->kind != MIR_VALUE_GLOBAL) {
        return;
    }

    mr_mark_unit_name_reachable(program,
                                value->as.global_name,
                                reachable,
                                changed,
                                ignore_module_init);
}

static void mr_mark_template_part_reachable_units(const MirProgram *program,
                                                  const MirTemplatePart *part,
                                                  bool *reachable,
                                                  bool *changed,
                                                  bool ignore_module_init) {
    if (!part || part->kind != MIR_TEMPLATE_PART_VALUE) {
        return;
    }

    mr_mark_value_reachable_units(program,
                                  &part->as.value,
                                  reachable,
                                  changed,
                                  ignore_module_init);
}

static void mr_mark_instruction_reachable_units(const MirProgram *program,
                                                const MirInstruction *instruction,
                                                bool *reachable,
                                                bool *changed,
                                                bool ignore_module_init) {
    size_t i;

    if (!program || !instruction) {
        return;
    }

    switch (instruction->kind) {
    case MIR_INSTR_BINARY:
        mr_mark_value_reachable_units(program,
                                      &instruction->as.binary.left,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        mr_mark_value_reachable_units(program,
                                      &instruction->as.binary.right,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        break;
    case MIR_INSTR_UNARY:
        mr_mark_value_reachable_units(program,
                                      &instruction->as.unary.operand,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        break;
    case MIR_INSTR_CLOSURE:
        mr_mark_unit_name_reachable(program,
                                    instruction->as.closure.unit_name,
                                    reachable,
                                    changed,
                                    ignore_module_init);
        for (i = 0; i < instruction->as.closure.capture_count; i++) {
            mr_mark_value_reachable_units(program,
                                          &instruction->as.closure.captures[i],
                                          reachable,
                                          changed,
                                          ignore_module_init);
        }
        break;
    case MIR_INSTR_CALL:
        mr_mark_value_reachable_units(program,
                                      &instruction->as.call.callee,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        for (i = 0; i < instruction->as.call.argument_count; i++) {
            mr_mark_value_reachable_units(program,
                                          &instruction->as.call.arguments[i],
                                          reachable,
                                          changed,
                                          ignore_module_init);
        }
        break;
    case MIR_INSTR_CAST:
        mr_mark_value_reachable_units(program,
                                      &instruction->as.cast.operand,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        break;
    case MIR_INSTR_MEMBER:
        mr_mark_value_reachable_units(program,
                                      &instruction->as.member.target,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        break;
    case MIR_INSTR_INDEX_LOAD:
        mr_mark_value_reachable_units(program,
                                      &instruction->as.index_load.target,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        mr_mark_value_reachable_units(program,
                                      &instruction->as.index_load.index,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        break;
    case MIR_INSTR_ARRAY_LITERAL:
        for (i = 0; i < instruction->as.array_literal.element_count; i++) {
            mr_mark_value_reachable_units(program,
                                          &instruction->as.array_literal.elements[i],
                                          reachable,
                                          changed,
                                          ignore_module_init);
        }
        break;
    case MIR_INSTR_TEMPLATE:
        for (i = 0; i < instruction->as.template_literal.part_count; i++) {
            mr_mark_template_part_reachable_units(program,
                                                  &instruction->as.template_literal.parts[i],
                                                  reachable,
                                                  changed,
                                                  ignore_module_init);
        }
        break;
    case MIR_INSTR_STORE_LOCAL:
        mr_mark_value_reachable_units(program,
                                      &instruction->as.store_local.value,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        break;
    case MIR_INSTR_STORE_GLOBAL:
        mr_mark_value_reachable_units(program,
                                      &instruction->as.store_global.value,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        break;
    case MIR_INSTR_STORE_INDEX:
        mr_mark_value_reachable_units(program,
                                      &instruction->as.store_index.target,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        mr_mark_value_reachable_units(program,
                                      &instruction->as.store_index.index,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        mr_mark_value_reachable_units(program,
                                      &instruction->as.store_index.value,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        break;
    case MIR_INSTR_STORE_MEMBER:
        mr_mark_value_reachable_units(program,
                                      &instruction->as.store_member.target,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        mr_mark_value_reachable_units(program,
                                      &instruction->as.store_member.value,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        break;
    case MIR_INSTR_UNION_NEW:
        if (instruction->as.union_new.has_payload) {
            mr_mark_value_reachable_units(program,
                                          &instruction->as.union_new.payload,
                                          reachable,
                                          changed,
                                          ignore_module_init);
        }
        break;
    case MIR_INSTR_UNION_GET_TAG:
        mr_mark_value_reachable_units(program,
                                      &instruction->as.union_get_tag.target,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        break;
    case MIR_INSTR_UNION_GET_PAYLOAD:
        mr_mark_value_reachable_units(program,
                                      &instruction->as.union_get_payload.target,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        break;
    case MIR_INSTR_HETERO_ARRAY_NEW:
        for (i = 0; i < instruction->as.hetero_array_new.element_count; i++) {
            mr_mark_value_reachable_units(program,
                                          &instruction->as.hetero_array_new.elements[i],
                                          reachable,
                                          changed,
                                          ignore_module_init);
        }
        break;
    }
}

static void mr_mark_terminator_reachable_units(const MirProgram *program,
                                               const MirTerminator *terminator,
                                               bool *reachable,
                                               bool *changed,
                                               bool ignore_module_init) {
    if (!program || !terminator) {
        return;
    }

    switch (terminator->kind) {
    case MIR_TERM_RETURN:
        if (terminator->as.return_term.has_value) {
            mr_mark_value_reachable_units(program,
                                          &terminator->as.return_term.value,
                                          reachable,
                                          changed,
                                          ignore_module_init);
        }
        break;
    case MIR_TERM_BRANCH:
        mr_mark_value_reachable_units(program,
                                      &terminator->as.branch_term.condition,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        break;
    case MIR_TERM_THROW:
        mr_mark_value_reachable_units(program,
                                      &terminator->as.throw_term.value,
                                      reachable,
                                      changed,
                                      ignore_module_init);
        break;
    case MIR_TERM_NONE:
    case MIR_TERM_GOTO:
        break;
    }
}

static void mr_mark_asm_body_reachable_units(const MirProgram *program,
                                             const char *asm_body,
                                             bool *reachable,
                                             bool *changed,
                                             bool ignore_module_init) {
    size_t i;

    if (!program || !asm_body) {
        return;
    }

    for (i = 0; i < program->unit_count; i++) {
        const char *unit_name = program->units[i].name;
        size_t unit_name_length;
        const char *cursor;

        if (!unit_name || unit_name[0] == '\0') {
            continue;
        }

        unit_name_length = strlen(unit_name);
        cursor = asm_body;
        while ((cursor = strstr(cursor, unit_name)) != NULL) {
            char before = cursor == asm_body ? '\0' : cursor[-1];
            char after = cursor[unit_name_length];

            if (!mr_is_unit_name_char(before) && !mr_is_unit_name_char(after)) {
                mr_mark_unit_name_reachable(program,
                                            unit_name,
                                            reachable,
                                            changed,
                                            ignore_module_init);
                break;
            }

            cursor++;
        }
    }
}

static void mr_mark_unit_reachable_dependencies(const MirProgram *program,
                                                const MirUnit *unit,
                                                bool *reachable,
                                                bool *changed,
                                                bool ignore_module_init) {
    size_t block_index;

    if (!program || !unit) {
        return;
    }

    if (unit->kind == MIR_UNIT_ASM) {
        mr_mark_asm_body_reachable_units(program,
                                         unit->asm_body,
                                         reachable,
                                         changed,
                                         ignore_module_init);
        return;
    }

    for (block_index = 0; block_index < unit->block_count; block_index++) {
        const MirBasicBlock *block = &unit->blocks[block_index];
        size_t instruction_index;

        for (instruction_index = 0;
             instruction_index < block->instruction_count;
             instruction_index++) {
            mr_mark_instruction_reachable_units(program,
                                               &block->instructions[instruction_index],
                                               reachable,
                                               changed,
                                               ignore_module_init);
        }

        mr_mark_terminator_reachable_units(program,
                                           &block->terminator,
                                           reachable,
                                           changed,
                                           ignore_module_init);
    }
}

static bool mr_compute_reachable_units(const MirProgram *program,
                                       bool *reachable,
                                       bool ignore_module_init,
                                       bool *has_start_root) {
    size_t i;
    bool changed;

    if (!program || !reachable) {
        return false;
    }

    if (has_start_root) {
        *has_start_root = false;
    }

    for (i = 0; i < program->unit_count; i++) {
        if (program->units[i].kind == MIR_UNIT_START) {
            reachable[i] = true;
            if (has_start_root) {
                *has_start_root = true;
            }
            continue;
        }

        /* Keep raw asm roots conservative until their bodies are dependency-aware. */
        if (program->units[i].kind == MIR_UNIT_ASM) {
            reachable[i] = true;
        }
    }

    do {
        changed = false;
        for (i = 0; i < program->unit_count; i++) {
            if (!reachable[i]) {
                continue;
            }

            mr_mark_unit_reachable_dependencies(program,
                                                &program->units[i],
                                                reachable,
                                                &changed,
                                                ignore_module_init);
        }
    } while (changed);

    return true;
}

static size_t mr_find_module_init_segment_index(const MirModuleInitSegment *segments,
                                                size_t segment_count,
                                                const char *global_name) {
    size_t i;

    if (!segments || !global_name) {
        return SIZE_MAX;
    }

    for (i = 0; i < segment_count; i++) {
        if (segments[i].global_name && strcmp(segments[i].global_name, global_name) == 0) {
            return i;
        }
    }

    return SIZE_MAX;
}

static void mr_mark_module_init_segment_reachable(const MirModuleInitSegment *segments,
                                                  size_t segment_count,
                                                  const char *global_name,
                                                  bool *reachable_segments,
                                                  bool *changed) {
    size_t segment_index;

    if (!segments || !global_name || !reachable_segments || !changed) {
        return;
    }

    segment_index = mr_find_module_init_segment_index(segments,
                                                      segment_count,
                                                      global_name);
    if (segment_index == SIZE_MAX || reachable_segments[segment_index]) {
        return;
    }

    reachable_segments[segment_index] = true;
    *changed = true;
}

static void mr_mark_value_reachable_module_init_segments(
    const MirModuleInitSegment *segments,
    size_t segment_count,
    const MirValue *value,
    bool *reachable_segments,
    bool *changed) {
    if (!value || value->kind != MIR_VALUE_GLOBAL) {
        return;
    }

    mr_mark_module_init_segment_reachable(segments,
                                          segment_count,
                                          value->as.global_name,
                                          reachable_segments,
                                          changed);
}

static void mr_mark_template_part_reachable_module_init_segments(
    const MirModuleInitSegment *segments,
    size_t segment_count,
    const MirTemplatePart *part,
    bool *reachable_segments,
    bool *changed) {
    if (!part || part->kind != MIR_TEMPLATE_PART_VALUE) {
        return;
    }

    mr_mark_value_reachable_module_init_segments(segments,
                                                 segment_count,
                                                 &part->as.value,
                                                 reachable_segments,
                                                 changed);
}

static void mr_mark_instruction_reachable_module_init_segments(
    const MirModuleInitSegment *segments,
    size_t segment_count,
    const MirInstruction *instruction,
    bool *reachable_segments,
    bool *changed) {
    size_t i;

    if (!segments || !instruction) {
        return;
    }

    switch (instruction->kind) {
    case MIR_INSTR_BINARY:
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &instruction->as.binary.left,
                                                     reachable_segments,
                                                     changed);
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &instruction->as.binary.right,
                                                     reachable_segments,
                                                     changed);
        break;
    case MIR_INSTR_UNARY:
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &instruction->as.unary.operand,
                                                     reachable_segments,
                                                     changed);
        break;
    case MIR_INSTR_CLOSURE:
        for (i = 0; i < instruction->as.closure.capture_count; i++) {
            mr_mark_value_reachable_module_init_segments(
                segments,
                segment_count,
                &instruction->as.closure.captures[i],
                reachable_segments,
                changed);
        }
        break;
    case MIR_INSTR_CALL:
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &instruction->as.call.callee,
                                                     reachable_segments,
                                                     changed);
        for (i = 0; i < instruction->as.call.argument_count; i++) {
            mr_mark_value_reachable_module_init_segments(
                segments,
                segment_count,
                &instruction->as.call.arguments[i],
                reachable_segments,
                changed);
        }
        break;
    case MIR_INSTR_CAST:
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &instruction->as.cast.operand,
                                                     reachable_segments,
                                                     changed);
        break;
    case MIR_INSTR_MEMBER:
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &instruction->as.member.target,
                                                     reachable_segments,
                                                     changed);
        break;
    case MIR_INSTR_INDEX_LOAD:
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &instruction->as.index_load.target,
                                                     reachable_segments,
                                                     changed);
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &instruction->as.index_load.index,
                                                     reachable_segments,
                                                     changed);
        break;
    case MIR_INSTR_ARRAY_LITERAL:
        for (i = 0; i < instruction->as.array_literal.element_count; i++) {
            mr_mark_value_reachable_module_init_segments(
                segments,
                segment_count,
                &instruction->as.array_literal.elements[i],
                reachable_segments,
                changed);
        }
        break;
    case MIR_INSTR_TEMPLATE:
        for (i = 0; i < instruction->as.template_literal.part_count; i++) {
            mr_mark_template_part_reachable_module_init_segments(
                segments,
                segment_count,
                &instruction->as.template_literal.parts[i],
                reachable_segments,
                changed);
        }
        break;
    case MIR_INSTR_STORE_LOCAL:
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &instruction->as.store_local.value,
                                                     reachable_segments,
                                                     changed);
        break;
    case MIR_INSTR_STORE_GLOBAL:
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &instruction->as.store_global.value,
                                                     reachable_segments,
                                                     changed);
        break;
    case MIR_INSTR_STORE_INDEX:
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &instruction->as.store_index.target,
                                                     reachable_segments,
                                                     changed);
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &instruction->as.store_index.index,
                                                     reachable_segments,
                                                     changed);
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &instruction->as.store_index.value,
                                                     reachable_segments,
                                                     changed);
        break;
    case MIR_INSTR_STORE_MEMBER:
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &instruction->as.store_member.target,
                                                     reachable_segments,
                                                     changed);
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &instruction->as.store_member.value,
                                                     reachable_segments,
                                                     changed);
        break;
    case MIR_INSTR_UNION_NEW:
        if (instruction->as.union_new.has_payload) {
            mr_mark_value_reachable_module_init_segments(segments,
                                                         segment_count,
                                                         &instruction->as.union_new.payload,
                                                         reachable_segments,
                                                         changed);
        }
        break;
    case MIR_INSTR_UNION_GET_TAG:
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &instruction->as.union_get_tag.target,
                                                     reachable_segments,
                                                     changed);
        break;
    case MIR_INSTR_UNION_GET_PAYLOAD:
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &instruction->as.union_get_payload.target,
                                                     reachable_segments,
                                                     changed);
        break;
    case MIR_INSTR_HETERO_ARRAY_NEW:
        for (i = 0; i < instruction->as.hetero_array_new.element_count; i++) {
            mr_mark_value_reachable_module_init_segments(
                segments,
                segment_count,
                &instruction->as.hetero_array_new.elements[i],
                reachable_segments,
                changed);
        }
        break;
    }
}

static void mr_mark_terminator_reachable_module_init_segments(
    const MirModuleInitSegment *segments,
    size_t segment_count,
    const MirTerminator *terminator,
    bool *reachable_segments,
    bool *changed) {
    if (!segments || !terminator) {
        return;
    }

    switch (terminator->kind) {
    case MIR_TERM_RETURN:
        if (terminator->as.return_term.has_value) {
            mr_mark_value_reachable_module_init_segments(segments,
                                                         segment_count,
                                                         &terminator->as.return_term.value,
                                                         reachable_segments,
                                                         changed);
        }
        break;
    case MIR_TERM_BRANCH:
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &terminator->as.branch_term.condition,
                                                     reachable_segments,
                                                     changed);
        break;
    case MIR_TERM_THROW:
        mr_mark_value_reachable_module_init_segments(segments,
                                                     segment_count,
                                                     &terminator->as.throw_term.value,
                                                     reachable_segments,
                                                     changed);
        break;
    case MIR_TERM_NONE:
    case MIR_TERM_GOTO:
        break;
    }
}

static void mr_mark_asm_body_reachable_module_init_segments(
    const MirModuleInitSegment *segments,
    size_t segment_count,
    const char *asm_body,
    bool *reachable_segments,
    bool *changed) {
    size_t i;

    if (!segments || !asm_body) {
        return;
    }

    for (i = 0; i < segment_count; i++) {
        const char *global_name = segments[i].global_name;
        size_t global_name_length;
        const char *cursor;

        if (!global_name || global_name[0] == '\0') {
            continue;
        }

        global_name_length = strlen(global_name);
        cursor = asm_body;
        while ((cursor = strstr(cursor, global_name)) != NULL) {
            char before = cursor == asm_body ? '\0' : cursor[-1];
            char after = cursor[global_name_length];

            if (!mr_is_unit_name_char(before) && !mr_is_unit_name_char(after)) {
                mr_mark_module_init_segment_reachable(segments,
                                                      segment_count,
                                                      global_name,
                                                      reachable_segments,
                                                      changed);
                break;
            }

            cursor++;
        }
    }
}

static void mr_mark_module_init_segment_dependencies(
    const MirUnit *module_init_unit,
    const MirModuleInitSegment *segment,
    const MirModuleInitSegment *segments,
    size_t segment_count,
    bool *reachable_segments,
    bool *changed) {
    size_t block_index;

    if (!module_init_unit || !segment || !segments || !reachable_segments || !changed) {
        return;
    }

    for (block_index = segment->start_block_index;
         block_index <= segment->end_block_index;
         block_index++) {
        const MirBasicBlock *block = &module_init_unit->blocks[block_index];
        size_t instruction_start = 0;
        size_t instruction_end = block->instruction_count;
        size_t instruction_index;

        if (block_index == segment->start_block_index) {
            instruction_start = segment->start_instruction_index;
        }
        if (block_index == segment->end_block_index) {
            instruction_end = segment->end_instruction_index + 1;
        }

        if (instruction_start > instruction_end || instruction_end > block->instruction_count) {
            continue;
        }

        for (instruction_index = instruction_start;
             instruction_index < instruction_end;
             instruction_index++) {
            mr_mark_instruction_reachable_module_init_segments(
                segments,
                segment_count,
                &block->instructions[instruction_index],
                reachable_segments,
                changed);
        }

        if (block_index < segment->end_block_index) {
            mr_mark_terminator_reachable_module_init_segments(segments,
                                                              segment_count,
                                                              &block->terminator,
                                                              reachable_segments,
                                                              changed);
        }
    }
}

static void mr_clear_block_terminator(MirBasicBlock *block) {
    if (!block) {
        return;
    }

    switch (block->terminator.kind) {
    case MIR_TERM_RETURN:
        if (block->terminator.as.return_term.has_value) {
            mr_value_free(&block->terminator.as.return_term.value);
        }
        break;
    case MIR_TERM_BRANCH:
        mr_value_free(&block->terminator.as.branch_term.condition);
        break;
    case MIR_TERM_THROW:
        mr_value_free(&block->terminator.as.throw_term.value);
        break;
    case MIR_TERM_NONE:
    case MIR_TERM_GOTO:
        break;
    }

    memset(&block->terminator, 0, sizeof(block->terminator));
}

static void mr_remove_instruction_range(MirBasicBlock *block,
                                        size_t start_index,
                                        size_t end_index) {
    size_t remove_count;
    size_t read_index;

    if (!block || start_index > end_index || end_index >= block->instruction_count) {
        return;
    }

    for (read_index = start_index; read_index <= end_index; read_index++) {
        mr_instruction_free(&block->instructions[read_index]);
    }

    remove_count = end_index - start_index + 1;
    for (read_index = end_index + 1; read_index < block->instruction_count; read_index++) {
        block->instructions[read_index - remove_count] = block->instructions[read_index];
        memset(&block->instructions[read_index], 0, sizeof(*block->instructions));
    }
    while (remove_count > 0) {
        block->instruction_count--;
        memset(&block->instructions[block->instruction_count], 0, sizeof(*block->instructions));
        remove_count--;
    }
}

static void mr_remove_module_init_segment(MirUnit *module_init_unit,
                                          const MirModuleInitSegment *segment) {
    MirBasicBlock *start_block;

    if (!module_init_unit || !segment || segment->end_block_index >= module_init_unit->block_count) {
        return;
    }

    start_block = &module_init_unit->blocks[segment->start_block_index];
    if (segment->start_block_index == segment->end_block_index) {
        mr_remove_instruction_range(start_block,
                                    segment->start_instruction_index,
                                    segment->end_instruction_index);
        return;
    }

    mr_remove_instruction_range(&module_init_unit->blocks[segment->end_block_index],
                                0,
                                segment->end_instruction_index);

    if (segment->start_instruction_index < start_block->instruction_count) {
        mr_remove_instruction_range(start_block,
                                    segment->start_instruction_index,
                                    start_block->instruction_count - 1);
    }

    mr_clear_block_terminator(start_block);
    start_block->terminator.kind = MIR_TERM_GOTO;
    start_block->terminator.as.goto_term.target_block = segment->end_block_index;
}

static void mr_mark_reachable_blocks(const MirUnit *unit,
                                     size_t block_index,
                                     bool *reachable_blocks) {
    const MirBasicBlock *block;

    if (!unit || !reachable_blocks || block_index >= unit->block_count ||
        reachable_blocks[block_index]) {
        return;
    }

    reachable_blocks[block_index] = true;
    block = &unit->blocks[block_index];
    switch (block->terminator.kind) {
    case MIR_TERM_GOTO:
        mr_mark_reachable_blocks(unit,
                                 block->terminator.as.goto_term.target_block,
                                 reachable_blocks);
        break;
    case MIR_TERM_BRANCH:
        mr_mark_reachable_blocks(unit,
                                 block->terminator.as.branch_term.true_block,
                                 reachable_blocks);
        mr_mark_reachable_blocks(unit,
                                 block->terminator.as.branch_term.false_block,
                                 reachable_blocks);
        break;
    case MIR_TERM_NONE:
    case MIR_TERM_RETURN:
    case MIR_TERM_THROW:
        break;
    }
}

static bool mr_compact_reachable_blocks(MirBuildContext *context, MirUnit *unit) {
    bool *reachable_blocks;
    size_t *remap;
    MirBasicBlock *new_blocks;
    size_t old_block_count;
    size_t kept_block_count = 0;
    size_t old_index;
    size_t new_index = 0;

    if (!context || !unit || unit->block_count == 0) {
        return true;
    }

    old_block_count = unit->block_count;
    reachable_blocks = calloc(old_block_count, sizeof(bool));
    remap = malloc(old_block_count * sizeof(size_t));
    if (!reachable_blocks || !remap) {
        free(reachable_blocks);
        free(remap);
        mr_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Out of memory while compacting reachable MIR blocks.");
        return false;
    }

    for (old_index = 0; old_index < old_block_count; old_index++) {
        remap[old_index] = SIZE_MAX;
    }

    mr_mark_reachable_blocks(unit, 0, reachable_blocks);
    for (old_index = 0; old_index < old_block_count; old_index++) {
        if (reachable_blocks[old_index]) {
            kept_block_count++;
        }
    }

    if (kept_block_count == old_block_count) {
        free(reachable_blocks);
        free(remap);
        return true;
    }

    new_blocks = calloc(kept_block_count, sizeof(*new_blocks));
    if (!new_blocks) {
        free(reachable_blocks);
        free(remap);
        mr_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Out of memory while rewriting reachable MIR blocks.");
        return false;
    }

    for (old_index = 0; old_index < old_block_count; old_index++) {
        if (!reachable_blocks[old_index]) {
            mr_basic_block_free(&unit->blocks[old_index]);
            continue;
        }

        remap[old_index] = new_index;
        new_blocks[new_index] = unit->blocks[old_index];
        memset(&unit->blocks[old_index], 0, sizeof(*unit->blocks));
        new_index++;
    }

    free(unit->blocks);
    unit->blocks = new_blocks;
    unit->block_count = kept_block_count;
    unit->block_capacity = kept_block_count;

    for (new_index = 0; new_index < kept_block_count; new_index++) {
        MirBasicBlock *block = &unit->blocks[new_index];

        switch (block->terminator.kind) {
        case MIR_TERM_GOTO:
            block->terminator.as.goto_term.target_block =
                remap[block->terminator.as.goto_term.target_block];
            break;
        case MIR_TERM_BRANCH:
            block->terminator.as.branch_term.true_block =
                remap[block->terminator.as.branch_term.true_block];
            block->terminator.as.branch_term.false_block =
                remap[block->terminator.as.branch_term.false_block];
            break;
        case MIR_TERM_NONE:
        case MIR_TERM_RETURN:
        case MIR_TERM_THROW:
            break;
        }
    }

    free(reachable_blocks);
    free(remap);
    return true;
}

static void mr_remove_module_init_call_from_start_units(MirProgram *program) {
    size_t unit_index;

    if (!program) {
        return;
    }

    for (unit_index = 0; unit_index < program->unit_count; unit_index++) {
        MirUnit *unit = &program->units[unit_index];
        MirBasicBlock *entry_block;
        size_t instruction_index;

        if (unit->kind != MIR_UNIT_START || unit->block_count == 0) {
            continue;
        }

        entry_block = &unit->blocks[0];
        if (entry_block->instruction_count == 0 ||
            !mr_is_module_init_call(&entry_block->instructions[0])) {
            continue;
        }

        mr_instruction_free(&entry_block->instructions[0]);
        for (instruction_index = 1;
             instruction_index < entry_block->instruction_count;
             instruction_index++) {
            entry_block->instructions[instruction_index - 1] =
                entry_block->instructions[instruction_index];
            memset(&entry_block->instructions[instruction_index],
                   0,
                   sizeof(*entry_block->instructions));
        }
        entry_block->instruction_count--;
    }
}

static bool mr_prune_dead_module_init_segments(MirBuildContext *context,
                                               const MirModuleInitSegment *segments,
                                               size_t segment_count) {
    MirProgram *program;
    MirUnit *module_init_unit = NULL;
    bool *reachable_units = NULL;
    bool *reachable_segments = NULL;
    bool has_start_root = false;
    size_t unit_index;
    size_t segment_index;
    bool has_live_segments = false;
    bool changed;

    if (!context || !context->program) {
        return false;
    }

    program = context->program;
    for (unit_index = 0; unit_index < program->unit_count; unit_index++) {
        if (program->units[unit_index].kind == MIR_UNIT_INIT &&
            mr_is_module_init_name(program->units[unit_index].name)) {
            module_init_unit = &program->units[unit_index];
            break;
        }
    }

    if (!module_init_unit || !segments || segment_count == 0) {
        return true;
    }

    reachable_units = calloc(program->unit_count, sizeof(bool));
    reachable_segments = calloc(segment_count, sizeof(bool));
    if (!reachable_units || !reachable_segments) {
        free(reachable_units);
        free(reachable_segments);
        mr_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Out of memory while pruning MIR module init segments.");
        return false;
    }

    if (!mr_compute_reachable_units(program,
                                    reachable_units,
                                    true,
                                    &has_start_root)) {
        free(reachable_units);
        free(reachable_segments);
        mr_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Internal error while scanning reachable MIR units.");
        return false;
    }

    if (!has_start_root) {
        free(reachable_units);
        free(reachable_segments);
        return true;
    }

    for (segment_index = 0; segment_index < segment_count; segment_index++) {
        if (!segments[segment_index].is_pure) {
            reachable_segments[segment_index] = true;
        }
    }

    for (unit_index = 0; unit_index < program->unit_count; unit_index++) {
        const MirUnit *unit = &program->units[unit_index];
        size_t block_index;

        if (!reachable_units[unit_index] || unit == module_init_unit) {
            continue;
        }

        if (unit->kind == MIR_UNIT_ASM) {
            changed = false;
            mr_mark_asm_body_reachable_module_init_segments(segments,
                                                            segment_count,
                                                            unit->asm_body,
                                                            reachable_segments,
                                                            &changed);
            continue;
        }

        for (block_index = 0; block_index < unit->block_count; block_index++) {
            const MirBasicBlock *other_block = &unit->blocks[block_index];
            size_t instruction_index;

            for (instruction_index = 0;
                 instruction_index < other_block->instruction_count;
                 instruction_index++) {
                changed = false;
                mr_mark_instruction_reachable_module_init_segments(
                    segments,
                    segment_count,
                    &other_block->instructions[instruction_index],
                    reachable_segments,
                    &changed);
            }

            changed = false;
            mr_mark_terminator_reachable_module_init_segments(segments,
                                                              segment_count,
                                                              &other_block->terminator,
                                                              reachable_segments,
                                                              &changed);
        }
    }

    do {
        changed = false;
        for (segment_index = 0; segment_index < segment_count; segment_index++) {
            if (!reachable_segments[segment_index]) {
                continue;
            }

            mr_mark_module_init_segment_dependencies(module_init_unit,
                                                     &segments[segment_index],
                                                     segments,
                                                     segment_count,
                                                     reachable_segments,
                                                     &changed);
        }
    } while (changed);

    for (segment_index = segment_count; segment_index > 0; segment_index--) {
        const MirModuleInitSegment *segment = &segments[segment_index - 1];

        if (!reachable_segments[segment_index - 1] && segment->is_pure) {
            mr_remove_module_init_segment(module_init_unit, segment);
            continue;
        }

        has_live_segments = true;
    }

    if (!mr_compact_reachable_blocks(context, module_init_unit)) {
        free(reachable_units);
        free(reachable_segments);
        return false;
    }

    if (!has_live_segments) {
        mr_remove_module_init_call_from_start_units(program);
    }

    free(reachable_units);
    free(reachable_segments);
    return true;
}

static bool mr_prune_unreachable_units(MirBuildContext *context) {
    MirProgram *program;
    bool *reachable;
    bool has_start_root = false;
    size_t i;
    size_t kept_unit_count = 0;

    if (!context || !context->program) {
        return false;
    }

    program = context->program;
    if (program->unit_count == 0) {
        return true;
    }

    reachable = calloc(program->unit_count, sizeof(bool));
    if (!reachable) {
        mr_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Out of memory while pruning unreachable MIR units.");
        return false;
    }

    if (!mr_compute_reachable_units(program, reachable, false, &has_start_root)) {
        free(reachable);
        mr_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Internal error while pruning unreachable MIR units.");
        return false;
    }

    if (!has_start_root) {
        free(reachable);
        return true;
    }

    for (i = 0; i < program->unit_count; i++) {
        if (!reachable[i]) {
            mr_unit_free(&program->units[i]);
            continue;
        }

        if (kept_unit_count != i) {
            program->units[kept_unit_count] = program->units[i];
            memset(&program->units[i], 0, sizeof(program->units[i]));
        }
        kept_unit_count++;
    }

    program->unit_count = kept_unit_count;
    free(reachable);
    return true;
}

bool mir_build_program(MirProgram *program, const HirProgram *hir_program,
                       bool global_bounds_check) {
    MirBuildContext context;
    const HirStartDecl *start_decl = NULL;
    MirModuleInitSegment *module_init_segments = NULL;
    size_t module_init_segment_count = 0;
    size_t i;
    bool created_module_init_unit;

    if (!program || !hir_program) {
        return false;
    }

    mir_program_free(program);
    mir_program_init(program);

    memset(&context, 0, sizeof(context));
    context.program = program;
    context.hir_program = hir_program;
    context.global_bounds_check = global_bounds_check;

    if (hir_get_error(hir_program) != NULL) {
        const HirBuildError *hir_error = hir_get_error(hir_program);

        mr_set_error(&context,
                     hir_error->primary_span,
                     hir_error->has_related_span
                         ? &hir_error->related_span
                         : NULL,
                     "%s",
                     hir_error->message);
        return false;
    }

    for (i = 0; i < hir_program->top_level_count; i++) {
        const HirTopLevelDecl *decl = hir_program->top_level_decls[i];

        if (decl->kind == HIR_TOP_LEVEL_START) {
            start_decl = &decl->as.start;
            continue;
        }

        /* Union declarations are type-only metadata; no MIR code emitted. */
        if (decl->kind == HIR_TOP_LEVEL_UNION) {
            continue;
        }

        if (decl->kind == HIR_TOP_LEVEL_ASM) {
            MirUnit asm_unit;
            memset(&asm_unit, 0, sizeof(asm_unit));
            asm_unit.kind = MIR_UNIT_ASM;
            asm_unit.name = ast_copy_text(mr_named_symbol_global_name(
                decl->as.asm_decl.name,
                decl->as.asm_decl.symbol));
            asm_unit.symbol = decl->as.asm_decl.symbol;
            asm_unit.return_type = decl->as.asm_decl.return_type;
            asm_unit.parameter_count = decl->as.asm_decl.parameter_count;
            asm_unit.asm_body = ast_copy_text_n(decl->as.asm_decl.body,
                                                decl->as.asm_decl.body_length);
            asm_unit.asm_body_length = decl->as.asm_decl.body_length;
            if (!asm_unit.name || !asm_unit.asm_body) {
                mr_unit_free(&asm_unit);
                mr_set_error(&context,
                              decl->as.asm_decl.source_span,
                              NULL,
                              "Out of memory while lowering asm unit '%s'.",
                              decl->as.asm_decl.name);
                return false;
            }
            if (!mr_append_unit(context.program, asm_unit)) {
                mr_unit_free(&asm_unit);
                mr_set_error(&context,
                              decl->as.asm_decl.source_span,
                              NULL,
                              "Out of memory while appending asm unit '%s'.",
                              decl->as.asm_decl.name);
                return false;
            }
            continue;
        }

        if (mr_top_level_binding_uses_lambda_unit(decl)) {
            if (!mr_lower_lambda_unit(&context,
                                   mr_named_symbol_global_name(
                                       decl->as.binding.name,
                                       decl->as.binding.symbol),
                                   decl->as.binding.symbol,
                                   &decl->as.binding.initializer->as.lambda,
                                   decl->as.binding.callable_signature.return_type,
                                   MIR_UNIT_BINDING)) {
                return false;
            }
            continue;
        }
    }

    if (!mr_lower_module_init_unit(&context,
                                   &created_module_init_unit,
                                   &module_init_segments,
                                   &module_init_segment_count)) {
        return false;
    }

    if (start_decl &&
        !mr_lower_start_unit(&context, start_decl, created_module_init_unit)) {
        free(module_init_segments);
        return false;
    }

    if (!mr_prune_dead_module_init_segments(&context,
                                            module_init_segments,
                                            module_init_segment_count)) {
        free(module_init_segments);
        return false;
    }

    if (!mr_prune_unreachable_units(&context)) {
        free(module_init_segments);
        return false;
    }

    free(module_init_segments);

    for (i = 0; i < program->unit_count; i++) {
        mir_apply_self_tco(&program->units[i]);
    }

    return true;
}
