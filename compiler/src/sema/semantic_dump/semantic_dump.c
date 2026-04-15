#include "semantic_dump_internal.h"

bool semantic_dump_program(FILE *out,
                           const SymbolTable *symbols,
                           const TypeChecker *checker) {
    char *dump;
    int written;

    if (!out) {
        return false;
    }

    dump = semantic_dump_program_to_string(symbols, checker);
    if (!dump) {
        return false;
    }

    written = fputs(dump, out);
    free(dump);
    return written >= 0;
}

char *semantic_dump_program_to_string(const SymbolTable *symbols,
                                      const TypeChecker *checker) {
    SemanticDumpBuilder builder = {0};
    const TypeCheckError *type_error;
    char error_buffer[256];

    if (!symbols) {
        return NULL;
    }

    if (!sd_builder_start_line(&builder, 0) ||
        !sd_builder_append(&builder, "SemanticProgram") ||
        !sd_builder_finish_line(&builder) ||
        !sd_dump_package(&builder, symbols, checker, 1) ||
        !sd_dump_imports(&builder, symbols, checker, 1)) {
        free(builder.data);
        return NULL;
    }

    type_error = checker ? type_checker_get_error(checker) : NULL;
    if (type_error) {
        if (!type_checker_format_error(type_error, error_buffer, sizeof(error_buffer))) {
            strncpy(error_buffer, type_error->message, sizeof(error_buffer) - 1);
            error_buffer[sizeof(error_buffer) - 1] = '\0';
        }
        if (!sd_builder_start_line(&builder, 1) ||
            !sd_builder_appendf(&builder, "TypeCheckError: %s", error_buffer) ||
            !sd_builder_finish_line(&builder)) {
            free(builder.data);
            return NULL;
        }
    }

    if (!sd_builder_start_line(&builder, 1) || !sd_builder_append(&builder, "Scopes:")) {
        free(builder.data);
        return NULL;
    }
    if (!sd_builder_finish_line(&builder)) {
        free(builder.data);
        return NULL;
    }

    if (!symbols->root_scope) {
        if (!sd_builder_start_line(&builder, 2) ||
            !sd_builder_append(&builder, "<none>") ||
            !sd_builder_finish_line(&builder)) {
            free(builder.data);
            return NULL;
        }
        return builder.data;
    }

    if (!sd_dump_scope(&builder, symbols, checker, symbols->root_scope, 2)) {
        free(builder.data);
        return NULL;
    }

    return builder.data;
}