#include "type_checker_internal.h"

/* type_checker_expr_array.c */
bool tc_check_array_literal(TypeChecker *checker,
                            const AstExpression *expression,
                            TypeCheckInfo *info);
static const AstParameterList tc_empty_parameter_list = { NULL, 0, 0 };

static const Symbol *tc_find_named_type_symbol(const TypeChecker *checker,
                                               CheckedType type) {
    const Scope *root_scope;
    const Symbol *symbol;

    if (!checker || type.kind != CHECKED_TYPE_NAMED || !type.name) {
        return NULL;
    }

    root_scope = symbol_table_root_scope(checker->symbols);
    symbol = root_scope ? scope_lookup_local(root_scope, type.name) : NULL;
    if (!symbol) {
        symbol = symbol_table_find_import(checker->symbols, type.name);
    }
    return symbol;
}

static bool tc_checked_type_is_named_type(CheckedType type, const char *name) {
    return type.kind == CHECKED_TYPE_NAMED &&
           type.array_depth == 0 &&
           type.name != NULL &&
           strcmp(type.name, name) == 0;
}

static bool tc_program_has_boot_entry(const TypeChecker *checker) {
    size_t i;

    if (!checker || !checker->program) {
        return false;
    }

    for (i = 0; i < checker->program->top_level_count; i++) {
        const AstTopLevelDecl *decl = checker->program->top_level_decls[i];

        if (decl &&
            decl->kind == AST_TOP_LEVEL_START &&
            decl->as.start_decl.is_boot) {
            return true;
        }
    }

    return false;
}

static bool tc_is_supported_freestanding_import_member(const Symbol *symbol,
                                                       const char *member) {
    return symbol &&
           (symbol->kind == SYMBOL_KIND_IMPORT ||
            symbol->kind == SYMBOL_KIND_PACKAGE) &&
           symbol->qualified_name &&
           member &&
           strcmp(symbol->qualified_name, "io.stdlib") == 0 &&
           (strcmp(member, "print") == 0 || strcmp(member, "input") == 0);
}

static void tc_report_freestanding_import_member_error(TypeChecker *checker,
                                                       const AstExpression *expression,
                                                       const Symbol *target_symbol) {
    const char *target_name = "<external>";
    const AstSourceSpan *related_span = NULL;

    if (!checker || !expression) {
        return;
    }

    if (target_symbol) {
        related_span = &target_symbol->declaration_span;
        if (target_symbol->name) {
            target_name = target_symbol->name;
        }
        tc_set_error_at(checker,
                        expression->source_span,
                        related_span,
                        "Freestanding boot() code cannot lower imported member '%s.%s' from '%s' statically. Only io.stdlib.print and io.stdlib.input are supported right now.",
                        target_name,
                        expression->as.member.member ? expression->as.member.member : "?",
                        target_symbol->qualified_name ? target_symbol->qualified_name : "<unknown>");
        return;
    }

    tc_set_error_at(checker,
                    expression->source_span,
                    NULL,
                    "Freestanding boot() code cannot lower external member '%s' statically. Only io.stdlib.print and io.stdlib.input are supported right now.",
                    expression->as.member.member ? expression->as.member.member : "?");
}

typedef struct {
    const Symbol **items;
    size_t         count;
    size_t         capacity;
} TcVisitedSymbolSet;

static bool tc_scope_is_within(const Scope *scope, const Scope *ancestor) {
    const Scope *walk = scope;

    while (walk) {
        if (walk == ancestor) {
            return true;
        }
        walk = walk->parent;
    }

    return false;
}

static bool tc_visited_contains(const TcVisitedSymbolSet *set,
                                const Symbol *symbol) {
    size_t i;

    if (!set || !symbol) {
        return false;
    }
    for (i = 0; i < set->count; i++) {
        if (set->items[i] == symbol) {
            return true;
        }
    }
    return false;
}

static void tc_visited_free(TcVisitedSymbolSet *set) {
    if (!set) {
        return;
    }
    free(set->items);
    memset(set, 0, sizeof(*set));
}

static bool tc_visited_push(TypeChecker *checker,
                            TcVisitedSymbolSet *set,
                            const Symbol *symbol,
                            AstSourceSpan source_span) {
    if (!set || !symbol || tc_visited_contains(set, symbol)) {
        return true;
    }

    if (!tc_reserve_items((void **)&set->items,
                          &set->capacity,
                          set->count + 1,
                          sizeof(*set->items))) {
        tc_set_error_at(checker,
                        source_span,
                        NULL,
                        "Out of memory while analyzing spawn callables.");
        return false;
    }

    set->items[set->count++] = symbol;
    return true;
}

static bool tc_symbol_is_atomic(TypeChecker *checker, const Symbol *symbol) {
    const TypeCheckInfo *symbol_info;

    if (!checker || !symbol) {
        return false;
    }

    symbol_info = type_checker_get_symbol_info(checker, symbol);
    if (!symbol_info) {
        symbol_info = tc_resolve_symbol_info(checker, symbol);
    }
    if (!symbol_info) {
        return false;
    }

    return tc_checked_type_is_named_type(symbol_info->type, "Atomic");
}

static bool tc_symbol_is_spawn_safe(TypeChecker *checker, const Symbol *symbol) {
    return symbol &&
           (symbol->is_final ||
            symbol->is_thread_local ||
            tc_symbol_is_atomic(checker, symbol));
}

static bool tc_emit_spawn_race_diagnostic(TypeChecker *checker,
                                          const AstExpression *expression,
                                          const Symbol *symbol) {
    if (!checker || !expression || !symbol || tc_symbol_is_spawn_safe(checker, symbol)) {
        return !checker || !checker->has_error;
    }

    if (type_checker_get_global_strict_race_check()) {
        tc_set_error_at(checker,
                        expression->source_span,
                        &symbol->declaration_span,
                        "Possible data race: spawned callable captures mutable symbol '%s'. Use final, thread_local, or Atomic<T> to share it safely.",
                        symbol->name ? symbol->name : "<anonymous>");
        return false;
    }

    tc_set_warning_at(checker,
                      expression->source_span,
                      &symbol->declaration_span,
                      "Possible data race: spawned callable captures mutable symbol '%s'. Use final, thread_local, or Atomic<T> to share it safely.",
                      symbol->name ? symbol->name : "<anonymous>");
    return true;
}

static bool tc_scan_spawn_expression_for_races(TypeChecker *checker,
                                               const AstExpression *expression,
                                               const Scope *spawn_scope,
                                               TcVisitedSymbolSet *visited);

static bool tc_scan_spawn_statement_for_races(TypeChecker *checker,
                                              const AstStatement *statement,
                                              const Scope *spawn_scope,
                                              TcVisitedSymbolSet *visited) {
    if (!statement) {
        return true;
    }

#include "type_checker_expr_more_p2.inc"
