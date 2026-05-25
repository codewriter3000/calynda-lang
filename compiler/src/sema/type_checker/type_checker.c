#include "type_checker_internal.h"

static bool g_type_checker_strict_race_check = false;
static bool g_type_checker_performance_warnings = true;
static bool g_type_checker_performance_advisories = false;
static bool g_type_checker_size_focus = false;

void type_checker_init(TypeChecker *checker) {
    if (!checker) {
        return;
    }

    memset(checker, 0, sizeof(*checker));
}

void type_checker_free(TypeChecker *checker) {
    size_t i;

    if (!checker) {
        return;
    }

    type_resolver_free(&checker->resolver);
    for (i = 0; i < checker->expression_count; i++) {
        free(checker->expression_entries[i]);
    }
    for (i = 0; i < checker->symbol_count; i++) {
        free(checker->symbol_entries[i]);
    }
    for (i = 0; i < checker->owned_array_extent_block_count; i++) {
        free(checker->owned_array_extent_blocks[i]);
    }
    free(checker->expression_entries);
    free(checker->symbol_entries);
    free(checker->owned_array_extent_blocks);
    free(checker->warnings);
    free(checker->advisories);
    memset(checker, 0, sizeof(*checker));
}

const TypeCheckError *type_checker_get_error(const TypeChecker *checker) {
    if (!checker || !checker->has_error) {
        return NULL;
    }

    return &checker->error;
}

const TypeCheckError *type_checker_get_warning(const TypeChecker *checker) {
    return type_checker_get_warning_at(checker, 0);
}

const TypeCheckError *type_checker_get_warning_at(const TypeChecker *checker,
                                                  size_t index) {
    if (!checker || index >= checker->warning_count) {
        return NULL;
    }

    return &checker->warnings[index].error;
}

size_t type_checker_warning_count(const TypeChecker *checker) {
    return checker ? checker->warning_count : 0;
}

bool type_checker_warning_is_performance(const TypeChecker *checker,
                                         size_t index) {
    if (!checker || index >= checker->warning_count) {
        return false;
    }

    return checker->warnings[index].is_performance;
}

const TypeCheckError *type_checker_get_advisory(const TypeChecker *checker) {
    return type_checker_get_advisory_at(checker, 0);
}

const TypeCheckError *type_checker_get_advisory_at(const TypeChecker *checker,
                                                   size_t index) {
    if (!checker || index >= checker->advisory_count) {
        return NULL;
    }

    return &checker->advisories[index].error;
}

size_t type_checker_advisory_count(const TypeChecker *checker) {
    return checker ? checker->advisory_count : 0;
}

bool type_checker_advisory_is_performance(const TypeChecker *checker,
                                          size_t index) {
    if (!checker || index >= checker->advisory_count) {
        return false;
    }

    return checker->advisories[index].is_performance;
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

void type_checker_set_global_strict_race_check(bool enabled) {
    g_type_checker_strict_race_check = enabled;
}

bool type_checker_get_global_strict_race_check(void) {
    return g_type_checker_strict_race_check;
}

void type_checker_set_global_performance_warnings(bool enabled) {
    g_type_checker_performance_warnings = enabled;
}

bool type_checker_get_global_performance_warnings(void) {
    return g_type_checker_performance_warnings;
}

void type_checker_set_global_performance_advisories(bool enabled) {
    g_type_checker_performance_advisories = enabled;
}

bool type_checker_get_global_performance_advisories(void) {
    return g_type_checker_performance_advisories;
}

void type_checker_set_global_size_focus(bool enabled) {
    g_type_checker_size_focus = enabled;
}

bool type_checker_get_global_size_focus(void) {
    return g_type_checker_size_focus;
}

const TypeCheckInfo *type_checker_get_expression_info(const TypeChecker *checker,
                                                      const AstExpression *expression) {
    size_t i;

    if (!checker || !expression) {
        return NULL;
    }

    for (i = 0; i < checker->expression_count; i++) {
        if (checker->expression_entries[i] &&
            checker->expression_entries[i]->expression == expression) {
            return &checker->expression_entries[i]->info;
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
        if (checker->symbol_entries[i] &&
            checker->symbol_entries[i]->symbol == symbol &&
            checker->symbol_entries[i]->is_resolved) {
            return &checker->symbol_entries[i]->info;
        }
    }

    return NULL;
}
