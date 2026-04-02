#include "bytecode.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    BytecodeProgram *program;
    const MirProgram *mir_program;
} BytecodeBuildContext;

static bool reserve_items(void **items, size_t *capacity, size_t needed, size_t item_size);
static bool source_span_is_valid(AstSourceSpan span);
static void bytecode_set_error(BytecodeBuildContext *context,
                               AstSourceSpan primary_span,
                               const AstSourceSpan *related_span,
                               const char *format,
                               ...);
static BytecodeValue bytecode_invalid_value(void);
static void bytecode_constant_free(BytecodeConstant *constant);
static void bytecode_template_part_free(BytecodeTemplatePart *part);
static void bytecode_instruction_free(BytecodeInstruction *instruction);
static void bytecode_block_free(BytecodeBasicBlock *block);
static void bytecode_unit_free(BytecodeUnit *unit);
static BytecodeLocalKind local_kind_from_mir_kind(MirLocalKind kind);
static BytecodeUnitKind unit_kind_from_mir_kind(MirUnitKind kind);
static size_t find_unit_index(const MirProgram *program, const char *name);
static char *normalize_literal_text(AstLiteralKind kind, const char *text);
static size_t intern_text_constant(BytecodeBuildContext *context,
                                   BytecodeConstantKind kind,
                                   const char *text);
static size_t intern_literal_constant(BytecodeBuildContext *context,
                                      AstLiteralKind kind,
                                      const char *text,
                                      bool bool_value);
static bool bytecode_value_from_mir_value(BytecodeBuildContext *context,
                                          MirValue value,
                                          BytecodeValue *lowered);
static bool lower_template_part(BytecodeBuildContext *context,
                                const MirTemplatePart *part,
                                BytecodeTemplatePart *lowered);
static bool lower_instruction(BytecodeBuildContext *context,
                              const MirInstruction *instruction,
                              BytecodeInstruction *lowered);
static bool lower_terminator(BytecodeBuildContext *context,
                             const MirTerminator *terminator,
                             BytecodeTerminator *lowered);
static bool lower_unit(BytecodeBuildContext *context,
                       const MirUnit *mir_unit,
                       BytecodeUnit *unit);

void bytecode_program_init(BytecodeProgram *program) {
    if (!program) {
        return;
    }

    memset(program, 0, sizeof(*program));
    program->target = BYTECODE_TARGET_PORTABLE_V1;
}

void bytecode_program_free(BytecodeProgram *program) {
    size_t i;

    if (!program) {
        return;
    }

    for (i = 0; i < program->constant_count; i++) {
        bytecode_constant_free(&program->constants[i]);
    }
    free(program->constants);

    for (i = 0; i < program->unit_count; i++) {
        bytecode_unit_free(&program->units[i]);
    }
    free(program->units);

    memset(program, 0, sizeof(*program));
}

const BytecodeBuildError *bytecode_get_error(const BytecodeProgram *program) {
    if (!program || !program->has_error) {
        return NULL;
    }

    return &program->error;
}

bool bytecode_format_error(const BytecodeBuildError *error,
                           char *buffer,
                           size_t buffer_size) {
    int written;

    if (!error || !buffer || buffer_size == 0) {
        return false;
    }

    if (source_span_is_valid(error->primary_span)) {
        if (error->has_related_span && source_span_is_valid(error->related_span)) {
            written = snprintf(buffer,
                               buffer_size,
                               "%d:%d: %s Related location at %d:%d.",
                               error->primary_span.start_line,
                               error->primary_span.start_column,
                               error->message,
                               error->related_span.start_line,
                               error->related_span.start_column);
        } else {
            written = snprintf(buffer,
                               buffer_size,
                               "%d:%d: %s",
                               error->primary_span.start_line,
                               error->primary_span.start_column,
                               error->message);
        }
    } else {
        written = snprintf(buffer, buffer_size, "%s", error->message);
    }

    return written >= 0 && (size_t)written < buffer_size;
}

const char *bytecode_target_name(BytecodeTargetKind target) {
    switch (target) {
    case BYTECODE_TARGET_PORTABLE_V1:
        return "portable-v1";
    }

    return "unknown";
}

bool bytecode_build_program(BytecodeProgram *program, const MirProgram *mir_program) {
    BytecodeBuildContext context;
    size_t unit_index;

    if (!program || !mir_program) {
        return false;
    }

    bytecode_program_free(program);
    bytecode_program_init(program);

    memset(&context, 0, sizeof(context));
    context.program = program;
    context.mir_program = mir_program;

    if (mir_get_error(mir_program) != NULL) {
        bytecode_set_error(&context,
                           (AstSourceSpan){0},
                           NULL,
                           "Cannot lower bytecode while the MIR reports errors.");
        return false;
    }

    program->unit_count = mir_program->unit_count;
    if (program->unit_count > 0) {
        program->units = calloc(program->unit_count, sizeof(*program->units));
        if (!program->units) {
            bytecode_set_error(&context,
                               (AstSourceSpan){0},
                               NULL,
                               "Out of memory while allocating bytecode units.");
            return false;
        }
    }

    for (unit_index = 0; unit_index < mir_program->unit_count; unit_index++) {
        if (!lower_unit(&context, &mir_program->units[unit_index], &program->units[unit_index])) {
            return false;
        }
    }

    return true;
}

static bool reserve_items(void **items, size_t *capacity, size_t needed, size_t item_size) {
    void *resized;
    size_t new_capacity;

    if (*capacity >= needed) {
        return true;
    }

    new_capacity = (*capacity == 0) ? 8 : *capacity;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }

    resized = realloc(*items, new_capacity * item_size);
    if (!resized) {
        return false;
    }

    *items = resized;
    *capacity = new_capacity;
    return true;
}

static bool source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

static void bytecode_set_error(BytecodeBuildContext *context,
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
    if (related_span && source_span_is_valid(*related_span)) {
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

static BytecodeValue bytecode_invalid_value(void) {
    BytecodeValue value;

    memset(&value, 0, sizeof(value));
    value.kind = BYTECODE_VALUE_INVALID;
    return value;
}

static void bytecode_constant_free(BytecodeConstant *constant) {
    if (!constant) {
        return;
    }

    if (constant->kind == BYTECODE_CONSTANT_LITERAL) {
        free(constant->as.literal.text);
    } else {
        free(constant->as.text);
    }
    memset(constant, 0, sizeof(*constant));
}

static void bytecode_template_part_free(BytecodeTemplatePart *part) {
    if (!part) {
        return;
    }

    memset(part, 0, sizeof(*part));
}

static void bytecode_instruction_free(BytecodeInstruction *instruction) {
    size_t i;

    if (!instruction) {
        return;
    }

    switch (instruction->kind) {
    case BYTECODE_INSTR_CLOSURE:
        free(instruction->as.closure.captures);
        break;
    case BYTECODE_INSTR_CALL:
        free(instruction->as.call.arguments);
        break;
    case BYTECODE_INSTR_ARRAY_LITERAL:
        free(instruction->as.array_literal.elements);
        break;
    case BYTECODE_INSTR_TEMPLATE:
        for (i = 0; i < instruction->as.template_literal.part_count; i++) {
            bytecode_template_part_free(&instruction->as.template_literal.parts[i]);
        }
        free(instruction->as.template_literal.parts);
        break;
    case BYTECODE_INSTR_BINARY:
    case BYTECODE_INSTR_UNARY:
    case BYTECODE_INSTR_CAST:
    case BYTECODE_INSTR_MEMBER:
    case BYTECODE_INSTR_INDEX_LOAD:
    case BYTECODE_INSTR_STORE_LOCAL:
    case BYTECODE_INSTR_STORE_GLOBAL:
    case BYTECODE_INSTR_STORE_INDEX:
    case BYTECODE_INSTR_STORE_MEMBER:
        break;
    }

    memset(instruction, 0, sizeof(*instruction));
}

static void bytecode_block_free(BytecodeBasicBlock *block) {
    size_t i;

    if (!block) {
        return;
    }

    free(block->label);
    for (i = 0; i < block->instruction_count; i++) {
        bytecode_instruction_free(&block->instructions[i]);
    }
    free(block->instructions);
    memset(block, 0, sizeof(*block));
}

static void bytecode_unit_free(BytecodeUnit *unit) {
    size_t i;

    if (!unit) {
        return;
    }

    free(unit->name);
    for (i = 0; i < unit->local_count; i++) {
        free(unit->locals[i].name);
    }
    free(unit->locals);

    for (i = 0; i < unit->block_count; i++) {
        bytecode_block_free(&unit->blocks[i]);
    }
    free(unit->blocks);
    memset(unit, 0, sizeof(*unit));
}

static BytecodeLocalKind local_kind_from_mir_kind(MirLocalKind kind) {
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

static BytecodeUnitKind unit_kind_from_mir_kind(MirUnitKind kind) {
    switch (kind) {
    case MIR_UNIT_START:
        return BYTECODE_UNIT_START;
    case MIR_UNIT_BINDING:
        return BYTECODE_UNIT_BINDING;
    case MIR_UNIT_INIT:
        return BYTECODE_UNIT_INIT;
    case MIR_UNIT_LAMBDA:
        return BYTECODE_UNIT_LAMBDA;
    }

    return BYTECODE_UNIT_BINDING;
}

static size_t find_unit_index(const MirProgram *program, const char *name) {
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

static char *normalize_literal_text(AstLiteralKind kind, const char *text) {
    char *normalized;
    size_t input_index;
    size_t output_index = 0;
    size_t length;
    char quote;

    if (!text) {
        return NULL;
    }
    if (kind != AST_LITERAL_STRING && kind != AST_LITERAL_CHAR) {
        return ast_copy_text(text);
    }

    length = strlen(text);
    if (length < 2) {
        return ast_copy_text(text);
    }
    quote = kind == AST_LITERAL_CHAR ? '\'' : '"';
    if (text[0] != quote || text[length - 1] != quote) {
        return ast_copy_text(text);
    }

    normalized = malloc(length - 1);
    if (!normalized) {
        return NULL;
    }
    for (input_index = 1; input_index + 1 < length; input_index++) {
        if (text[input_index] == '\\' && input_index + 2 < length) {
            input_index++;
            switch (text[input_index]) {
            case 'n':
                normalized[output_index++] = '\n';
                break;
            case 'r':
                normalized[output_index++] = '\r';
                break;
            case 't':
                normalized[output_index++] = '\t';
                break;
            case '0':
                normalized[output_index++] = '\0';
                break;
            default:
                normalized[output_index++] = text[input_index];
                break;
            }
        } else {
            normalized[output_index++] = text[input_index];
        }
    }
    normalized[output_index] = '\0';
    return normalized;
}

static size_t intern_text_constant(BytecodeBuildContext *context,
                                   BytecodeConstantKind kind,
                                   const char *text) {
    size_t i;
    char *copy;

    if (!context || !context->program || !text) {
        return (size_t)-1;
    }

    for (i = 0; i < context->program->constant_count; i++) {
        if (context->program->constants[i].kind == kind &&
            strcmp(context->program->constants[i].as.text, text) == 0) {
            return i;
        }
    }

    if (!reserve_items((void **)&context->program->constants,
                       &context->program->constant_capacity,
                       context->program->constant_count + 1,
                       sizeof(*context->program->constants))) {
        return (size_t)-1;
    }

    copy = ast_copy_text(text);
    if (!copy) {
        return (size_t)-1;
    }

    i = context->program->constant_count++;
    context->program->constants[i].kind = kind;
    context->program->constants[i].as.text = copy;
    return i;
}

static size_t intern_literal_constant(BytecodeBuildContext *context,
                                      AstLiteralKind kind,
                                      const char *text,
                                      bool bool_value) {
    size_t i;
    char *normalized_text = NULL;

    if (!context || !context->program) {
        return (size_t)-1;
    }

    normalized_text = normalize_literal_text(kind, text);
    if (text && !normalized_text) {
        return (size_t)-1;
    }

    for (i = 0; i < context->program->constant_count; i++) {
        const BytecodeConstant *constant = &context->program->constants[i];

        if (constant->kind != BYTECODE_CONSTANT_LITERAL ||
            constant->as.literal.kind != kind ||
            constant->as.literal.bool_value != bool_value) {
            continue;
        }
        if (!constant->as.literal.text && !text) {
            free(normalized_text);
            return i;
        }
        if (constant->as.literal.text && normalized_text &&
            strcmp(constant->as.literal.text, normalized_text) == 0) {
            free(normalized_text);
            return i;
        }
    }

    if (!reserve_items((void **)&context->program->constants,
                       &context->program->constant_capacity,
                       context->program->constant_count + 1,
                       sizeof(*context->program->constants))) {
        return (size_t)-1;
    }

    i = context->program->constant_count++;
    context->program->constants[i].kind = BYTECODE_CONSTANT_LITERAL;
    context->program->constants[i].as.literal.kind = kind;
    context->program->constants[i].as.literal.text = normalized_text;
    context->program->constants[i].as.literal.bool_value = bool_value;
    return i;
}

static bool bytecode_value_from_mir_value(BytecodeBuildContext *context,
                                          MirValue value,
                                          BytecodeValue *lowered) {
    size_t index;

    if (!lowered) {
        return false;
    }

    *lowered = bytecode_invalid_value();
    lowered->type = value.type;

    switch (value.kind) {
    case MIR_VALUE_INVALID:
        return true;
    case MIR_VALUE_TEMP:
        lowered->kind = BYTECODE_VALUE_TEMP;
        lowered->as.temp_index = value.as.temp_index;
        return true;
    case MIR_VALUE_LOCAL:
        lowered->kind = BYTECODE_VALUE_LOCAL;
        lowered->as.local_index = value.as.local_index;
        return true;
    case MIR_VALUE_GLOBAL:
        index = intern_text_constant(context, BYTECODE_CONSTANT_SYMBOL, value.as.global_name);
        if (index == (size_t)-1) {
            return false;
        }
        lowered->kind = BYTECODE_VALUE_GLOBAL;
        lowered->as.global_index = index;
        return true;
    case MIR_VALUE_LITERAL:
        index = intern_literal_constant(context,
                                        value.as.literal.kind,
                                        value.as.literal.text,
                                        value.as.literal.bool_value);
        if (index == (size_t)-1) {
            return false;
        }
        lowered->kind = BYTECODE_VALUE_CONSTANT;
        lowered->as.constant_index = index;
        return true;
    }

    return false;
}

static bool lower_template_part(BytecodeBuildContext *context,
                                const MirTemplatePart *part,
                                BytecodeTemplatePart *lowered) {
    size_t text_index;

    if (!part || !lowered) {
        return false;
    }

    memset(lowered, 0, sizeof(*lowered));
    if (part->kind == MIR_TEMPLATE_PART_TEXT) {
        text_index = intern_text_constant(context, BYTECODE_CONSTANT_TEXT, part->as.text ? part->as.text : "");
        if (text_index == (size_t)-1) {
            return false;
        }
        lowered->kind = BYTECODE_TEMPLATE_PART_TEXT;
        lowered->as.text_index = text_index;
        return true;
    }

    lowered->kind = BYTECODE_TEMPLATE_PART_VALUE;
    return bytecode_value_from_mir_value(context, part->as.value, &lowered->as.value);
}

static bool lower_instruction(BytecodeBuildContext *context,
                              const MirInstruction *instruction,
                              BytecodeInstruction *lowered) {
    size_t i;

    if (!instruction || !lowered) {
        return false;
    }

    memset(lowered, 0, sizeof(*lowered));
    lowered->kind = (BytecodeInstructionKind)instruction->kind;

    switch (instruction->kind) {
    case MIR_INSTR_BINARY:
        lowered->as.binary.dest_temp = instruction->as.binary.dest_temp;
        lowered->as.binary.operator = instruction->as.binary.operator;
        return bytecode_value_from_mir_value(context,
                                             instruction->as.binary.left,
                                             &lowered->as.binary.left) &&
               bytecode_value_from_mir_value(context,
                                             instruction->as.binary.right,
                                             &lowered->as.binary.right);

    case MIR_INSTR_UNARY:
        lowered->as.unary.dest_temp = instruction->as.unary.dest_temp;
        lowered->as.unary.operator = instruction->as.unary.operator;
        return bytecode_value_from_mir_value(context,
                                             instruction->as.unary.operand,
                                             &lowered->as.unary.operand);

    case MIR_INSTR_CLOSURE:
        lowered->as.closure.dest_temp = instruction->as.closure.dest_temp;
        lowered->as.closure.unit_index = find_unit_index(context->mir_program,
                                                         instruction->as.closure.unit_name);
        lowered->as.closure.capture_count = instruction->as.closure.capture_count;
        if (lowered->as.closure.unit_index == (size_t)-1) {
            bytecode_set_error(context,
                               (AstSourceSpan){0},
                               NULL,
                               "Bytecode lowering could not find MIR unit %s for closure construction.",
                               instruction->as.closure.unit_name);
            return false;
        }
        if (lowered->as.closure.capture_count > 0) {
            lowered->as.closure.captures = calloc(lowered->as.closure.capture_count,
                                                  sizeof(*lowered->as.closure.captures));
            if (!lowered->as.closure.captures) {
                return false;
            }
        }
        for (i = 0; i < lowered->as.closure.capture_count; i++) {
            if (!bytecode_value_from_mir_value(context,
                                               instruction->as.closure.captures[i],
                                               &lowered->as.closure.captures[i])) {
                return false;
            }
        }
        return true;

    case MIR_INSTR_CALL:
        lowered->as.call.has_result = instruction->as.call.has_result;
        lowered->as.call.dest_temp = instruction->as.call.dest_temp;
        lowered->as.call.argument_count = instruction->as.call.argument_count;
        if (!bytecode_value_from_mir_value(context,
                                           instruction->as.call.callee,
                                           &lowered->as.call.callee)) {
            return false;
        }
        if (lowered->as.call.argument_count > 0) {
            lowered->as.call.arguments = calloc(lowered->as.call.argument_count,
                                                sizeof(*lowered->as.call.arguments));
            if (!lowered->as.call.arguments) {
                return false;
            }
        }
        for (i = 0; i < lowered->as.call.argument_count; i++) {
            if (!bytecode_value_from_mir_value(context,
                                               instruction->as.call.arguments[i],
                                               &lowered->as.call.arguments[i])) {
                return false;
            }
        }
        return true;

    case MIR_INSTR_CAST:
        lowered->as.cast.dest_temp = instruction->as.cast.dest_temp;
        lowered->as.cast.target_type = instruction->as.cast.target_type;
        return bytecode_value_from_mir_value(context,
                                             instruction->as.cast.operand,
                                             &lowered->as.cast.operand);

    case MIR_INSTR_MEMBER:
        lowered->as.member.dest_temp = instruction->as.member.dest_temp;
        lowered->as.member.member_index = intern_text_constant(context,
                                                               BYTECODE_CONSTANT_SYMBOL,
                                                               instruction->as.member.member);
        return lowered->as.member.member_index != (size_t)-1 &&
               bytecode_value_from_mir_value(context,
                                             instruction->as.member.target,
                                             &lowered->as.member.target);

    case MIR_INSTR_INDEX_LOAD:
        lowered->as.index_load.dest_temp = instruction->as.index_load.dest_temp;
        return bytecode_value_from_mir_value(context,
                                             instruction->as.index_load.target,
                                             &lowered->as.index_load.target) &&
               bytecode_value_from_mir_value(context,
                                             instruction->as.index_load.index,
                                             &lowered->as.index_load.index);

    case MIR_INSTR_ARRAY_LITERAL:
        lowered->as.array_literal.dest_temp = instruction->as.array_literal.dest_temp;
        lowered->as.array_literal.element_count = instruction->as.array_literal.element_count;
        if (lowered->as.array_literal.element_count > 0) {
            lowered->as.array_literal.elements = calloc(lowered->as.array_literal.element_count,
                                                        sizeof(*lowered->as.array_literal.elements));
            if (!lowered->as.array_literal.elements) {
                return false;
            }
        }
        for (i = 0; i < lowered->as.array_literal.element_count; i++) {
            if (!bytecode_value_from_mir_value(context,
                                               instruction->as.array_literal.elements[i],
                                               &lowered->as.array_literal.elements[i])) {
                return false;
            }
        }
        return true;

    case MIR_INSTR_TEMPLATE:
        lowered->as.template_literal.dest_temp = instruction->as.template_literal.dest_temp;
        lowered->as.template_literal.part_count = instruction->as.template_literal.part_count;
        if (lowered->as.template_literal.part_count > 0) {
            lowered->as.template_literal.parts = calloc(lowered->as.template_literal.part_count,
                                                        sizeof(*lowered->as.template_literal.parts));
            if (!lowered->as.template_literal.parts) {
                return false;
            }
        }
        for (i = 0; i < lowered->as.template_literal.part_count; i++) {
            if (!lower_template_part(context,
                                     &instruction->as.template_literal.parts[i],
                                     &lowered->as.template_literal.parts[i])) {
                return false;
            }
        }
        return true;

    case MIR_INSTR_STORE_LOCAL:
        lowered->as.store_local.local_index = instruction->as.store_local.local_index;
        return bytecode_value_from_mir_value(context,
                                             instruction->as.store_local.value,
                                             &lowered->as.store_local.value);

    case MIR_INSTR_STORE_GLOBAL:
        lowered->as.store_global.global_index = intern_text_constant(context,
                                                                     BYTECODE_CONSTANT_SYMBOL,
                                                                     instruction->as.store_global.global_name);
        return lowered->as.store_global.global_index != (size_t)-1 &&
               bytecode_value_from_mir_value(context,
                                             instruction->as.store_global.value,
                                             &lowered->as.store_global.value);

    case MIR_INSTR_STORE_INDEX:
        return bytecode_value_from_mir_value(context,
                                             instruction->as.store_index.target,
                                             &lowered->as.store_index.target) &&
               bytecode_value_from_mir_value(context,
                                             instruction->as.store_index.index,
                                             &lowered->as.store_index.index) &&
               bytecode_value_from_mir_value(context,
                                             instruction->as.store_index.value,
                                             &lowered->as.store_index.value);

    case MIR_INSTR_STORE_MEMBER:
        lowered->as.store_member.member_index = intern_text_constant(context,
                                                                     BYTECODE_CONSTANT_SYMBOL,
                                                                     instruction->as.store_member.member);
        return lowered->as.store_member.member_index != (size_t)-1 &&
               bytecode_value_from_mir_value(context,
                                             instruction->as.store_member.target,
                                             &lowered->as.store_member.target) &&
               bytecode_value_from_mir_value(context,
                                             instruction->as.store_member.value,
                                             &lowered->as.store_member.value);
    }

    return false;
}

static bool lower_terminator(BytecodeBuildContext *context,
                             const MirTerminator *terminator,
                             BytecodeTerminator *lowered) {
    if (!terminator || !lowered) {
        return false;
    }

    memset(lowered, 0, sizeof(*lowered));
    lowered->kind = (BytecodeTerminatorKind)terminator->kind;

    switch (terminator->kind) {
    case MIR_TERM_NONE:
        return true;
    case MIR_TERM_RETURN:
        lowered->as.return_term.has_value = terminator->as.return_term.has_value;
        if (!lowered->as.return_term.has_value) {
            return true;
        }
        return bytecode_value_from_mir_value(context,
                                             terminator->as.return_term.value,
                                             &lowered->as.return_term.value);
    case MIR_TERM_GOTO:
        lowered->as.jump_term.target_block = terminator->as.goto_term.target_block;
        return true;
    case MIR_TERM_BRANCH:
        lowered->as.branch_term.true_block = terminator->as.branch_term.true_block;
        lowered->as.branch_term.false_block = terminator->as.branch_term.false_block;
        return bytecode_value_from_mir_value(context,
                                             terminator->as.branch_term.condition,
                                             &lowered->as.branch_term.condition);
    case MIR_TERM_THROW:
        return bytecode_value_from_mir_value(context,
                                             terminator->as.throw_term.value,
                                             &lowered->as.throw_term.value);
    }

    return false;
}

static bool lower_unit(BytecodeBuildContext *context,
                       const MirUnit *mir_unit,
                       BytecodeUnit *unit) {
    size_t local_index;
    size_t block_index;

    if (!context || !mir_unit || !unit) {
        return false;
    }

    memset(unit, 0, sizeof(*unit));
    unit->kind = unit_kind_from_mir_kind(mir_unit->kind);
    unit->name = ast_copy_text(mir_unit->name);
    unit->symbol = mir_unit->symbol;
    unit->return_type = mir_unit->return_type;
    unit->local_count = mir_unit->local_count;
    unit->parameter_count = mir_unit->parameter_count;
    unit->temp_count = mir_unit->next_temp_index;
    unit->block_count = mir_unit->block_count;
    if (!unit->name) {
        bytecode_set_error(context,
                           (AstSourceSpan){0},
                           NULL,
                           "Out of memory while naming bytecode unit.");
        return false;
    }

    if (unit->local_count > 0) {
        unit->locals = calloc(unit->local_count, sizeof(*unit->locals));
        if (!unit->locals) {
            bytecode_set_error(context,
                               (AstSourceSpan){0},
                               NULL,
                               "Out of memory while allocating bytecode locals.");
            return false;
        }
    }
    for (local_index = 0; local_index < unit->local_count; local_index++) {
        unit->locals[local_index].kind = local_kind_from_mir_kind(mir_unit->locals[local_index].kind);
        unit->locals[local_index].name = ast_copy_text(mir_unit->locals[local_index].name);
        unit->locals[local_index].symbol = mir_unit->locals[local_index].symbol;
        unit->locals[local_index].type = mir_unit->locals[local_index].type;
        unit->locals[local_index].is_final = mir_unit->locals[local_index].is_final;
        unit->locals[local_index].index = mir_unit->locals[local_index].index;
        if (!unit->locals[local_index].name) {
            bytecode_set_error(context,
                               (AstSourceSpan){0},
                               NULL,
                               "Out of memory while naming bytecode local.");
            return false;
        }
    }

    if (unit->block_count > 0) {
        unit->blocks = calloc(unit->block_count, sizeof(*unit->blocks));
        if (!unit->blocks) {
            bytecode_set_error(context,
                               (AstSourceSpan){0},
                               NULL,
                               "Out of memory while allocating bytecode blocks.");
            return false;
        }
    }
    for (block_index = 0; block_index < unit->block_count; block_index++) {
        const MirBasicBlock *mir_block = &mir_unit->blocks[block_index];
        BytecodeBasicBlock *block = &unit->blocks[block_index];
        size_t instruction_index;

        block->label = ast_copy_text(mir_block->label);
        block->instruction_count = mir_block->instruction_count;
        if (!block->label) {
            bytecode_set_error(context,
                               (AstSourceSpan){0},
                               NULL,
                               "Out of memory while naming bytecode block.");
            return false;
        }
        if (block->instruction_count > 0) {
            block->instructions = calloc(block->instruction_count, sizeof(*block->instructions));
            if (!block->instructions) {
                bytecode_set_error(context,
                                   (AstSourceSpan){0},
                                   NULL,
                                   "Out of memory while allocating bytecode instructions.");
                return false;
            }
        }
        for (instruction_index = 0; instruction_index < block->instruction_count; instruction_index++) {
            if (!lower_instruction(context,
                                   &mir_block->instructions[instruction_index],
                                   &block->instructions[instruction_index])) {
                return false;
            }
        }
        if (!lower_terminator(context, &mir_block->terminator, &block->terminator)) {
            return false;
        }
    }

    return true;
}