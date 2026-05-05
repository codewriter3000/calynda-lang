    if (scope->overload_set_count < scope->overload_set_capacity) {
        scope->overload_sets[scope->overload_set_count++] = overload_set;
        return true;
    }

    new_capacity = (scope->overload_set_capacity == 0)
        ? 4
        : scope->overload_set_capacity * 2;
    resized = realloc(scope->overload_sets, new_capacity * sizeof(*scope->overload_sets));
    if (!resized) {
        st_set_error(table, "Out of memory while growing scope overload sets.");
        return false;
    }

    scope->overload_sets = resized;
    scope->overload_set_capacity = new_capacity;
    scope->overload_sets[scope->overload_set_count++] = overload_set;
    return true;
}

bool st_overload_set_append_symbol(SymbolTable *table,
                                   OverloadSet *overload_set,
                                   Symbol *symbol) {
    Symbol **resized;
    size_t new_capacity;

    if (!overload_set || !symbol) {
        return false;
    }

    if (overload_set->symbol_count < overload_set->symbol_capacity) {
        overload_set->symbols[overload_set->symbol_count++] = symbol;
        return true;
    }

    new_capacity = (overload_set->symbol_capacity == 0)
        ? 4
        : overload_set->symbol_capacity * 2;
    resized = realloc(overload_set->symbols, new_capacity * sizeof(*overload_set->symbols));
    if (!resized) {
        st_set_error(table, "Out of memory while growing overload-set members.");
        return false;
    }

    overload_set->symbols = resized;
    overload_set->symbol_capacity = new_capacity;
    overload_set->symbols[overload_set->symbol_count++] = symbol;
    return true;
}
