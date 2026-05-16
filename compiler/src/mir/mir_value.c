#include "mir_internal.h"

MirValue mr_invalid_value(void) {
    MirValue value;

    memset(&value, 0, sizeof(value));
    value.kind = MIR_VALUE_INVALID;
    return value;
}

bool mr_value_clone(MirBuildContext *context,
                    const MirValue *source,
                    MirValue *clone) {
    if (!source || !clone) {
        return false;
    }

    *clone = mr_invalid_value();
    clone->kind = source->kind;
    clone->type = source->type;
    switch (source->kind) {
    case MIR_VALUE_INVALID:
        return true;
    case MIR_VALUE_TEMP:
        clone->as.temp_index = source->as.temp_index;
        return true;
    case MIR_VALUE_LOCAL:
        clone->as.local_index = source->as.local_index;
        return true;
    case MIR_VALUE_GLOBAL:
        clone->as.global_name = ast_copy_text(source->as.global_name);
        if (!clone->as.global_name) {
            mr_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while cloning MIR globals.");
            return false;
        }
        return true;
    case MIR_VALUE_LITERAL:
        clone->as.literal.kind = source->as.literal.kind;
        if (source->as.literal.kind == AST_LITERAL_BOOL) {
            clone->as.literal.bool_value = source->as.literal.bool_value;
            return true;
        }
        if (source->as.literal.kind == AST_LITERAL_NULL) {
            return true;
        }
        clone->as.literal.text = ast_copy_text(source->as.literal.text);
        if (!clone->as.literal.text) {
            mr_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while cloning MIR literals.");
            return false;
        }
        return true;
    }

    return false;
}

void mr_value_free(MirValue *value) {
    if (!value) {
        return;
    }

    if (value->kind == MIR_VALUE_GLOBAL) {
        free(value->as.global_name);
    } else if (value->kind == MIR_VALUE_LITERAL &&
               value->as.literal.kind != AST_LITERAL_BOOL &&
               value->as.literal.kind != AST_LITERAL_NULL) {
        free(value->as.literal.text);
    }

    memset(value, 0, sizeof(*value));
}

void mr_template_part_free(MirTemplatePart *part) {
    if (!part) {
        return;
    }

    if (part->kind == MIR_TEMPLATE_PART_TEXT) {
        free(part->as.text);
    } else {
        mr_value_free(&part->as.value);
    }

    memset(part, 0, sizeof(*part));
}

void mr_lvalue_free(MirLValue *lvalue) {
    if (!lvalue) {
        return;
    }

    switch (lvalue->kind) {
    case MIR_LVALUE_GLOBAL:
        free(lvalue->as.global_name);
        break;
    case MIR_LVALUE_INDEX:
        mr_value_free(&lvalue->as.index.target);
        mr_value_free(&lvalue->as.index.index);
        break;
    case MIR_LVALUE_MEMBER:
        mr_value_free(&lvalue->as.member.target);
        free(lvalue->as.member.member);
        break;
    case MIR_LVALUE_MEMORY_DEREF:
        mr_value_free(&lvalue->as.memory_deref.target);
        break;
    case MIR_LVALUE_LOCAL:
        break;
    }

    memset(lvalue, 0, sizeof(*lvalue));
}

bool mr_value_from_literal(MirBuildContext *context,
                           const HirLiteral *literal,
                           CheckedType type,
                           MirValue *value) {
    if (!literal || !value) {
        return false;
    }

    memset(value, 0, sizeof(*value));
    value->kind = MIR_VALUE_LITERAL;
    value->type = type;
    value->as.literal.kind = literal->kind;
    if (literal->kind == AST_LITERAL_BOOL) {
        value->as.literal.bool_value = literal->as.bool_value;
        return true;
    }
    if (literal->kind == AST_LITERAL_NULL) {
        return true;
    }

    value->as.literal.text = ast_copy_text(literal->as.text);
    if (!value->as.literal.text) {
        mr_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering MIR literals.");
        return false;
    }
    return true;
}

bool mr_value_from_global(MirBuildContext *context,
                          const char *name,
                          CheckedType type,
                          MirValue *value) {
    if (!name || !value) {
        return false;
    }

    memset(value, 0, sizeof(*value));
    value->kind = MIR_VALUE_GLOBAL;
    value->type = type;
    value->as.global_name = ast_copy_text(name);
    if (!value->as.global_name) {
        mr_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering MIR globals.");
        return false;
    }
    return true;
}
