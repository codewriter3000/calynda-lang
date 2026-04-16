#include "asm_emit_internal.h"
#include "runtime.h"
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Extended operand translation for RV64                              */
/* ------------------------------------------------------------------ */

bool ae_translate_operand_ext_riscv64(AsmEmitContext *context,
                                      const MachineUnit *unit,
                                      const AsmUnitLayout *layout,
                                      const char *operand_text,
                                      bool destination,
                                      AsmOperand *operand) {
    char *inner = NULL;

    (void)layout;

    /* global(name) → data label */
    inner = ae_between_parens(operand_text, "global(");
    if (inner) {
        AsmGlobalSymbol *sym = ae_ensure_global_symbol(context, inner, destination);

        free(inner);
        if (!sym) {
            return false;
        }
        operand->kind = ASM_OPERAND_ADDRESS;
        operand->text = ae_copy_text(sym->symbol);
        return operand->text != NULL;
    }

    /* code(name) → function label address */
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
            AsmUnitSymbol *sym = ae_ensure_unit_symbol(context, inner);

            free(inner);
            if (!sym) {
                return false;
            }
            operand->kind = ASM_OPERAND_ADDRESS;
            operand->text = ae_copy_text(sym->symbol);
            return operand->text != NULL;
        }
    }

    /* symbol(name) → interned symbol string pointer */
    inner = ae_between_parens(operand_text, "symbol(");
    if (inner) {
        AsmByteLiteral *lit =
            ae_ensure_byte_literal(context, inner, strlen(inner), "sym");

        free(inner);
        if (!lit) {
            return false;
        }
        operand->kind = ASM_OPERAND_ADDRESS;
        operand->text = ae_copy_text(lit->label);
        return operand->text != NULL;
    }

    /* text("...") → raw byte literal */
    inner = ae_between_parens(operand_text, "text(");
    if (inner) {
        char *decoded = NULL;
        size_t decoded_length = 0;
        AsmByteLiteral *lit;

        if (!ae_decode_quoted_text(inner, &decoded, &decoded_length)) {
            free(inner);
            return false;
        }
        lit = ae_ensure_byte_literal(context, decoded, decoded_length, "txt");
        free(decoded);
        free(inner);
        if (!lit) {
            return false;
        }
        operand->kind = ASM_OPERAND_ADDRESS;
        operand->text = ae_copy_text(lit->label);
        return operand->text != NULL;
    }

    /* string("...") → string object literal */
    inner = ae_between_parens(operand_text, "string(");
    if (inner) {
        char *decoded = NULL;
        size_t decoded_length = 0;
        AsmStringObjectLiteral *lit;

        if (!ae_decode_quoted_text(inner, &decoded, &decoded_length)) {
            free(inner);
            return false;
        }
        lit = ae_ensure_string_literal(context, decoded, decoded_length);
        free(decoded);
        free(inner);
        if (!lit) {
            return false;
        }
        operand->kind = ASM_OPERAND_ADDRESS;
        operand->text = ae_copy_text(lit->object_label);
        return operand->text != NULL;
    }
    if (ae_starts_with(operand_text, "typedesc(")) {
        return ae_translate_type_descriptor_operand(context, operand_text, operand);
    }

    /* tag(text) / tag(value) */
    inner = ae_between_parens(operand_text, "tag(");
    if (inner) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_format("%s", strcmp(inner, "text") == 0 ? "0" : "1");
        free(inner);
        return operand->text != NULL;
    }

    /* type(name) → runtime type tag */
    inner = ae_between_parens(operand_text, "type(");
    if (inner) {
        long long type_tag = ae_runtime_type_tag_value(inner);

        free(inner);
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_format("%lld", type_tag);
        return operand->text != NULL;
    }

    /* int32(N) / int64(N) / i64(N) */
    inner = ae_between_parens(operand_text, "int32(");
    if (!inner) inner = ae_between_parens(operand_text, "int64(");
    if (!inner) inner = ae_between_parens(operand_text, "i64(");
    if (inner) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_text(inner);
        free(inner);
        return operand->text != NULL;
    }

    /* Branch target labels (bbN) */
    if (ae_starts_with(operand_text, "bb")) {
        AsmUnitSymbol *sym = ae_ensure_unit_symbol(context, unit->name);

        if (!sym) {
            return false;
        }
        operand->kind = ASM_OPERAND_BRANCH_LABEL;
        operand->text = ae_copy_format(".L%s_%s", sym->symbol, operand_text);
        return operand->text != NULL;
    }

    /* "zero" pseudo-register for RV64 */
    if (strcmp(operand_text, "zero") == 0) {
        operand->kind = ASM_OPERAND_REGISTER;
        operand->text = ae_copy_text("zero");
        return operand->text != NULL;
    }

    /* Runtime function call target */
    if (ae_starts_with(operand_text, "__calynda_")) {
        operand->kind = ASM_OPERAND_CALL_TARGET;
        operand->text = ae_copy_text(operand_text);
        return operand->text != NULL;
    }

    /* Named function call target */
    {
        AsmUnitSymbol *sym = ae_ensure_unit_symbol(context, operand_text);

        if (!sym) {
            return false;
        }
        operand->kind = ASM_OPERAND_CALL_TARGET;
        operand->text = ae_copy_text(sym->symbol);
        return operand->text != NULL;
    }
}
