#include "bytecode_dump_internal.h"

bool bytecode_dump_write_checked_type(FILE *out, CheckedType type) {
    char buffer[64];

    if (!checked_type_to_string(type, buffer, sizeof(buffer))) {
        return false;
    }

    return fputs(buffer, out) != EOF;
}

bool bytecode_dump_write_quoted_text(FILE *out, const char *text) {
    const unsigned char *cursor = (const unsigned char *)(text ? text : "");

    if (fputc('"', out) == EOF) {
        return false;
    }
    while (*cursor) {
        switch (*cursor) {
        case '\\':
            if (fputs("\\\\", out) == EOF) {
                return false;
            }
            break;
        case '"':
            if (fputs("\\\"", out) == EOF) {
                return false;
            }
            break;
        case '\n':
            if (fputs("\\n", out) == EOF) {
                return false;
            }
            break;
        case '\r':
            if (fputs("\\r", out) == EOF) {
                return false;
            }
            break;
        case '\t':
            if (fputs("\\t", out) == EOF) {
                return false;
            }
            break;
        default:
            if (fputc((int)*cursor, out) == EOF) {
                return false;
            }
            break;
        }
        cursor++;
    }
    return fputc('"', out) != EOF;
}

bool bytecode_dump_constant_ref(FILE *out, const BytecodeProgram *program,
                                size_t constant_index) {
    if (!out || !program || constant_index >= program->constant_count) {
        return false;
    }

    fprintf(out, "c%zu:", constant_index);
    switch (program->constants[constant_index].kind) {
    case BYTECODE_CONSTANT_LITERAL:
        fprintf(out, "%s",
                bytecode_dump_literal_kind_name(
                    program->constants[constant_index].as.literal.kind));
        if (program->constants[constant_index].as.literal.kind == AST_LITERAL_BOOL) {
            fprintf(out,
                    "(%s)",
                    program->constants[constant_index].as.literal.bool_value ?
                        "true" : "false");
        } else if (program->constants[constant_index].as.literal.text) {
            fputc('(', out);
            if (!bytecode_dump_write_quoted_text(out,
                    program->constants[constant_index].as.literal.text)) {
                return false;
            }
            fputc(')', out);
        }
        return true;
    case BYTECODE_CONSTANT_SYMBOL:
        return bytecode_dump_write_quoted_text(out,
                program->constants[constant_index].as.text);
    case BYTECODE_CONSTANT_TEXT:
        return bytecode_dump_write_quoted_text(out,
                program->constants[constant_index].as.text);
    case BYTECODE_CONSTANT_TYPE_DESCRIPTOR:
        return fprintf(out,
                       "type_desc(%s)",
                       program->constants[constant_index].as.type_descriptor.name) >= 0;
    }

    return false;
}

bool bytecode_dump_value(FILE *out,
                         const BytecodeProgram *program,
                         const BytecodeUnit *unit,
                         BytecodeValue value) {
    if (!out || !program || !unit) {
        return false;
    }

    switch (value.kind) {
    case BYTECODE_VALUE_INVALID:
        return fputs("<invalid>", out) != EOF;
    case BYTECODE_VALUE_TEMP:
        return fprintf(out, "t%zu", value.as.temp_index) >= 0;
    case BYTECODE_VALUE_LOCAL:
        return fprintf(out,
                       "local(%zu:%s)",
                       value.as.local_index,
                       unit->locals[value.as.local_index].name) >= 0;
    case BYTECODE_VALUE_GLOBAL:
        return fprintf(out, "global(") >= 0 &&
               bytecode_dump_constant_ref(out, program, value.as.global_index) &&
               fprintf(out, ")") >= 0;
    case BYTECODE_VALUE_CONSTANT:
        return fprintf(out, "const(") >= 0 &&
               bytecode_dump_constant_ref(out, program, value.as.constant_index) &&
               fprintf(out, ")") >= 0;
    }

    return false;
}

bool bytecode_dump_template_part(FILE *out,
                                 const BytecodeProgram *program,
                                 const BytecodeUnit *unit,
                                 const BytecodeTemplatePart *part) {
    if (!out || !program || !unit || !part) {
        return false;
    }

    if (part->kind == BYTECODE_TEMPLATE_PART_TEXT) {
        return fprintf(out, "text(") >= 0 &&
               bytecode_dump_constant_ref(out, program, part->as.text_index) &&
               fprintf(out, ")") >= 0;
    }

    return fprintf(out, "value(") >= 0 &&
           bytecode_dump_value(out, program, unit, part->as.value) &&
           fprintf(out, ")") >= 0;
}
