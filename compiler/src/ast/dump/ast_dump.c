#include "ast_dump_internal.h"

#include <stdio.h>

bool ast_dump_builder_reserve(AstDumpBuilder *builder, size_t needed) {
    char *resized;
    size_t new_capacity;

    if (needed <= builder->capacity) {
        return true;
    }

    new_capacity = (builder->capacity == 0) ? 64 : builder->capacity;
    while (new_capacity < needed) {
        if (new_capacity > SIZE_MAX / 2) {
            new_capacity = needed;
            break;
        }
        new_capacity *= 2;
    }

    resized = realloc(builder->data, new_capacity);
    if (!resized) {
        return false;
    }

    builder->data = resized;
    builder->capacity = new_capacity;
    return true;
}

bool ast_dump_builder_append_n(AstDumpBuilder *builder, const char *text, size_t length) {
    if (!builder || !text) {
        return false;
    }

    if (!ast_dump_builder_reserve(builder, builder->length + length + 1)) {
        return false;
    }

    if (length > 0) {
        memcpy(builder->data + builder->length, text, length);
    }
    builder->length += length;
    builder->data[builder->length] = '\0';
    return true;
}

bool ast_dump_builder_append(AstDumpBuilder *builder, const char *text) {
    return ast_dump_builder_append_n(builder, text, strlen(text));
}

bool ast_dump_builder_append_char(AstDumpBuilder *builder, char value) {
    return ast_dump_builder_append_n(builder, &value, 1);
}

bool ast_dump_builder_appendf(AstDumpBuilder *builder, const char *format, ...) {
    int written;
    va_list args;
    va_list copied_args;

    va_start(args, format);
    va_copy(copied_args, args);
    written = vsnprintf(NULL, 0, format, copied_args);
    va_end(copied_args);

    if (written < 0) {
        va_end(args);
        return false;
    }

    if (!ast_dump_builder_reserve(builder, builder->length + (size_t)written + 1)) {
        va_end(args);
        return false;
    }

    vsnprintf(builder->data + builder->length,
              builder->capacity - builder->length, format, args);
    builder->length += (size_t)written;
    va_end(args);
    return true;
}

bool ast_dump_builder_indent(AstDumpBuilder *builder, int indent) {
    int i;

    for (i = 0; i < indent; i++) {
        if (!ast_dump_builder_append(builder, "  ")) {
            return false;
        }
    }

    return true;
}

bool ast_dump_builder_start_line(AstDumpBuilder *builder, int indent) {
    return ast_dump_builder_indent(builder, indent);
}

bool ast_dump_builder_finish_line(AstDumpBuilder *builder) {
    return ast_dump_builder_append_char(builder, '\n');
}

bool ast_dump_builder_append_quoted(AstDumpBuilder *builder, const char *text) {
    const unsigned char *current = (const unsigned char *)text;

    if (!ast_dump_builder_append_char(builder, '"')) {
        return false;
    }

    while (current && *current != '\0') {
        switch (*current) {
        case '\\':
            if (!ast_dump_builder_append(builder, "\\\\")) {
                return false;
            }
            break;
        case '"':
            if (!ast_dump_builder_append(builder, "\\\"")) {
                return false;
            }
            break;
        case '\n':
            if (!ast_dump_builder_append(builder, "\\n")) {
                return false;
            }
            break;
        case '\r':
            if (!ast_dump_builder_append(builder, "\\r")) {
                return false;
            }
            break;
        case '\t':
            if (!ast_dump_builder_append(builder, "\\t")) {
                return false;
            }
            break;
        default:
            if (isprint(*current)) {
                if (!ast_dump_builder_append_char(builder, (char)*current)) {
                    return false;
                }
            } else if (!ast_dump_builder_appendf(builder, "\\x%02X", *current)) {
                return false;
            }
            break;
        }

        current++;
    }

    return ast_dump_builder_append_char(builder, '"');
}

bool ast_dump_program(FILE *out, const AstProgram *program) {
    char *dump;
    int status;

    if (!out) {
        return false;
    }

    dump = ast_dump_program_to_string(program);
    if (!dump) {
        return false;
    }

    status = fputs(dump, out);
    free(dump);
    return status >= 0;
}

bool ast_dump_expression(FILE *out, const AstExpression *expression) {
    char *dump;
    int status;

    if (!out) {
        return false;
    }

    dump = ast_dump_expression_to_string(expression);
    if (!dump) {
        return false;
    }

    status = fputs(dump, out);
    free(dump);
    return status >= 0;
}

char *ast_dump_program_to_string(const AstProgram *program) {
    AstDumpBuilder builder;

    memset(&builder, 0, sizeof(builder));
    if (!ast_dump_builder_reserve(&builder, 1)) {
        return NULL;
    }
    builder.data[0] = '\0';

    if (!ast_dump_program_node(&builder, program, 0)) {
        free(builder.data);
        return NULL;
    }

    return builder.data;
}

char *ast_dump_expression_to_string(const AstExpression *expression) {
    AstDumpBuilder builder;

    memset(&builder, 0, sizeof(builder));
    if (!ast_dump_builder_reserve(&builder, 1)) {
        return NULL;
    }
    builder.data[0] = '\0';

    if (!ast_dump_expression_node(&builder, expression, 0)) {
        free(builder.data);
        return NULL;
    }

    return builder.data;
}
