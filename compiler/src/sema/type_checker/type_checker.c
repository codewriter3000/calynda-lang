#include "type_checker_internal.h"

void type_checker_init(TypeChecker *checker) {
    if (!checker) {
        return;
    }

    memset(checker, 0, sizeof(*checker));
}

void type_checker_free(TypeChecker *checker) {
    if (!checker) {
        return;
    }

    type_resolver_free(&checker->resolver);
    free(checker->expression_entries);
    free(checker->symbol_entries);
    memset(checker, 0, sizeof(*checker));
}

const TypeCheckError *type_checker_get_error(const TypeChecker *checker) {
    if (!checker || !checker->has_error) {
        return NULL;
    }

    return &checker->error;
}

bool type_checker_format_error(const TypeCheckError *error,
                               char *buffer,
                               size_t buffer_size) {
    int written;

    if (!error || !buffer || buffer_size == 0) {
        return false;
    }

    if (tc_source_span_is_valid(error->primary_span)) {
        written = snprintf(buffer, buffer_size, "%d:%d: %s",
                           error->primary_span.start_line,
                           error->primary_span.start_column,
                           error->message);
    } else {
        written = snprintf(buffer, buffer_size, "%s", error->message);
    }

    if (written < 0 || (size_t)written >= buffer_size) {
        return false;
    }

    if (error->has_related_span && tc_source_span_is_valid(error->related_span)) {
        written += snprintf(buffer + written, buffer_size - (size_t)written,
                            " Related location at %d:%d.",
                            error->related_span.start_line,
                            error->related_span.start_column);
        if (written < 0 || (size_t)written >= buffer_size) {
            return false;
        }
    }

    return true;
}

const TypeCheckInfo *type_checker_get_expression_info(const TypeChecker *checker,
                                                      const AstExpression *expression) {
    size_t i;

    if (!checker || !expression) {
        return NULL;
    }

    for (i = 0; i < checker->expression_count; i++) {
        if (checker->expression_entries[i].expression == expression) {
            return &checker->expression_entries[i].info;
        }
    }

    return NULL;
}

const TypeCheckInfo *type_checker_get_symbol_info(const TypeChecker *checker,
                                                  const Symbol *symbol) {
    size_t i;

    if (!checker || !symbol) {
        return NULL;
    }

    for (i = 0; i < checker->symbol_count; i++) {
        if (checker->symbol_entries[i].symbol == symbol &&
            checker->symbol_entries[i].is_resolved) {
            return &checker->symbol_entries[i].info;
        }
    }

    return NULL;
}
