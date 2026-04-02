#include "machine.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    MachineProgram      *program;
    const LirProgram    *lir_program;
    const CodegenProgram *codegen_program;
} MachineBuildContext;

static bool reserve_items(void **items, size_t *capacity,
                          size_t needed, size_t item_size);
static bool source_span_is_valid(AstSourceSpan span);
static void machine_set_error(MachineBuildContext *context,
                              AstSourceSpan primary_span,
                              const AstSourceSpan *related_span,
                              const char *format,
                              ...);
static void machine_block_free(MachineBlock *block);
static void machine_unit_free(MachineUnit *unit);
static char *copy_format(const char *format, ...);
static bool append_line(MachineBuildContext *context,
                        MachineBlock *block,
                        const char *format,
                        ...);
static const char *unit_kind_name(LirUnitKind kind);
static bool checked_type_is_unsigned(CheckedType type);
static bool unit_uses_register(const CodegenUnit *unit, CodegenRegister reg);
static size_t compute_helper_slot_count(const LirUnit *lir_unit,
                                        const CodegenUnit *codegen_unit);
static size_t compute_outgoing_stack_slot_count(const LirUnit *lir_unit,
                                                const CodegenUnit *codegen_unit);
static size_t unit_stack_word_count(const MachineUnit *unit);
static bool format_slot_operand(const CodegenUnit *codegen_unit,
                                size_t slot_index,
                                char **text);
static bool format_vreg_operand(const CodegenUnit *codegen_unit,
                                size_t vreg_index,
                                char **text);
static bool format_literal_operand(LirOperand operand, char **text);
static bool format_operand(const LirUnit *lir_unit,
                           const CodegenUnit *codegen_unit,
                           LirOperand operand,
                           char **text);
static bool format_capture_operand(size_t capture_index, char **text);
static bool format_helper_slot_operand(size_t helper_slot_index, char **text);
static bool format_incoming_arg_stack_operand(size_t argument_index, char **text);
static bool format_outgoing_arg_stack_operand(size_t argument_index, char **text);
static bool format_member_symbol_operand(const char *member, char **text);
static bool format_code_label_operand(const char *unit_name, char **text);
static bool format_template_text_operand(const char *text, char **formatted);
static bool emit_move_to_destination(MachineBuildContext *context,
                                     MachineBlock *block,
                                     const char *destination,
                                     const char *source);
static bool emit_store_vreg(MachineBuildContext *context,
                            const CodegenUnit *codegen_unit,
                            size_t vreg_index,
                            MachineBlock *block,
                            const char *source);
static bool emit_preserve_before_call(MachineBuildContext *context,
                                      const CodegenUnit *codegen_unit,
                                      MachineBlock *block);
static bool emit_preserve_after_call(MachineBuildContext *context,
                                     const CodegenUnit *codegen_unit,
                                     MachineBlock *block);
static bool emit_entry_prologue(MachineBuildContext *context,
                                const CodegenUnit *codegen_unit,
                                MachineUnit *machine_unit,
                                MachineBlock *block);
static bool emit_return_epilogue(MachineBuildContext *context,
                                 const CodegenUnit *codegen_unit,
                                 const MachineUnit *machine_unit,
                                 MachineBlock *block);
static const LirInstruction *find_pending_call_instruction(const LirBasicBlock *block,
                                                           size_t instruction_index,
                                                           size_t *call_index_out);
static const CodegenSelectedInstruction *find_pending_call_selection(const CodegenBlock *block,
                                                                     size_t instruction_index,
                                                                     size_t *call_index_out);
static bool emit_binary(MachineBuildContext *context,
                        const LirUnit *lir_unit,
                        const CodegenUnit *codegen_unit,
                        const LirInstruction *instruction,
                        MachineBlock *block);
static bool emit_unary(MachineBuildContext *context,
                       const LirUnit *lir_unit,
                       const CodegenUnit *codegen_unit,
                       const LirInstruction *instruction,
                       MachineBlock *block);
static bool emit_cast(MachineBuildContext *context,
                      const LirUnit *lir_unit,
                      const CodegenUnit *codegen_unit,
                      const LirInstruction *instruction,
                      MachineBlock *block);
static bool emit_call_global(MachineBuildContext *context,
                             const LirUnit *lir_unit,
                             const CodegenUnit *codegen_unit,
                             const LirInstruction *instruction,
                             MachineBlock *block);
static bool emit_runtime_helper_call(MachineBuildContext *context,
                                     const CodegenUnit *codegen_unit,
                                     const RuntimeAbiHelperSignature *signature,
                                     MachineBlock *block,
                                     bool is_noreturn);
static bool emit_instruction(MachineBuildContext *context,
                             const LirUnit *lir_unit,
                             const CodegenUnit *codegen_unit,
                             const LirBasicBlock *lir_block,
                             const CodegenBlock *codegen_block,
                             size_t instruction_index,
                             MachineBlock *block);
static bool emit_terminator(MachineBuildContext *context,
                            const LirUnit *lir_unit,
                            const CodegenUnit *codegen_unit,
                            const LirBasicBlock *lir_block,
                            const MachineUnit *machine_unit,
                            MachineBlock *block);
static bool build_unit(MachineBuildContext *context,
                       const LirUnit *lir_unit,
                       const CodegenUnit *codegen_unit,
                       MachineUnit *machine_unit);

void machine_program_init(MachineProgram *program) {
    if (!program) {
        return;
    }

    memset(program, 0, sizeof(*program));
    program->target = CODEGEN_TARGET_X86_64_SYSV_ELF;
}

void machine_program_free(MachineProgram *program) {
    size_t i;

    if (!program) {
        return;
    }

    for (i = 0; i < program->unit_count; i++) {
        machine_unit_free(&program->units[i]);
    }
    free(program->units);
    memset(program, 0, sizeof(*program));
}

const MachineBuildError *machine_get_error(const MachineProgram *program) {
    if (!program || !program->has_error) {
        return NULL;
    }

    return &program->error;
}

bool machine_format_error(const MachineBuildError *error,
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

bool machine_build_program(MachineProgram *program,
                          const LirProgram *lir_program,
                          const CodegenProgram *codegen_program) {
    MachineBuildContext context;
    size_t i;

    if (!program || !lir_program || !codegen_program) {
        return false;
    }

    machine_program_free(program);
    machine_program_init(program);

    memset(&context, 0, sizeof(context));
    context.program = program;
    context.lir_program = lir_program;
    context.codegen_program = codegen_program;

    if (lir_get_error(lir_program) != NULL) {
        machine_set_error(&context,
                          (AstSourceSpan){0},
                          NULL,
                          "Cannot emit machine instructions while the LIR reports errors.");
        return false;
    }
    if (codegen_get_error(codegen_program) != NULL) {
        machine_set_error(&context,
                          (AstSourceSpan){0},
                          NULL,
                          "Cannot emit machine instructions while the codegen plan reports errors.");
        return false;
    }
    if (codegen_program->target != CODEGEN_TARGET_X86_64_SYSV_ELF) {
        machine_set_error(&context,
                          (AstSourceSpan){0},
                          NULL,
                          "Unsupported machine emission target %d.",
                          codegen_program->target);
        return false;
    }
    if (lir_program->unit_count != codegen_program->unit_count) {
        machine_set_error(&context,
                          (AstSourceSpan){0},
                          NULL,
                          "Machine emission requires matching LIR/codegen unit counts.");
        return false;
    }

    program->target = codegen_program->target;
    program->unit_count = lir_program->unit_count;
    if (program->unit_count > 0) {
        program->units = calloc(program->unit_count, sizeof(*program->units));
        if (!program->units) {
            machine_set_error(&context,
                              (AstSourceSpan){0},
                              NULL,
                              "Out of memory while allocating machine units.");
            return false;
        }
    }

    for (i = 0; i < program->unit_count; i++) {
        if (!build_unit(&context,
                        &lir_program->units[i],
                        &codegen_program->units[i],
                        &program->units[i])) {
            return false;
        }
    }

    return true;
}

bool machine_dump_program(FILE *out, const MachineProgram *program) {
    size_t i;
    size_t j;

    if (!out || !program) {
        return false;
    }

    fprintf(out,
            "MachineProgram target=%s scratch=%s\n",
            codegen_target_name(program->target),
            codegen_register_name(CODEGEN_REG_R14));
    if (!runtime_abi_dump_surface(out, program->target)) {
        return false;
    }
    for (i = 0; i < program->unit_count; i++) {
        const MachineUnit *unit = &program->units[i];

        fprintf(out,
                "  Unit name=%s kind=%s return=",
                unit->name,
                unit_kind_name(unit->kind));
        {
            char type_buffer[64];
            if (!checked_type_to_string(unit->return_type, type_buffer, sizeof(type_buffer))) {
                return false;
            }
            if (fputs(type_buffer, out) == EOF) {
                return false;
            }
        }
        fprintf(out,
                " frame_slots=%zu spills=%zu helper_slots=%zu outgoing_stack=%zu blocks=%zu\n",
                unit->frame_slot_count,
                unit->spill_slot_count,
                unit->helper_slot_count,
                unit->outgoing_stack_slot_count,
                unit->block_count);

        fprintf(out, "    Blocks:\n");
        for (j = 0; j < unit->block_count; j++) {
            size_t k;

            fprintf(out, "      Block %s:\n", unit->blocks[j].label);
            for (k = 0; k < unit->blocks[j].instruction_count; k++) {
                fprintf(out, "        %s\n", unit->blocks[j].instructions[k].text);
            }
        }
    }

    return !ferror(out);
}

char *machine_dump_program_to_string(const MachineProgram *program) {
    FILE *stream;
    char *buffer;
    long size;
    size_t read_size;

    if (!program) {
        return NULL;
    }

    stream = tmpfile();
    if (!stream) {
        return NULL;
    }

    if (!machine_dump_program(stream, program) || fflush(stream) != 0 ||
        fseek(stream, 0, SEEK_END) != 0) {
        fclose(stream);
        return NULL;
    }

    size = ftell(stream);
    if (size < 0 || fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream);
        return NULL;
    }

    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(stream);
        return NULL;
    }

    read_size = fread(buffer, 1, (size_t)size, stream);
    fclose(stream);
    if (read_size != (size_t)size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}

static bool reserve_items(void **items, size_t *capacity,
                          size_t needed, size_t item_size) {
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

static void machine_set_error(MachineBuildContext *context,
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

static void machine_block_free(MachineBlock *block) {
    size_t i;

    if (!block) {
        return;
    }

    free(block->label);
    for (i = 0; i < block->instruction_count; i++) {
        free(block->instructions[i].text);
    }
    free(block->instructions);
    memset(block, 0, sizeof(*block));
}

static void machine_unit_free(MachineUnit *unit) {
    size_t i;

    if (!unit) {
        return;
    }

    free(unit->name);
    for (i = 0; i < unit->frame_slot_count; i++) {
        free(unit->frame_slots[i].name);
    }
    free(unit->frame_slots);
    for (i = 0; i < unit->block_count; i++) {
        machine_block_free(&unit->blocks[i]);
    }
    free(unit->blocks);
    memset(unit, 0, sizeof(*unit));
}

static char *copy_format(const char *format, ...) {
    va_list args;
    va_list args_copy;
    int needed;
    char *buffer;

    if (!format) {
        return NULL;
    }

    va_start(args, format);
    va_copy(args_copy, args);
    needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    if (needed < 0) {
        va_end(args);
        return NULL;
    }

    buffer = malloc((size_t)needed + 1);
    if (!buffer) {
        va_end(args);
        return NULL;
    }

    vsnprintf(buffer, (size_t)needed + 1, format, args);
    va_end(args);
    return buffer;
}

static bool append_line(MachineBuildContext *context,
                        MachineBlock *block,
                        const char *format,
                        ...) {
    va_list args;
    va_list args_copy;
    int needed;
    char *buffer;

    if (!block || !format) {
        return false;
    }

    if (!reserve_items((void **)&block->instructions,
                       &block->instruction_capacity,
                       block->instruction_count + 1,
                       sizeof(*block->instructions))) {
        machine_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while appending machine instructions.");
        return false;
    }

    va_start(args, format);
    va_copy(args_copy, args);
    needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    if (needed < 0) {
        va_end(args);
        machine_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Failed to format a machine instruction.");
        return false;
    }

    buffer = malloc((size_t)needed + 1);
    if (!buffer) {
        va_end(args);
        machine_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while formatting a machine instruction.");
        return false;
    }
    vsnprintf(buffer, (size_t)needed + 1, format, args);
    va_end(args);

    block->instructions[block->instruction_count].text = buffer;
    block->instruction_count++;
    return true;
}

static const char *unit_kind_name(LirUnitKind kind) {
    switch (kind) {
    case LIR_UNIT_START:
        return "start";
    case LIR_UNIT_BINDING:
        return "binding";
    case LIR_UNIT_INIT:
        return "init";
    case LIR_UNIT_LAMBDA:
        return "lambda";
    }

    return "unknown";
}

static bool checked_type_is_unsigned(CheckedType type) {
    if (type.kind != CHECKED_TYPE_VALUE || type.array_depth != 0) {
        return false;
    }

    switch (type.primitive) {
    case AST_PRIMITIVE_UINT8:
    case AST_PRIMITIVE_UINT16:
    case AST_PRIMITIVE_UINT32:
    case AST_PRIMITIVE_UINT64:
    case AST_PRIMITIVE_BOOL:
    case AST_PRIMITIVE_CHAR:
        return true;
    default:
        return false;
    }
}

static bool unit_uses_register(const CodegenUnit *unit, CodegenRegister reg) {
    size_t i;

    if (!unit) {
        return false;
    }

    for (i = 0; i < unit->vreg_count; i++) {
        if (unit->vreg_allocations[i].location.kind == CODEGEN_VREG_REGISTER &&
            unit->vreg_allocations[i].location.as.reg == reg) {
            return true;
        }
    }

    return false;
}

static size_t compute_helper_slot_count(const LirUnit *lir_unit,
                                        const CodegenUnit *codegen_unit) {
    size_t block_index;
    size_t max_slots = 0;

    if (!lir_unit || !codegen_unit) {
        return 0;
    }

    for (block_index = 0; block_index < lir_unit->block_count; block_index++) {
        size_t instruction_index;

        for (instruction_index = 0;
             instruction_index < lir_unit->blocks[block_index].instruction_count;
             instruction_index++) {
            const LirInstruction *instruction = &lir_unit->blocks[block_index].instructions[instruction_index];
            const CodegenSelectedInstruction *selected = &codegen_unit->blocks[block_index].instructions[instruction_index];
            size_t needed = 0;

            if (selected->selection.kind == CODEGEN_SELECTION_RUNTIME) {
                switch (selected->selection.as.runtime_helper) {
                case CODEGEN_RUNTIME_CLOSURE_NEW:
                    needed = instruction->as.closure.capture_count;
                    break;
                case CODEGEN_RUNTIME_CALL_CALLABLE:
                    needed = instruction->as.call.argument_count;
                    break;
                case CODEGEN_RUNTIME_ARRAY_LITERAL:
                    needed = instruction->as.array_literal.element_count;
                    break;
                case CODEGEN_RUNTIME_TEMPLATE_BUILD:
                    needed = instruction->as.template_literal.part_count * 2;
                    break;
                case CODEGEN_RUNTIME_MEMBER_LOAD:
                case CODEGEN_RUNTIME_INDEX_LOAD:
                case CODEGEN_RUNTIME_STORE_INDEX:
                case CODEGEN_RUNTIME_STORE_MEMBER:
                case CODEGEN_RUNTIME_THROW:
                case CODEGEN_RUNTIME_CAST_VALUE:
                    needed = 0;
                    break;
                }
            }
            if (needed > max_slots) {
                max_slots = needed;
            }
        }
    }

    return max_slots;
}

static size_t compute_outgoing_stack_slot_count(const LirUnit *lir_unit,
                                                const CodegenUnit *codegen_unit) {
    size_t block_index;
    size_t max_slots = 0;

    if (!lir_unit || !codegen_unit) {
        return 0;
    }

    for (block_index = 0; block_index < lir_unit->block_count; block_index++) {
        size_t instruction_index;

        for (instruction_index = 0;
             instruction_index < lir_unit->blocks[block_index].instruction_count;
             instruction_index++) {
            const LirInstruction *instruction = &lir_unit->blocks[block_index].instructions[instruction_index];
            const CodegenSelectedInstruction *selected = &codegen_unit->blocks[block_index].instructions[instruction_index];

            if (instruction->kind == LIR_INSTR_CALL &&
                selected->selection.kind == CODEGEN_SELECTION_DIRECT &&
                selected->selection.as.direct_pattern == CODEGEN_DIRECT_CALL_GLOBAL &&
                instruction->as.call.argument_count > 6) {
                size_t needed = instruction->as.call.argument_count - 6;
                if (needed > max_slots) {
                    max_slots = needed;
                }
            }
        }
    }

    return max_slots;
}

static size_t unit_stack_word_count(const MachineUnit *unit) {
    return unit->frame_slot_count + unit->spill_slot_count + unit->helper_slot_count +
           unit->outgoing_stack_slot_count;
}

static bool format_slot_operand(const CodegenUnit *codegen_unit,
                                size_t slot_index,
                                char **text) {
    if (!codegen_unit || !text || slot_index >= codegen_unit->frame_slot_count) {
        return false;
    }

    *text = copy_format("frame(%s)", codegen_unit->frame_slots[slot_index].name);
    return *text != NULL;
}

static bool format_vreg_operand(const CodegenUnit *codegen_unit,
                                size_t vreg_index,
                                char **text) {
    const CodegenVRegAllocation *allocation;

    if (!codegen_unit || !text || vreg_index >= codegen_unit->vreg_count) {
        return false;
    }

    allocation = &codegen_unit->vreg_allocations[vreg_index];
    if (allocation->location.kind == CODEGEN_VREG_REGISTER) {
        *text = ast_copy_text(codegen_register_name(allocation->location.as.reg));
    } else {
        *text = copy_format("spill(%zu)", allocation->location.as.spill_slot_index);
    }
    return *text != NULL;
}

static bool format_literal_operand(LirOperand operand, char **text) {
    char type_name[64];

    if (!text || operand.kind != LIR_OPERAND_LITERAL) {
        return false;
    }

    switch (operand.as.literal.kind) {
    case AST_LITERAL_BOOL:
        *text = copy_format("bool(%s)", operand.as.literal.bool_value ? "true" : "false");
        return *text != NULL;
    case AST_LITERAL_NULL:
        *text = ast_copy_text("null");
        return *text != NULL;
    default:
        if (!checked_type_to_string(operand.type, type_name, sizeof(type_name))) {
            return false;
        }
        *text = copy_format("%s(%s)", type_name, operand.as.literal.text ? operand.as.literal.text : "\"\"");
        if (operand.as.literal.kind == AST_LITERAL_INTEGER || operand.as.literal.kind == AST_LITERAL_FLOAT) {
            free(*text);
            *text = copy_format("%s(%s)", type_name, operand.as.literal.text ? operand.as.literal.text : "0");
        }
        return *text != NULL;
    }
}

static bool format_operand(const LirUnit *lir_unit,
                           const CodegenUnit *codegen_unit,
                           LirOperand operand,
                           char **text) {
    (void)lir_unit;

    if (!text) {
        return false;
    }

    switch (operand.kind) {
    case LIR_OPERAND_INVALID:
        *text = ast_copy_text("<invalid>");
        return *text != NULL;
    case LIR_OPERAND_VREG:
        return format_vreg_operand(codegen_unit, operand.as.vreg_index, text);
    case LIR_OPERAND_SLOT:
        return format_slot_operand(codegen_unit, operand.as.slot_index, text);
    case LIR_OPERAND_GLOBAL:
        *text = copy_format("global(%s)", operand.as.global_name);
        return *text != NULL;
    case LIR_OPERAND_LITERAL:
        return format_literal_operand(operand, text);
    }

    return false;
}

static bool format_capture_operand(size_t capture_index, char **text) {
    if (!text) {
        return false;
    }

    *text = copy_format("env(%zu)", capture_index);
    return *text != NULL;
}

static bool format_helper_slot_operand(size_t helper_slot_index, char **text) {
    if (!text) {
        return false;
    }

    *text = copy_format("helper(%zu)", helper_slot_index);
    return *text != NULL;
}

static bool format_incoming_arg_stack_operand(size_t argument_index, char **text) {
    if (!text) {
        return false;
    }

    *text = copy_format("argin(%zu)", argument_index);
    return *text != NULL;
}

static bool format_outgoing_arg_stack_operand(size_t argument_index, char **text) {
    if (!text) {
        return false;
    }

    *text = copy_format("argout(%zu)", argument_index);
    return *text != NULL;
}

static bool format_member_symbol_operand(const char *member, char **text) {
    if (!text) {
        return false;
    }

    *text = copy_format("symbol(%s)", member ? member : "");
    return *text != NULL;
}

static bool format_code_label_operand(const char *unit_name, char **text) {
    if (!text) {
        return false;
    }

    *text = copy_format("code(%s)", unit_name ? unit_name : "");
    return *text != NULL;
}

static bool format_template_text_operand(const char *text, char **formatted) {
    if (!formatted) {
        return false;
    }

    *formatted = copy_format("text(\"%s\")", text ? text : "");
    return *formatted != NULL;
}

static bool emit_move_to_destination(MachineBuildContext *context,
                                     MachineBlock *block,
                                     const char *destination,
                                     const char *source) {
    return append_line(context, block, "mov %s, %s", destination, source);
}

static bool emit_store_vreg(MachineBuildContext *context,
                            const CodegenUnit *codegen_unit,
                            size_t vreg_index,
                            MachineBlock *block,
                            const char *source) {
    const CodegenVRegAllocation *allocation;

    if (!codegen_unit || vreg_index >= codegen_unit->vreg_count) {
        machine_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Machine emission missing virtual-register allocation %zu.",
                          vreg_index);
        return false;
    }

    allocation = &codegen_unit->vreg_allocations[vreg_index];
    if (allocation->location.kind == CODEGEN_VREG_REGISTER) {
        if (strcmp(codegen_register_name(allocation->location.as.reg), source) == 0) {
            return true;
        }
        return append_line(context,
                           block,
                           "mov %s, %s",
                           codegen_register_name(allocation->location.as.reg),
                           source);
    }

    if (strcmp(codegen_register_name(CODEGEN_REG_R14), source) == 0) {
        return append_line(context,
                           block,
                           "mov spill(%zu), %s",
                           allocation->location.as.spill_slot_index,
                           codegen_register_name(CODEGEN_REG_R14));
    }

    return append_line(context, block, "mov %s, %s", codegen_register_name(CODEGEN_REG_R14), source) &&
           append_line(context,
                       block,
                       "mov spill(%zu), %s",
                       allocation->location.as.spill_slot_index,
                       codegen_register_name(CODEGEN_REG_R14));
}

static bool emit_preserve_before_call(MachineBuildContext *context,
                                      const CodegenUnit *codegen_unit,
                                      MachineBlock *block) {
    if (unit_uses_register(codegen_unit, CODEGEN_REG_R10) &&
        !append_line(context, block, "push %s", codegen_register_name(CODEGEN_REG_R10))) {
        return false;
    }
    if (unit_uses_register(codegen_unit, CODEGEN_REG_R11) &&
        !append_line(context, block, "push %s", codegen_register_name(CODEGEN_REG_R11))) {
        return false;
    }
    return true;
}

static bool emit_preserve_after_call(MachineBuildContext *context,
                                     const CodegenUnit *codegen_unit,
                                     MachineBlock *block) {
    if (unit_uses_register(codegen_unit, CODEGEN_REG_R11) &&
        !append_line(context, block, "pop %s", codegen_register_name(CODEGEN_REG_R11))) {
        return false;
    }
    if (unit_uses_register(codegen_unit, CODEGEN_REG_R10) &&
        !append_line(context, block, "pop %s", codegen_register_name(CODEGEN_REG_R10))) {
        return false;
    }
    return true;
}

static bool emit_entry_prologue(MachineBuildContext *context,
                                const CodegenUnit *codegen_unit,
                                MachineUnit *machine_unit,
                                MachineBlock *block) {
    size_t stack_words;

    stack_words = unit_stack_word_count(machine_unit);
    if (!append_line(context, block, "push rbp") ||
        !append_line(context, block, "mov rbp, rsp") ||
        !append_line(context, block, "push %s", codegen_register_name(CODEGEN_REG_R14))) {
        return false;
    }
    if (unit_uses_register(codegen_unit, CODEGEN_REG_R12) &&
        !append_line(context, block, "push %s", codegen_register_name(CODEGEN_REG_R12))) {
        return false;
    }
    if (unit_uses_register(codegen_unit, CODEGEN_REG_R13) &&
        !append_line(context, block, "push %s", codegen_register_name(CODEGEN_REG_R13))) {
        return false;
    }
    if (stack_words > 0 &&
        !append_line(context, block, "sub rsp, %zu", stack_words * 8)) {
        return false;
    }
    return true;
}

static bool emit_return_epilogue(MachineBuildContext *context,
                                 const CodegenUnit *codegen_unit,
                                 const MachineUnit *machine_unit,
                                 MachineBlock *block) {
    size_t stack_words;

    stack_words = unit_stack_word_count(machine_unit);
    if (stack_words > 0 &&
        !append_line(context, block, "add rsp, %zu", stack_words * 8)) {
        return false;
    }
    if (unit_uses_register(codegen_unit, CODEGEN_REG_R13) &&
        !append_line(context, block, "pop %s", codegen_register_name(CODEGEN_REG_R13))) {
        return false;
    }
    if (unit_uses_register(codegen_unit, CODEGEN_REG_R12) &&
        !append_line(context, block, "pop %s", codegen_register_name(CODEGEN_REG_R12))) {
        return false;
    }
    if (!append_line(context, block, "pop %s", codegen_register_name(CODEGEN_REG_R14)) ||
        !append_line(context, block, "pop rbp") ||
        !append_line(context, block, "ret")) {
        return false;
    }
    return true;
}

static const LirInstruction *find_pending_call_instruction(const LirBasicBlock *block,
                                                           size_t instruction_index,
                                                           size_t *call_index_out) {
    size_t index;

    if (!block) {
        return NULL;
    }

    for (index = instruction_index + 1; index < block->instruction_count; index++) {
        if (block->instructions[index].kind == LIR_INSTR_OUTGOING_ARG) {
            continue;
        }
        if (block->instructions[index].kind == LIR_INSTR_CALL) {
            if (call_index_out) {
                *call_index_out = index;
            }
            return &block->instructions[index];
        }
        break;
    }

    return NULL;
}

static const CodegenSelectedInstruction *find_pending_call_selection(const CodegenBlock *block,
                                                                     size_t instruction_index,
                                                                     size_t *call_index_out) {
    size_t index;

    if (!block) {
        return NULL;
    }

    for (index = instruction_index + 1; index < block->instruction_count; index++) {
        if (block->instructions[index].kind == LIR_INSTR_OUTGOING_ARG) {
            continue;
        }
        if (block->instructions[index].kind == LIR_INSTR_CALL) {
            if (call_index_out) {
                *call_index_out = index;
            }
            return &block->instructions[index];
        }
        break;
    }

    return NULL;
}

static bool emit_binary(MachineBuildContext *context,
                        const LirUnit *lir_unit,
                        const CodegenUnit *codegen_unit,
                        const LirInstruction *instruction,
                        MachineBlock *block) {
    char *left = NULL;
    char *right = NULL;
    const CodegenVRegAllocation *allocation;
    const char *work_reg;
    bool is_unsigned;
    bool ok = false;

    if (!format_operand(lir_unit, codegen_unit, instruction->as.binary.left, &left) ||
        !format_operand(lir_unit, codegen_unit, instruction->as.binary.right, &right)) {
        goto cleanup;
    }
    if (instruction->as.binary.dest_vreg >= codegen_unit->vreg_count) {
        machine_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Machine emission missing destination virtual register %zu.",
                          instruction->as.binary.dest_vreg);
        goto cleanup;
    }

    allocation = &codegen_unit->vreg_allocations[instruction->as.binary.dest_vreg];
    work_reg = allocation->location.kind == CODEGEN_VREG_REGISTER
        ? codegen_register_name(allocation->location.as.reg)
        : codegen_register_name(CODEGEN_REG_R14);
    is_unsigned = checked_type_is_unsigned(instruction->as.binary.left.type);

    switch (instruction->as.binary.operator) {
    case AST_BINARY_OP_ADD:
        ok = append_line(context, block, "mov %s, %s", work_reg, left) &&
             append_line(context, block, "add %s, %s", work_reg, right);
        break;
    case AST_BINARY_OP_SUBTRACT:
        ok = append_line(context, block, "mov %s, %s", work_reg, left) &&
             append_line(context, block, "sub %s, %s", work_reg, right);
        break;
    case AST_BINARY_OP_MULTIPLY:
        ok = append_line(context, block, "mov %s, %s", work_reg, left) &&
             append_line(context, block, "imul %s, %s", work_reg, right);
        break;
    case AST_BINARY_OP_DIVIDE:
    case AST_BINARY_OP_MODULO:
        ok = append_line(context, block, "mov rax, %s", left) &&
             (is_unsigned
                  ? append_line(context, block, "xor rdx, rdx")
                  : append_line(context, block, "cqo")) &&
             append_line(context,
                         block,
                         "%s %s",
                         is_unsigned ? "div" : "idiv",
                         right) &&
             append_line(context,
                         block,
                         "mov %s, %s",
                         work_reg,
                         instruction->as.binary.operator == AST_BINARY_OP_DIVIDE ? "rax" : "rdx");
        break;
    case AST_BINARY_OP_BIT_AND:
        ok = append_line(context, block, "mov %s, %s", work_reg, left) &&
             append_line(context, block, "and %s, %s", work_reg, right);
        break;
    case AST_BINARY_OP_BIT_OR:
        ok = append_line(context, block, "mov %s, %s", work_reg, left) &&
             append_line(context, block, "or %s, %s", work_reg, right);
        break;
    case AST_BINARY_OP_BIT_XOR:
        ok = append_line(context, block, "mov %s, %s", work_reg, left) &&
             append_line(context, block, "xor %s, %s", work_reg, right);
        break;
    case AST_BINARY_OP_BIT_NAND:
        ok = append_line(context, block, "mov %s, %s", work_reg, left) &&
             append_line(context, block, "and %s, %s", work_reg, right) &&
             append_line(context, block, "not %s", work_reg);
        break;
    case AST_BINARY_OP_BIT_XNOR:
        ok = append_line(context, block, "mov %s, %s", work_reg, left) &&
             append_line(context, block, "xor %s, %s", work_reg, right) &&
             append_line(context, block, "not %s", work_reg);
        break;
    case AST_BINARY_OP_SHIFT_LEFT:
        ok = append_line(context, block, "mov %s, %s", work_reg, left) &&
             append_line(context, block, "shl %s, %s", work_reg, right);
        break;
    case AST_BINARY_OP_SHIFT_RIGHT:
        ok = append_line(context, block, "mov %s, %s", work_reg, left) &&
             append_line(context,
                         block,
                         "%s %s, %s",
                         is_unsigned ? "shr" : "sar",
                         work_reg,
                         right);
        break;
    case AST_BINARY_OP_EQUAL:
        ok = append_line(context, block, "mov %s, %s", work_reg, left) &&
             append_line(context, block, "cmp %s, %s", work_reg, right) &&
             append_line(context, block, "sete %s", work_reg);
        break;
    case AST_BINARY_OP_NOT_EQUAL:
        ok = append_line(context, block, "mov %s, %s", work_reg, left) &&
             append_line(context, block, "cmp %s, %s", work_reg, right) &&
             append_line(context, block, "setne %s", work_reg);
        break;
    case AST_BINARY_OP_LESS:
        ok = append_line(context, block, "mov %s, %s", work_reg, left) &&
             append_line(context, block, "cmp %s, %s", work_reg, right) &&
             append_line(context, block, "%s %s", is_unsigned ? "setb" : "setl", work_reg);
        break;
    case AST_BINARY_OP_GREATER:
        ok = append_line(context, block, "mov %s, %s", work_reg, left) &&
             append_line(context, block, "cmp %s, %s", work_reg, right) &&
             append_line(context, block, "%s %s", is_unsigned ? "seta" : "setg", work_reg);
        break;
    case AST_BINARY_OP_LESS_EQUAL:
        ok = append_line(context, block, "mov %s, %s", work_reg, left) &&
             append_line(context, block, "cmp %s, %s", work_reg, right) &&
             append_line(context, block, "%s %s", is_unsigned ? "setbe" : "setle", work_reg);
        break;
    case AST_BINARY_OP_GREATER_EQUAL:
        ok = append_line(context, block, "mov %s, %s", work_reg, left) &&
             append_line(context, block, "cmp %s, %s", work_reg, right) &&
             append_line(context, block, "%s %s", is_unsigned ? "setae" : "setge", work_reg);
        break;
    case AST_BINARY_OP_LOGICAL_AND:
    case AST_BINARY_OP_LOGICAL_OR:
        machine_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Logical short-circuit operators should not reach direct binary machine emission.");
        goto cleanup;
    }

    if (ok) {
        ok = emit_store_vreg(context,
                             codegen_unit,
                             instruction->as.binary.dest_vreg,
                             block,
                             work_reg);
    }

cleanup:
    free(left);
    free(right);
    return ok;
}

static bool emit_unary(MachineBuildContext *context,
                       const LirUnit *lir_unit,
                       const CodegenUnit *codegen_unit,
                       const LirInstruction *instruction,
                       MachineBlock *block) {
    char *operand = NULL;
    const CodegenVRegAllocation *allocation;
    const char *work_reg;
    bool ok = false;

    if (!format_operand(lir_unit, codegen_unit, instruction->as.unary.operand, &operand)) {
        goto cleanup;
    }
    if (instruction->as.unary.dest_vreg >= codegen_unit->vreg_count) {
        machine_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Machine emission missing unary destination virtual register %zu.",
                          instruction->as.unary.dest_vreg);
        goto cleanup;
    }

    allocation = &codegen_unit->vreg_allocations[instruction->as.unary.dest_vreg];
    work_reg = allocation->location.kind == CODEGEN_VREG_REGISTER
        ? codegen_register_name(allocation->location.as.reg)
        : codegen_register_name(CODEGEN_REG_R14);

    ok = append_line(context, block, "mov %s, %s", work_reg, operand);
    if (!ok) {
        goto cleanup;
    }

    switch (instruction->as.unary.operator) {
    case AST_UNARY_OP_PLUS:
        break;
    case AST_UNARY_OP_NEGATE:
        ok = append_line(context, block, "neg %s", work_reg);
        break;
    case AST_UNARY_OP_BITWISE_NOT:
        ok = append_line(context, block, "not %s", work_reg);
        break;
    case AST_UNARY_OP_LOGICAL_NOT:
        ok = append_line(context, block, "cmp %s, bool(false)", work_reg) &&
             append_line(context, block, "sete %s", work_reg);
        break;
    }

    if (ok) {
        ok = emit_store_vreg(context,
                             codegen_unit,
                             instruction->as.unary.dest_vreg,
                             block,
                             work_reg);
    }

cleanup:
    free(operand);
    return ok;
}

static bool emit_cast(MachineBuildContext *context,
                      const LirUnit *lir_unit,
                      const CodegenUnit *codegen_unit,
                      const LirInstruction *instruction,
                      MachineBlock *block) {
    char *operand = NULL;
    bool ok = false;

    if (!format_operand(lir_unit, codegen_unit, instruction->as.cast.operand, &operand)) {
        goto cleanup;
    }
    ok = emit_store_vreg(context,
                         codegen_unit,
                         instruction->as.cast.dest_vreg,
                         block,
                         operand);

cleanup:
    free(operand);
    return ok;
}

static bool emit_call_global(MachineBuildContext *context,
                             const LirUnit *lir_unit,
                             const CodegenUnit *codegen_unit,
                             const LirInstruction *instruction,
                             MachineBlock *block) {
    bool ok;

    (void)lir_unit;

    ok = emit_preserve_before_call(context, codegen_unit, block) &&
         append_line(context,
                     block,
                     "call %s",
                     instruction->as.call.callee.as.global_name) &&
         emit_preserve_after_call(context, codegen_unit, block);
    if (!ok) {
        return false;
    }
    if (instruction->as.call.has_result) {
        return emit_store_vreg(context,
                               codegen_unit,
                               instruction->as.call.dest_vreg,
                               block,
                               codegen_register_name(CODEGEN_REG_RAX));
    }

    return true;
}

static bool emit_runtime_helper_call(MachineBuildContext *context,
                                     const CodegenUnit *codegen_unit,
                                     const RuntimeAbiHelperSignature *signature,
                                     MachineBlock *block,
                                     bool is_noreturn) {
    if (!signature) {
        machine_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Machine emission missing runtime helper signature.");
        return false;
    }

    if (!emit_preserve_before_call(context, codegen_unit, block) ||
        !append_line(context, block, "call %s", signature->name)) {
        return false;
    }
    if (!is_noreturn && !emit_preserve_after_call(context, codegen_unit, block)) {
        return false;
    }
    return true;
}

static bool emit_instruction(MachineBuildContext *context,
                             const LirUnit *lir_unit,
                             const CodegenUnit *codegen_unit,
                             const LirBasicBlock *lir_block,
                             const CodegenBlock *codegen_block,
                             size_t instruction_index,
                             MachineBlock *block) {
    const LirInstruction *instruction;
    const CodegenSelectedInstruction *selected;

    if (!lir_unit || !codegen_unit || !lir_block || !codegen_block || !block) {
        return false;
    }
    if (instruction_index >= lir_block->instruction_count ||
        instruction_index >= codegen_block->instruction_count) {
        machine_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Machine emission block shape mismatch.");
        return false;
    }

    instruction = &lir_block->instructions[instruction_index];
    selected = &codegen_block->instructions[instruction_index];

    if (selected->selection.kind == CODEGEN_SELECTION_DIRECT) {
        switch (selected->selection.as.direct_pattern) {
        case CODEGEN_DIRECT_ABI_ARG_MOVE: {
            char *destination = NULL;
            char *source = NULL;
            bool ok;

            if (!format_slot_operand(codegen_unit,
                                     instruction->as.incoming_arg.slot_index,
                                     &destination)) {
                return false;
            }
            if (instruction->as.incoming_arg.argument_index < 6) {
                source = ast_copy_text(codegen_register_name(
                    runtime_abi_get_helper_argument_register(context->program->target,
                                                             instruction->as.incoming_arg.argument_index)));
            } else {
                format_incoming_arg_stack_operand(instruction->as.incoming_arg.argument_index - 6,
                                                  &source);
            }
            ok = source != NULL && emit_move_to_destination(context, block, destination, source);
            free(destination);
            free(source);
            return ok;
        }
        case CODEGEN_DIRECT_ABI_CAPTURE_LOAD: {
            char *destination = NULL;
            char *source = NULL;
            bool ok;

            if (!format_slot_operand(codegen_unit,
                                     instruction->as.incoming_capture.slot_index,
                                     &destination) ||
                !format_capture_operand(instruction->as.incoming_capture.capture_index, &source)) {
                free(destination);
                free(source);
                return false;
            }
            ok = emit_move_to_destination(context, block, destination, source);
            free(destination);
            free(source);
            return ok;
        }
        case CODEGEN_DIRECT_ABI_OUTGOING_ARG: {
            const LirInstruction *call_instruction;
            const CodegenSelectedInstruction *call_selected;
            char *source = NULL;
            char *destination = NULL;
            bool ok;

            call_instruction = find_pending_call_instruction(lir_block, instruction_index, NULL);
            call_selected = find_pending_call_selection(codegen_block, instruction_index, NULL);
            if (!call_instruction || !call_selected) {
                machine_set_error(context,
                                  (AstSourceSpan){0},
                                  NULL,
                                  "Outgoing argument at index %zu is not followed by a call.",
                                  instruction_index);
                return false;
            }
            if (!format_operand(lir_unit,
                                codegen_unit,
                                instruction->as.outgoing_arg.value,
                                &source)) {
                return false;
            }

            if (call_selected->selection.kind == CODEGEN_SELECTION_DIRECT &&
                call_selected->selection.as.direct_pattern == CODEGEN_DIRECT_CALL_GLOBAL) {
                if (instruction->as.outgoing_arg.argument_index < 6) {
                    destination = ast_copy_text(codegen_register_name(
                        runtime_abi_get_helper_argument_register(context->program->target,
                                                                 instruction->as.outgoing_arg.argument_index)));
                } else {
                    destination = NULL;
                    format_outgoing_arg_stack_operand(instruction->as.outgoing_arg.argument_index - 6,
                                                      &destination);
                }
            } else if (call_selected->selection.kind == CODEGEN_SELECTION_RUNTIME &&
                       call_selected->selection.as.runtime_helper == CODEGEN_RUNTIME_CALL_CALLABLE) {
                destination = copy_format("helper(%zu)", instruction->as.outgoing_arg.argument_index);
            } else {
                free(source);
                machine_set_error(context,
                                  (AstSourceSpan){0},
                                  NULL,
                                  "Outgoing argument lowering only supports direct global calls or runtime callable dispatch.");
                return false;
            }

            ok = destination != NULL && emit_move_to_destination(context, block, destination, source);
            free(destination);
            free(source);
            return ok;
        }
        case CODEGEN_DIRECT_SCALAR_BINARY:
            return emit_binary(context, lir_unit, codegen_unit, instruction, block);
        case CODEGEN_DIRECT_SCALAR_UNARY:
            return emit_unary(context, lir_unit, codegen_unit, instruction, block);
        case CODEGEN_DIRECT_SCALAR_CAST:
            return emit_cast(context, lir_unit, codegen_unit, instruction, block);
        case CODEGEN_DIRECT_CALL_GLOBAL:
            return emit_call_global(context, lir_unit, codegen_unit, instruction, block);
        case CODEGEN_DIRECT_STORE_SLOT: {
            char *destination = NULL;
            char *source = NULL;
            bool ok;

            if (!format_slot_operand(codegen_unit, instruction->as.store_slot.slot_index, &destination) ||
                !format_operand(lir_unit, codegen_unit, instruction->as.store_slot.value, &source)) {
                free(destination);
                free(source);
                return false;
            }
            ok = emit_move_to_destination(context, block, destination, source);
            free(destination);
            free(source);
            return ok;
        }
        case CODEGEN_DIRECT_STORE_GLOBAL: {
            char *source = NULL;
            char *destination = copy_format("global(%s)", instruction->as.store_global.global_name);
            bool ok;

            if (!destination ||
                !format_operand(lir_unit, codegen_unit, instruction->as.store_global.value, &source)) {
                free(destination);
                free(source);
                return false;
            }
            ok = emit_move_to_destination(context, block, destination, source);
            free(destination);
            free(source);
            return ok;
        }
        case CODEGEN_DIRECT_RETURN:
        case CODEGEN_DIRECT_JUMP:
        case CODEGEN_DIRECT_BRANCH:
            machine_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Direct terminator pattern reached machine instruction emission.");
            return false;
        }
    }

    switch (selected->selection.as.runtime_helper) {
    case CODEGEN_RUNTIME_CLOSURE_NEW: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        size_t i;
        bool ok = true;

        for (i = 0; i < instruction->as.closure.capture_count; i++) {
            char *slot = NULL;
            char *value = NULL;

            ok = format_helper_slot_operand(i, &slot) &&
                 format_operand(lir_unit, codegen_unit, instruction->as.closure.captures[i], &value) &&
                 emit_move_to_destination(context, block, slot, value);
            free(slot);
            free(value);
            if (!ok) {
                return false;
            }
        }

        {
            char *unit_label = NULL;
            bool pointer_ok;

            if (!format_code_label_operand(instruction->as.closure.unit_name, &unit_label)) {
                return false;
            }
            pointer_ok = instruction->as.closure.capture_count > 0
                ? append_line(context, block, "lea rdx, helper(0)")
                : append_line(context, block, "mov rdx, null");
            ok = append_line(context, block, "mov rdi, %s", unit_label) &&
                 append_line(context,
                             block,
                             "mov rsi, %zu",
                             instruction->as.closure.capture_count) &&
                 pointer_ok &&
                 emit_runtime_helper_call(context,
                                          codegen_unit,
                                          signature,
                                          block,
                                          false) &&
                 emit_store_vreg(context,
                                 codegen_unit,
                                 instruction->as.closure.dest_vreg,
                                 block,
                                 "rax");
            free(unit_label);
            return ok;
        }
    }
    case CODEGEN_RUNTIME_CALL_CALLABLE: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        char *callee = NULL;
        bool ok;

        if (!format_operand(lir_unit, codegen_unit, instruction->as.call.callee, &callee)) {
            return false;
        }
        ok = append_line(context, block, "mov rdi, %s", callee) &&
             append_line(context, block, "mov rsi, %zu", instruction->as.call.argument_count) &&
             (instruction->as.call.argument_count > 0
                  ? append_line(context, block, "lea rdx, helper(0)")
                  : append_line(context, block, "mov rdx, null")) &&
             emit_runtime_helper_call(context, codegen_unit, signature, block, false);
        free(callee);
        if (!ok) {
            return false;
        }
        if (instruction->as.call.has_result) {
            return emit_store_vreg(context,
                                   codegen_unit,
                                   instruction->as.call.dest_vreg,
                                   block,
                                   "rax");
        }
        return true;
    }
    case CODEGEN_RUNTIME_MEMBER_LOAD: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        char *target = NULL;
        char *member = NULL;
        bool ok;

        if (!format_operand(lir_unit, codegen_unit, instruction->as.member.target, &target) ||
            !format_member_symbol_operand(instruction->as.member.member, &member)) {
            free(target);
            free(member);
            return false;
        }
        ok = append_line(context, block, "mov rdi, %s", target) &&
             append_line(context, block, "mov rsi, %s", member) &&
             emit_runtime_helper_call(context, codegen_unit, signature, block, false) &&
             emit_store_vreg(context,
                             codegen_unit,
                             instruction->as.member.dest_vreg,
                             block,
                             "rax");
        free(target);
        free(member);
        return ok;
    }
    case CODEGEN_RUNTIME_INDEX_LOAD: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        char *target = NULL;
        char *index = NULL;
        bool ok;

        if (!format_operand(lir_unit, codegen_unit, instruction->as.index_load.target, &target) ||
            !format_operand(lir_unit, codegen_unit, instruction->as.index_load.index, &index)) {
            free(target);
            free(index);
            return false;
        }
        ok = append_line(context, block, "mov rdi, %s", target) &&
             append_line(context, block, "mov rsi, %s", index) &&
             emit_runtime_helper_call(context, codegen_unit, signature, block, false) &&
             emit_store_vreg(context,
                             codegen_unit,
                             instruction->as.index_load.dest_vreg,
                             block,
                             "rax");
        free(target);
        free(index);
        return ok;
    }
    case CODEGEN_RUNTIME_ARRAY_LITERAL: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        size_t i;
        bool ok = true;

        for (i = 0; i < instruction->as.array_literal.element_count; i++) {
            char *slot = NULL;
            char *value = NULL;

            ok = format_helper_slot_operand(i, &slot) &&
                 format_operand(lir_unit, codegen_unit, instruction->as.array_literal.elements[i], &value) &&
                 emit_move_to_destination(context, block, slot, value);
            free(slot);
            free(value);
            if (!ok) {
                return false;
            }
        }
        ok = append_line(context,
                         block,
                         "mov rdi, %zu",
                         instruction->as.array_literal.element_count) &&
             (instruction->as.array_literal.element_count > 0
                  ? append_line(context, block, "lea rsi, helper(0)")
                  : append_line(context, block, "mov rsi, null")) &&
             emit_runtime_helper_call(context, codegen_unit, signature, block, false) &&
             emit_store_vreg(context,
                             codegen_unit,
                             instruction->as.array_literal.dest_vreg,
                             block,
                             "rax");
        return ok;
    }
    case CODEGEN_RUNTIME_TEMPLATE_BUILD: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        size_t i;
        bool ok = true;

        for (i = 0; i < instruction->as.template_literal.part_count; i++) {
            char *tag_slot = NULL;
            char *payload_slot = NULL;
            char *payload = NULL;

            if (!format_helper_slot_operand(i * 2, &tag_slot) ||
                !format_helper_slot_operand((i * 2) + 1, &payload_slot)) {
                free(tag_slot);
                free(payload_slot);
                return false;
            }
            if (instruction->as.template_literal.parts[i].kind == LIR_TEMPLATE_PART_TEXT) {
                ok = format_template_text_operand(instruction->as.template_literal.parts[i].as.text, &payload) &&
                     emit_move_to_destination(context, block, tag_slot, "tag(text)") &&
                     emit_move_to_destination(context, block, payload_slot, payload);
            } else {
                ok = format_operand(lir_unit,
                                    codegen_unit,
                                    instruction->as.template_literal.parts[i].as.value,
                                    &payload) &&
                     emit_move_to_destination(context, block, tag_slot, "tag(value)") &&
                     emit_move_to_destination(context, block, payload_slot, payload);
            }
            free(tag_slot);
            free(payload_slot);
            free(payload);
            if (!ok) {
                return false;
            }
        }

        ok = append_line(context,
                         block,
                         "mov rdi, %zu",
                         instruction->as.template_literal.part_count) &&
             (instruction->as.template_literal.part_count > 0
                  ? append_line(context, block, "lea rsi, helper(0)")
                  : append_line(context, block, "mov rsi, null")) &&
             emit_runtime_helper_call(context, codegen_unit, signature, block, false) &&
             emit_store_vreg(context,
                             codegen_unit,
                             instruction->as.template_literal.dest_vreg,
                             block,
                             "rax");
        return ok;
    }
    case CODEGEN_RUNTIME_STORE_INDEX: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        char *target = NULL;
        char *index = NULL;
        char *value = NULL;
        bool ok;

        if (!format_operand(lir_unit, codegen_unit, instruction->as.store_index.target, &target) ||
            !format_operand(lir_unit, codegen_unit, instruction->as.store_index.index, &index) ||
            !format_operand(lir_unit, codegen_unit, instruction->as.store_index.value, &value)) {
            free(target);
            free(index);
            free(value);
            return false;
        }
        ok = append_line(context, block, "mov rdi, %s", target) &&
             append_line(context, block, "mov rsi, %s", index) &&
             append_line(context, block, "mov rdx, %s", value) &&
             emit_runtime_helper_call(context, codegen_unit, signature, block, false);
        free(target);
        free(index);
        free(value);
        return ok;
    }
    case CODEGEN_RUNTIME_STORE_MEMBER: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        char *target = NULL;
        char *member = NULL;
        char *value = NULL;
        bool ok;

        if (!format_operand(lir_unit, codegen_unit, instruction->as.store_member.target, &target) ||
            !format_member_symbol_operand(instruction->as.store_member.member, &member) ||
            !format_operand(lir_unit, codegen_unit, instruction->as.store_member.value, &value)) {
            free(target);
            free(member);
            free(value);
            return false;
        }
        ok = append_line(context, block, "mov rdi, %s", target) &&
             append_line(context, block, "mov rsi, %s", member) &&
             append_line(context, block, "mov rdx, %s", value) &&
             emit_runtime_helper_call(context, codegen_unit, signature, block, false);
        free(target);
        free(member);
        free(value);
        return ok;
    }
    case CODEGEN_RUNTIME_CAST_VALUE: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        char *value = NULL;
        char type_tag[64];
        bool ok;

        if (!format_operand(lir_unit, codegen_unit, instruction->as.cast.operand, &value) ||
            !runtime_abi_format_type_tag(instruction->as.cast.target_type, type_tag, sizeof(type_tag))) {
            free(value);
            return false;
        }
        ok = append_line(context, block, "mov rdi, %s", value) &&
             append_line(context, block, "mov rsi, %s", type_tag) &&
             emit_runtime_helper_call(context, codegen_unit, signature, block, false) &&
             emit_store_vreg(context,
                             codegen_unit,
                             instruction->as.cast.dest_vreg,
                             block,
                             "rax");
        free(value);
        return ok;
    }
    case CODEGEN_RUNTIME_THROW:
        machine_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Throw helper should only be emitted from terminators.");
        return false;
    }

    return false;
}

static bool emit_terminator(MachineBuildContext *context,
                            const LirUnit *lir_unit,
                            const CodegenUnit *codegen_unit,
                            const LirBasicBlock *lir_block,
                            const MachineUnit *machine_unit,
                            MachineBlock *block) {
    const LirTerminator *terminator;
    const CodegenSelectedTerminator *selected;

    if (!lir_block || !machine_unit || !block) {
        return false;
    }

    terminator = &lir_block->terminator;
    selected = &codegen_unit->blocks[block - machine_unit->blocks].terminator;

    if (selected->selection.kind == CODEGEN_SELECTION_DIRECT) {
        switch (selected->selection.as.direct_pattern) {
        case CODEGEN_DIRECT_RETURN: {
            if (terminator->kind == LIR_TERM_RETURN && terminator->as.return_term.has_value) {
                char *value = NULL;
                bool ok;

                if (!format_operand(lir_unit,
                                    codegen_unit,
                                    terminator->as.return_term.value,
                                    &value)) {
                    return false;
                }
                ok = append_line(context, block, "mov rax, %s", value);
                free(value);
                if (!ok) {
                    return false;
                }
            }
            return emit_return_epilogue(context, codegen_unit, machine_unit, block);
        }
        case CODEGEN_DIRECT_JUMP:
            return append_line(context,
                               block,
                               "jmp %s",
                               codegen_unit->blocks[terminator->as.jump_term.target_block].label);
        case CODEGEN_DIRECT_BRANCH: {
            char *condition = NULL;
            bool ok;

            if (!format_operand(lir_unit,
                                codegen_unit,
                                terminator->as.branch_term.condition,
                                &condition)) {
                return false;
            }
            ok = append_line(context, block, "cmp %s, bool(false)", condition) &&
                 append_line(context,
                             block,
                             "jne %s",
                             codegen_unit->blocks[terminator->as.branch_term.true_block].label) &&
                 append_line(context,
                             block,
                             "jmp %s",
                             codegen_unit->blocks[terminator->as.branch_term.false_block].label);
            free(condition);
            return ok;
        }
        case CODEGEN_DIRECT_ABI_ARG_MOVE:
        case CODEGEN_DIRECT_ABI_CAPTURE_LOAD:
        case CODEGEN_DIRECT_ABI_OUTGOING_ARG:
        case CODEGEN_DIRECT_SCALAR_BINARY:
        case CODEGEN_DIRECT_SCALAR_UNARY:
        case CODEGEN_DIRECT_SCALAR_CAST:
        case CODEGEN_DIRECT_CALL_GLOBAL:
        case CODEGEN_DIRECT_STORE_SLOT:
        case CODEGEN_DIRECT_STORE_GLOBAL:
            machine_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Non-terminator direct pattern reached machine terminator emission.");
            return false;
        }
    }

    if (selected->selection.as.runtime_helper == CODEGEN_RUNTIME_THROW &&
        terminator->kind == LIR_TERM_THROW) {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             CODEGEN_RUNTIME_THROW);
        char *value = NULL;
        bool ok;

        if (!format_operand(lir_unit, codegen_unit, terminator->as.throw_term.value, &value)) {
            return false;
        }
        ok = append_line(context, block, "mov rdi, %s", value) &&
             emit_runtime_helper_call(context, codegen_unit, signature, block, true);
        free(value);
        return ok;
    }

    machine_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Unsupported machine terminator selection.");
    return false;
}

static bool build_unit(MachineBuildContext *context,
                       const LirUnit *lir_unit,
                       const CodegenUnit *codegen_unit,
                       MachineUnit *machine_unit) {
    size_t block_index;

    if (!lir_unit || !codegen_unit || !machine_unit) {
        return false;
    }
    if (lir_unit->block_count != codegen_unit->block_count) {
        machine_set_error(context,
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
    machine_unit->parameter_count = lir_unit->parameter_count;
    machine_unit->frame_slot_count = codegen_unit->frame_slot_count;
    machine_unit->spill_slot_count = codegen_unit->spill_slot_count;
    machine_unit->helper_slot_count = compute_helper_slot_count(lir_unit, codegen_unit);
    machine_unit->outgoing_stack_slot_count = compute_outgoing_stack_slot_count(lir_unit, codegen_unit);
    machine_unit->block_count = lir_unit->block_count;
    if (!machine_unit->name) {
        machine_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while naming machine unit.");
        return false;
    }
    if (machine_unit->frame_slot_count > 0) {
        size_t slot_index;

        machine_unit->frame_slots = calloc(machine_unit->frame_slot_count,
                                           sizeof(*machine_unit->frame_slots));
        if (!machine_unit->frame_slots) {
            machine_unit_free(machine_unit);
            machine_set_error(context,
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
                machine_unit_free(machine_unit);
                machine_set_error(context,
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
            machine_unit_free(machine_unit);
            machine_set_error(context,
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
            machine_unit_free(machine_unit);
            machine_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Machine emission requires matching LIR/codegen instruction counts for block %s.",
                              lir_block->label);
            return false;
        }

        block->label = ast_copy_text(lir_block->label);
        if (!block->label) {
            machine_unit_free(machine_unit);
            machine_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Out of memory while naming machine blocks.");
            return false;
        }

        if (block_index == 0 && !emit_entry_prologue(context, codegen_unit, machine_unit, block)) {
            machine_unit_free(machine_unit);
            return false;
        }

        for (instruction_index = 0;
             instruction_index < lir_block->instruction_count;
             instruction_index++) {
            if (!emit_instruction(context,
                                  lir_unit,
                                  codegen_unit,
                                  lir_block,
                                  codegen_block,
                                  instruction_index,
                                  block)) {
                machine_unit_free(machine_unit);
                return false;
            }
        }
        if (!emit_terminator(context,
                             lir_unit,
                             codegen_unit,
                             lir_block,
                             machine_unit,
                             block)) {
            machine_unit_free(machine_unit);
            return false;
        }
    }

    return true;
}