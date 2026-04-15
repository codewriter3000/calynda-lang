#include "asm_emit_internal.h"
#include "runtime.h"
#include <stdlib.h>
#include <string.h>

bool ae_translate_operand_ext(AsmEmitContext *context,
                              const MachineUnit *unit,
                              const AsmUnitLayout *layout,
                              const char *operand_text,
                              bool destination,
                              AsmOperand *operand) {
    char *inner = NULL;
    char *decoded = NULL;
    size_t decoded_length = 0;

    (void)layout;

    inner = ae_between_parens(operand_text, "global(");
    if (inner) {
        AsmGlobalSymbol *symbol = ae_ensure_global_symbol(context, inner, destination);
        free(inner);
        if (!symbol) {
            return false;
        }
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("QWORD PTR [rip + %s]", symbol->symbol);
        return operand->text != NULL;
    }
    inner = ae_between_parens(operand_text, "code(");
    if (inner) {
        const MachineUnit *referenced_unit = ae_find_program_unit(context, inner);

        if (referenced_unit && referenced_unit->kind == LIR_UNIT_LAMBDA) {
            operand->kind = ASM_OPERAND_ADDRESS;
            operand->text = ae_closure_wrapper_symbol_name(inner);
            free(inner);
            return operand->text != NULL;
        }

        {
            AsmUnitSymbol *symbol = ae_ensure_unit_symbol(context, inner);

            free(inner);
            if (!symbol) {
                return false;
            }
            operand->kind = ASM_OPERAND_ADDRESS;
            operand->text = ae_copy_text(symbol->symbol);
            return operand->text != NULL;
        }
    }
    inner = ae_between_parens(operand_text, "symbol(");
    if (inner) {
        AsmByteLiteral *literal = ae_ensure_byte_literal(context, inner, strlen(inner), "sym");
        free(inner);
        if (!literal) {
            return false;
        }
        operand->kind = ASM_OPERAND_ADDRESS;
        operand->text = ae_copy_text(literal->label);
        return operand->text != NULL;
    }
    inner = ae_between_parens(operand_text, "text(");
    if (inner) {
        AsmByteLiteral *literal;

        if (!ae_decode_quoted_text(inner, &decoded, &decoded_length)) {
            free(inner);
            return false;
        }
        literal = ae_ensure_byte_literal(context, decoded, decoded_length, "txt");
        free(decoded);
        free(inner);
        if (!literal) {
            return false;
        }
        operand->kind = ASM_OPERAND_ADDRESS;
        operand->text = ae_copy_text(literal->label);
        return operand->text != NULL;
    }
    inner = ae_between_parens(operand_text, "string(");
    if (inner) {
        AsmStringObjectLiteral *literal;

        if (!ae_decode_quoted_text(inner, &decoded, &decoded_length)) {
            free(inner);
            return false;
        }
        literal = ae_ensure_string_literal(context, decoded, decoded_length);
        free(decoded);
        free(inner);
        if (!literal) {
            return false;
        }
        operand->kind = ASM_OPERAND_ADDRESS;
        operand->text = ae_copy_text(literal->object_label);
        return operand->text != NULL;
    }
    inner = ae_between_parens(operand_text, "tag(");
    if (inner) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_text(strcmp(inner, "text") == 0 ? "0" : "1");
        free(inner);
        return operand->text != NULL;
    }
    inner = ae_between_parens(operand_text, "type(");
    if (inner) {
        long long type_tag = ae_runtime_type_tag_value(inner);

        free(inner);
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_format("%lld", type_tag);
        return operand->text != NULL;
    }
    inner = ae_between_parens(operand_text, "int32(");
    if (inner) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = inner;
        return true;
    }
    inner = ae_between_parens(operand_text, "int64(");
    if (inner) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = inner;
        return true;
    }

    if (ae_starts_with(operand_text, "bb")) {
        AsmUnitSymbol *symbol = ae_ensure_unit_symbol(context, unit->name);

        if (!symbol) {
            return false;
        }
        operand->kind = ASM_OPERAND_BRANCH_LABEL;
        operand->text = ae_copy_format(".L%s_%s", symbol->symbol, operand_text);
        return operand->text != NULL;
    }

    operand->kind = ASM_OPERAND_CALL_TARGET;
    if (ae_starts_with(operand_text, "__calynda_rt_") ||
        strcmp(operand_text, "malloc") == 0 ||
        strcmp(operand_text, "calloc") == 0 ||
        strcmp(operand_text, "realloc") == 0 ||
        strcmp(operand_text, "free") == 0) {
        operand->text = ae_copy_text(operand_text);
    } else {
        AsmUnitSymbol *symbol = ae_ensure_unit_symbol(context, operand_text);

        if (!symbol) {
            return false;
        }
        operand->text = ae_copy_text(symbol->symbol);
    }
    return operand->text != NULL;
}

void ae_free_operand(AsmOperand *operand) {
    if (!operand) {
        return;
    }
    free(operand->text);
    memset(operand, 0, sizeof(*operand));
}

bool ae_write_operand(FILE *out, const AsmOperand *operand) {
    if (!out || !operand || !operand->text) {
        return false;
    }
    switch (operand->kind) {
    case ASM_OPERAND_IMMEDIATE:
        return fprintf(out, "%s", operand->text) >= 0;
    case ASM_OPERAND_ADDRESS:
        return fprintf(out, "OFFSET FLAT:%s", operand->text) >= 0;
    default:
        return fprintf(out, "%s", operand->text) >= 0;
    }
}

long long ae_runtime_type_tag_value(const char *type_name) {
    if (strcmp(type_name, "bool") == 0) {
        return CALYNDA_RT_TYPE_BOOL;
    }
    if (strcmp(type_name, "int32") == 0) {
        return CALYNDA_RT_TYPE_INT32;
    }
    if (strcmp(type_name, "int64") == 0) {
        return CALYNDA_RT_TYPE_INT64;
    }
    if (strcmp(type_name, "string") == 0) {
        return CALYNDA_RT_TYPE_STRING;
    }
    if (strcmp(type_name, "string[]") == 0 || strstr(type_name, "[]") != NULL) {
        return CALYNDA_RT_TYPE_ARRAY;
    }
    return CALYNDA_RT_TYPE_RAW_WORD;
}
