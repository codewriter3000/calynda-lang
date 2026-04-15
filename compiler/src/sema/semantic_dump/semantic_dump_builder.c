#include "semantic_dump_internal.h"

static bool builder_reserve(SemanticDumpBuilder *builder, size_t needed) {
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

bool sd_builder_append_n(SemanticDumpBuilder *builder, const char *text, size_t length) {
    if (!builder || !text) {
        return false;
    }

    if (!builder_reserve(builder, builder->length + length + 1)) {
        return false;
    }

    if (length > 0) {
        memcpy(builder->data + builder->length, text, length);
    }

    builder->length += length;
    builder->data[builder->length] = '\0';
    return true;
}

bool sd_builder_append(SemanticDumpBuilder *builder, const char *text) {
    return sd_builder_append_n(builder, text, strlen(text));
}

bool sd_builder_append_char(SemanticDumpBuilder *builder, char value) {
    return sd_builder_append_n(builder, &value, 1);
}

bool sd_builder_appendf(SemanticDumpBuilder *builder, const char *format, ...) {
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

    if (!builder_reserve(builder, builder->length + (size_t)written + 1)) {
        va_end(args);
        return false;
    }

    vsnprintf(builder->data + builder->length,
              builder->capacity - builder->length,
              format,
              args);
    builder->length += (size_t)written;
    va_end(args);
    return true;
}

bool sd_builder_indent(SemanticDumpBuilder *builder, int indent) {
    int i;

    for (i = 0; i < indent; i++) {
        if (!sd_builder_append(builder, "  ")) {
            return false;
        }
    }

    return true;
}

bool sd_builder_start_line(SemanticDumpBuilder *builder, int indent) {
    return sd_builder_indent(builder, indent);
}

bool sd_builder_finish_line(SemanticDumpBuilder *builder) {
    return sd_builder_append_char(builder, '\n');
}

const char *sd_primitive_type_name(AstPrimitiveType primitive) {
    switch (primitive) {
    case AST_PRIMITIVE_INT8:
        return "int8";
    case AST_PRIMITIVE_INT16:
        return "int16";
    case AST_PRIMITIVE_INT32:
        return "int32";
    case AST_PRIMITIVE_INT64:
        return "int64";
    case AST_PRIMITIVE_UINT8:
        return "uint8";
    case AST_PRIMITIVE_UINT16:
        return "uint16";
    case AST_PRIMITIVE_UINT32:
        return "uint32";
    case AST_PRIMITIVE_UINT64:
        return "uint64";
    case AST_PRIMITIVE_FLOAT32:
        return "float32";
    case AST_PRIMITIVE_FLOAT64:
        return "float64";
    case AST_PRIMITIVE_BOOL:
        return "bool";
    case AST_PRIMITIVE_CHAR:
        return "char";
    case AST_PRIMITIVE_STRING:
        return "string";
    case AST_PRIMITIVE_BYTE:
        return "byte";
    case AST_PRIMITIVE_SBYTE:
        return "sbyte";
    case AST_PRIMITIVE_SHORT:
        return "short";
    case AST_PRIMITIVE_INT:
        return "int";
    case AST_PRIMITIVE_LONG:
        return "long";
    case AST_PRIMITIVE_ULONG:
        return "ulong";
    case AST_PRIMITIVE_UINT:
        return "uint";
    case AST_PRIMITIVE_FLOAT:
        return "float";
    case AST_PRIMITIVE_DOUBLE:
        return "double";
    }

    return "unknown";
}

const char *sd_scope_kind_label(ScopeKind kind) {
    switch (kind) {
    case SCOPE_KIND_PROGRAM:
        return "program";
    case SCOPE_KIND_START:
        return "start";
    case SCOPE_KIND_LAMBDA:
        return "lambda";
    case SCOPE_KIND_BLOCK:
        return "block";
    case SCOPE_KIND_UNION:
        return "union";
    }

    return "unknown";
}

bool sd_source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

bool sd_builder_append_span(SemanticDumpBuilder *builder, AstSourceSpan span) {
    if (!sd_source_span_is_valid(span)) {
        return sd_builder_append(builder, "?");
    }

    return sd_builder_appendf(builder, "%d:%d", span.start_line, span.start_column);
}
