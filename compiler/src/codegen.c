#include "codegen.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    CodegenProgram  *program;
    const LirProgram *lir_program;
} CodegenBuildContext;

static const CodegenRegister X86_64_SYSV_ALLOCATABLE_REGS[] = {
    CODEGEN_REG_R10,
    CODEGEN_REG_R11,
    CODEGEN_REG_R12,
    CODEGEN_REG_R13
};

static bool source_span_is_valid(AstSourceSpan span);
static void codegen_set_error(CodegenBuildContext *context,
                              AstSourceSpan primary_span,
                              const AstSourceSpan *related_span,
                              const char *format,
                              ...);
static void codegen_block_free(CodegenBlock *block);
static void codegen_unit_free(CodegenUnit *unit);
static CheckedType checked_type_invalid_value(void);
static bool is_direct_scalar_type(CheckedType type);
static void assign_vreg_type(CheckedType *types,
                             size_t type_count,
                             size_t vreg_index,
                             CheckedType type);
static void propagate_operand_vreg_type(CheckedType *types,
                                        size_t type_count,
                                        LirOperand operand);
static void infer_vreg_types(const LirUnit *lir_unit,
                             CheckedType *types,
                             size_t type_count);
static bool select_instruction(CodegenBuildContext *context,
                               const LirInstruction *instruction,
                               CodegenSelectedInstruction *selected);
static bool select_terminator(CodegenBuildContext *context,
                              const LirTerminator *terminator,
                              CodegenSelectedTerminator *selected);
static bool build_unit(CodegenBuildContext *context,
                       const LirUnit *lir_unit,
                       CodegenUnit *unit);

void codegen_program_init(CodegenProgram *program) {
    if (!program) {
        return;
    }

    memset(program, 0, sizeof(*program));
    program->target = CODEGEN_TARGET_X86_64_SYSV_ELF;
}

void codegen_program_free(CodegenProgram *program) {
    size_t i;

    if (!program) {
        return;
    }

    for (i = 0; i < program->unit_count; i++) {
        codegen_unit_free(&program->units[i]);
    }
    free(program->units);
    memset(program, 0, sizeof(*program));
}

const CodegenBuildError *codegen_get_error(const CodegenProgram *program) {
    if (!program || !program->has_error) {
        return NULL;
    }

    return &program->error;
}

bool codegen_format_error(const CodegenBuildError *error,
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

const char *codegen_target_name(CodegenTargetKind target) {
    switch (target) {
    case CODEGEN_TARGET_X86_64_SYSV_ELF:
        return "x86_64_sysv_elf";
    }

    return "unknown";
}

const char *codegen_register_name(CodegenRegister reg) {
    switch (reg) {
    case CODEGEN_REG_RAX:
        return "rax";
    case CODEGEN_REG_RDI:
        return "rdi";
    case CODEGEN_REG_RSI:
        return "rsi";
    case CODEGEN_REG_RDX:
        return "rdx";
    case CODEGEN_REG_RCX:
        return "rcx";
    case CODEGEN_REG_R8:
        return "r8";
    case CODEGEN_REG_R9:
        return "r9";
    case CODEGEN_REG_R10:
        return "r10";
    case CODEGEN_REG_R11:
        return "r11";
    case CODEGEN_REG_R12:
        return "r12";
    case CODEGEN_REG_R13:
        return "r13";
    case CODEGEN_REG_R14:
        return "r14";
    case CODEGEN_REG_R15:
        return "r15";
    }

    return "?";
}

const char *codegen_direct_pattern_name(CodegenDirectPattern pattern) {
    switch (pattern) {
    case CODEGEN_DIRECT_ABI_ARG_MOVE:
        return "abi-arg-move";
    case CODEGEN_DIRECT_ABI_CAPTURE_LOAD:
        return "abi-capture-load";
    case CODEGEN_DIRECT_ABI_OUTGOING_ARG:
        return "abi-outgoing-arg";
    case CODEGEN_DIRECT_SCALAR_BINARY:
        return "scalar-binary";
    case CODEGEN_DIRECT_SCALAR_UNARY:
        return "scalar-unary";
    case CODEGEN_DIRECT_SCALAR_CAST:
        return "scalar-cast";
    case CODEGEN_DIRECT_CALL_GLOBAL:
        return "call-global";
    case CODEGEN_DIRECT_STORE_SLOT:
        return "store-slot";
    case CODEGEN_DIRECT_STORE_GLOBAL:
        return "store-global";
    case CODEGEN_DIRECT_RETURN:
        return "return";
    case CODEGEN_DIRECT_JUMP:
        return "jump";
    case CODEGEN_DIRECT_BRANCH:
        return "branch";
    }

    return "unknown-direct";
}

const char *codegen_runtime_helper_name(CodegenRuntimeHelper helper) {
    switch (helper) {
    case CODEGEN_RUNTIME_CLOSURE_NEW:
        return "__calynda_rt_closure_new";
    case CODEGEN_RUNTIME_CALL_CALLABLE:
        return "__calynda_rt_call_callable";
    case CODEGEN_RUNTIME_MEMBER_LOAD:
        return "__calynda_rt_member_load";
    case CODEGEN_RUNTIME_INDEX_LOAD:
        return "__calynda_rt_index_load";
    case CODEGEN_RUNTIME_ARRAY_LITERAL:
        return "__calynda_rt_array_literal";
    case CODEGEN_RUNTIME_TEMPLATE_BUILD:
        return "__calynda_rt_template_build";
    case CODEGEN_RUNTIME_STORE_INDEX:
        return "__calynda_rt_store_index";
    case CODEGEN_RUNTIME_STORE_MEMBER:
        return "__calynda_rt_store_member";
    case CODEGEN_RUNTIME_THROW:
        return "__calynda_rt_throw";
    case CODEGEN_RUNTIME_CAST_VALUE:
        return "__calynda_rt_cast_value";
    }

    return "__calynda_rt_unknown";
}

static bool source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

static void codegen_set_error(CodegenBuildContext *context,
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

static void codegen_block_free(CodegenBlock *block) {
    if (!block) {
        return;
    }

    free(block->label);
    free(block->instructions);
    memset(block, 0, sizeof(*block));
}

static void codegen_unit_free(CodegenUnit *unit) {
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
        codegen_block_free(&unit->blocks[i]);
    }
    free(unit->blocks);
    memset(unit, 0, sizeof(*unit));
}

static CheckedType checked_type_invalid_value(void) {
    CheckedType type;

    memset(&type, 0, sizeof(type));
    type.kind = CHECKED_TYPE_INVALID;
    return type;
}

static bool is_direct_scalar_type(CheckedType type) {
    return type.kind == CHECKED_TYPE_VALUE && type.array_depth == 0;
}

static void assign_vreg_type(CheckedType *types,
                             size_t type_count,
                             size_t vreg_index,
                             CheckedType type) {
    if (!types || vreg_index >= type_count || type.kind == CHECKED_TYPE_INVALID) {
        return;
    }

    if (types[vreg_index].kind == CHECKED_TYPE_INVALID) {
        types[vreg_index] = type;
    }
}

static void propagate_operand_vreg_type(CheckedType *types,
                                        size_t type_count,
                                        LirOperand operand) {
    if (operand.kind == LIR_OPERAND_VREG) {
        assign_vreg_type(types, type_count, operand.as.vreg_index, operand.type);
    }
}

static void infer_vreg_types(const LirUnit *lir_unit,
                             CheckedType *types,
                             size_t type_count) {
    size_t block_index;

    if (!lir_unit || !types) {
        return;
    }

    for (block_index = 0; block_index < lir_unit->block_count; block_index++) {
        const LirBasicBlock *block = &lir_unit->blocks[block_index];
        size_t instruction_index;

        for (instruction_index = 0;
             instruction_index < block->instruction_count;
             instruction_index++) {
            const LirInstruction *instruction = &block->instructions[instruction_index];

            switch (instruction->kind) {
            case LIR_INSTR_INCOMING_ARG:
            case LIR_INSTR_INCOMING_CAPTURE:
                break;

            case LIR_INSTR_OUTGOING_ARG:
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.outgoing_arg.value);
                break;

            case LIR_INSTR_BINARY:
                assign_vreg_type(types,
                                 type_count,
                                 instruction->as.binary.dest_vreg,
                                 instruction->as.binary.left.type);
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.binary.left);
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.binary.right);
                break;

            case LIR_INSTR_UNARY:
                assign_vreg_type(types,
                                 type_count,
                                 instruction->as.unary.dest_vreg,
                                 instruction->as.unary.operand.type);
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.unary.operand);
                break;

            case LIR_INSTR_CLOSURE:
                for (size_t capture_index = 0;
                     capture_index < instruction->as.closure.capture_count;
                     capture_index++) {
                    propagate_operand_vreg_type(types,
                                                type_count,
                                                instruction->as.closure.captures[capture_index]);
                }
                break;

            case LIR_INSTR_CALL:
                if (instruction->as.call.has_result) {
                    assign_vreg_type(types,
                                     type_count,
                                     instruction->as.call.dest_vreg,
                                     instruction->as.call.callee.type);
                }
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.call.callee);
                break;

            case LIR_INSTR_CAST:
                assign_vreg_type(types,
                                 type_count,
                                 instruction->as.cast.dest_vreg,
                                 instruction->as.cast.target_type);
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.cast.operand);
                break;

            case LIR_INSTR_MEMBER:
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.member.target);
                break;

            case LIR_INSTR_INDEX_LOAD:
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.index_load.target);
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.index_load.index);
                break;

            case LIR_INSTR_ARRAY_LITERAL:
                for (size_t element_index = 0;
                     element_index < instruction->as.array_literal.element_count;
                     element_index++) {
                    propagate_operand_vreg_type(types,
                                                type_count,
                                                instruction->as.array_literal.elements[element_index]);
                }
                break;

            case LIR_INSTR_TEMPLATE:
                for (size_t part_index = 0;
                     part_index < instruction->as.template_literal.part_count;
                     part_index++) {
                    if (instruction->as.template_literal.parts[part_index].kind ==
                        LIR_TEMPLATE_PART_VALUE) {
                        propagate_operand_vreg_type(types,
                                                    type_count,
                                                    instruction->as.template_literal.parts[part_index].as.value);
                    }
                }
                break;

            case LIR_INSTR_STORE_SLOT:
                if (instruction->as.store_slot.value.kind == LIR_OPERAND_VREG &&
                    instruction->as.store_slot.slot_index < lir_unit->slot_count) {
                    assign_vreg_type(types,
                                     type_count,
                                     instruction->as.store_slot.value.as.vreg_index,
                                     lir_unit->slots[instruction->as.store_slot.slot_index].type);
                }
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.store_slot.value);
                break;

            case LIR_INSTR_STORE_GLOBAL:
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.store_global.value);
                break;

            case LIR_INSTR_STORE_INDEX:
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.store_index.target);
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.store_index.index);
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.store_index.value);
                break;

            case LIR_INSTR_STORE_MEMBER:
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.store_member.target);
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.store_member.value);
                break;
            }
        }

        switch (block->terminator.kind) {
        case LIR_TERM_RETURN:
            if (block->terminator.as.return_term.has_value) {
                propagate_operand_vreg_type(types,
                                            type_count,
                                            block->terminator.as.return_term.value);
            }
            break;

        case LIR_TERM_BRANCH:
            propagate_operand_vreg_type(types,
                                        type_count,
                                        block->terminator.as.branch_term.condition);
            break;

        case LIR_TERM_THROW:
            propagate_operand_vreg_type(types,
                                        type_count,
                                        block->terminator.as.throw_term.value);
            break;

        case LIR_TERM_NONE:
        case LIR_TERM_JUMP:
            break;
        }
    }
}

static bool select_instruction(CodegenBuildContext *context,
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
            selected->selection.kind = CODEGEN_SELECTION_DIRECT;
            selected->selection.as.direct_pattern = CODEGEN_DIRECT_CALL_GLOBAL;
        } else {
            selected->selection.kind = CODEGEN_SELECTION_RUNTIME;
            selected->selection.as.runtime_helper = CODEGEN_RUNTIME_CALL_CALLABLE;
        }
        return true;

    case LIR_INSTR_CAST:
        if (is_direct_scalar_type(instruction->as.cast.target_type) &&
            is_direct_scalar_type(instruction->as.cast.operand.type)) {
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
    }

    codegen_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Internal error: unsupported LIR instruction kind %d during instruction selection.",
                      instruction->kind);
    return false;
}

static bool select_terminator(CodegenBuildContext *context,
                              const LirTerminator *terminator,
                              CodegenSelectedTerminator *selected) {
    if (!terminator || !selected) {
        return false;
    }

    memset(selected, 0, sizeof(*selected));
    selected->kind = terminator->kind;

    switch (terminator->kind) {
    case LIR_TERM_NONE:
        selected->selection.kind = CODEGEN_SELECTION_DIRECT;
        selected->selection.as.direct_pattern = CODEGEN_DIRECT_RETURN;
        return true;

    case LIR_TERM_RETURN:
        selected->selection.kind = CODEGEN_SELECTION_DIRECT;
        selected->selection.as.direct_pattern = CODEGEN_DIRECT_RETURN;
        return true;

    case LIR_TERM_JUMP:
        selected->selection.kind = CODEGEN_SELECTION_DIRECT;
        selected->selection.as.direct_pattern = CODEGEN_DIRECT_JUMP;
        return true;

    case LIR_TERM_BRANCH:
        selected->selection.kind = CODEGEN_SELECTION_DIRECT;
        selected->selection.as.direct_pattern = CODEGEN_DIRECT_BRANCH;
        return true;

    case LIR_TERM_THROW:
        selected->selection.kind = CODEGEN_SELECTION_RUNTIME;
        selected->selection.as.runtime_helper = CODEGEN_RUNTIME_THROW;
        return true;
    }

    codegen_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Internal error: unsupported LIR terminator kind %d during instruction selection.",
                      terminator->kind);
    return false;
}

static bool build_unit(CodegenBuildContext *context,
                       const LirUnit *lir_unit,
                       CodegenUnit *unit) {
    size_t i;
    CheckedType *vreg_types;

    if (!lir_unit || !unit) {
        return false;
    }

    memset(unit, 0, sizeof(*unit));
    unit->kind = lir_unit->kind;
    unit->name = ast_copy_text(lir_unit->name);
    unit->symbol = lir_unit->symbol;
    unit->return_type = lir_unit->return_type;
    unit->frame_slot_count = lir_unit->slot_count;
    unit->vreg_count = lir_unit->virtual_register_count;
    unit->block_count = lir_unit->block_count;
    if (!unit->name) {
        codegen_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while building codegen units.");
        return false;
    }

    if (unit->frame_slot_count > 0) {
        unit->frame_slots = calloc(unit->frame_slot_count, sizeof(*unit->frame_slots));
        if (!unit->frame_slots) {
            codegen_unit_free(unit);
            codegen_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Out of memory while building codegen frame slots.");
            return false;
        }
    }
    for (i = 0; i < unit->frame_slot_count; i++) {
        unit->frame_slots[i].lir_slot_index = i;
        unit->frame_slots[i].frame_slot_index = i;
        unit->frame_slots[i].kind = lir_unit->slots[i].kind;
        unit->frame_slots[i].name = ast_copy_text(lir_unit->slots[i].name);
        unit->frame_slots[i].type = lir_unit->slots[i].type;
        unit->frame_slots[i].is_final = lir_unit->slots[i].is_final;
        if (!unit->frame_slots[i].name) {
            codegen_unit_free(unit);
            codegen_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Out of memory while naming codegen frame slots.");
            return false;
        }
    }

    if (unit->vreg_count > 0) {
        unit->vreg_allocations = calloc(unit->vreg_count, sizeof(*unit->vreg_allocations));
        if (!unit->vreg_allocations) {
            codegen_unit_free(unit);
            codegen_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Out of memory while building codegen register allocations.");
            return false;
        }
    }
    vreg_types = NULL;
    if (unit->vreg_count > 0) {
        vreg_types = calloc(unit->vreg_count, sizeof(*vreg_types));
        if (!vreg_types) {
            codegen_unit_free(unit);
            codegen_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Out of memory while inferring codegen virtual-register types.");
            return false;
        }
        for (i = 0; i < unit->vreg_count; i++) {
            vreg_types[i] = checked_type_invalid_value();
        }
        infer_vreg_types(lir_unit, vreg_types, unit->vreg_count);
    }
    for (i = 0; i < unit->vreg_count; i++) {
        unit->vreg_allocations[i].vreg_index = i;
        unit->vreg_allocations[i].type = vreg_types ? vreg_types[i] : checked_type_invalid_value();
        if (i < (sizeof(X86_64_SYSV_ALLOCATABLE_REGS) / sizeof(X86_64_SYSV_ALLOCATABLE_REGS[0]))) {
            unit->vreg_allocations[i].location.kind = CODEGEN_VREG_REGISTER;
            unit->vreg_allocations[i].location.as.reg = X86_64_SYSV_ALLOCATABLE_REGS[i];
        } else {
            unit->vreg_allocations[i].location.kind = CODEGEN_VREG_SPILL;
            unit->vreg_allocations[i].location.as.spill_slot_index =
                i - (sizeof(X86_64_SYSV_ALLOCATABLE_REGS) / sizeof(X86_64_SYSV_ALLOCATABLE_REGS[0]));
            unit->spill_slot_count++;
        }
    }
    free(vreg_types);

    if (unit->block_count > 0) {
        unit->blocks = calloc(unit->block_count, sizeof(*unit->blocks));
        if (!unit->blocks) {
            codegen_unit_free(unit);
            codegen_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Out of memory while building codegen blocks.");
            return false;
        }
    }
    for (i = 0; i < unit->block_count; i++) {
        size_t j;

        unit->blocks[i].label = ast_copy_text(lir_unit->blocks[i].label);
        unit->blocks[i].instruction_count = lir_unit->blocks[i].instruction_count;
        if (!unit->blocks[i].label) {
            codegen_unit_free(unit);
            codegen_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Out of memory while naming codegen blocks.");
            return false;
        }

        if (unit->blocks[i].instruction_count > 0) {
            unit->blocks[i].instructions = calloc(unit->blocks[i].instruction_count,
                                                  sizeof(*unit->blocks[i].instructions));
            if (!unit->blocks[i].instructions) {
                codegen_unit_free(unit);
                codegen_set_error(context,
                                  (AstSourceSpan){0},
                                  NULL,
                                  "Out of memory while building codegen instruction selections.");
                return false;
            }
        }

        for (j = 0; j < unit->blocks[i].instruction_count; j++) {
            if (!select_instruction(context,
                                    &lir_unit->blocks[i].instructions[j],
                                    &unit->blocks[i].instructions[j])) {
                codegen_unit_free(unit);
                return false;
            }
        }

        if (!select_terminator(context,
                               &lir_unit->blocks[i].terminator,
                               &unit->blocks[i].terminator)) {
            codegen_unit_free(unit);
            return false;
        }
    }

    return true;
}

bool codegen_build_program(CodegenProgram *program, const LirProgram *lir_program) {
    CodegenBuildContext context;
    size_t i;

    if (!program || !lir_program) {
        return false;
    }

    codegen_program_free(program);
    codegen_program_init(program);

    memset(&context, 0, sizeof(context));
    context.program = program;
    context.lir_program = lir_program;

    if (lir_get_error(lir_program) != NULL) {
        codegen_set_error(&context,
                          (AstSourceSpan){0},
                          NULL,
                          "Cannot build codegen plan while the LIR reports errors.");
        return false;
    }

    program->unit_count = lir_program->unit_count;
    if (program->unit_count > 0) {
        program->units = calloc(program->unit_count, sizeof(*program->units));
        if (!program->units) {
            codegen_set_error(&context,
                              (AstSourceSpan){0},
                              NULL,
                              "Out of memory while allocating codegen units.");
            return false;
        }
    }

    for (i = 0; i < program->unit_count; i++) {
        if (!build_unit(&context, &lir_program->units[i], &program->units[i])) {
            return false;
        }
    }

    return true;
}