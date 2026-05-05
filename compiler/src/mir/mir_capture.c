#include "mir_internal.h"

void mr_capture_list_free(MirCaptureList *captures) {
    if (!captures) {
        return;
    }

    free(captures->items);
    memset(captures, 0, sizeof(*captures));
}

bool mr_capture_list_add(MirBuildContext *context,
                         MirCaptureList *captures,
                         const Symbol *symbol,
                         CheckedType type,
                         AstSourceSpan source_span) {
    size_t i;

    if (!captures || !symbol) {
        return false;
    }

    for (i = 0; i < captures->count; i++) {
        if (captures->items[i].symbol == symbol) {
            return true;
        }
    }

    if (!mr_reserve_items((void **)&captures->items,
                       &captures->capacity,
                       captures->count + 1,
                       sizeof(*captures->items))) {
        mr_set_error(context,
                      source_span,
                      NULL,
                      "Out of memory while collecting MIR closure captures.");
        return false;
    }

    captures->items[captures->count].symbol = symbol;
    captures->items[captures->count].type = type;
    captures->items[captures->count].is_cell = false;
    captures->count++;
    return true;
}

void mr_bound_symbol_set_free(MirBoundSymbolSet *set) {
    if (!set) {
        return;
    }

    free(set->items);
    memset(set, 0, sizeof(*set));
}

bool mr_bound_symbol_set_contains(const MirBoundSymbolSet *set,
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

bool mr_bound_symbol_set_add(MirBuildContext *context,
                             MirBoundSymbolSet *set,
                             const Symbol *symbol,
                             AstSourceSpan source_span) {
    if (!set || !symbol) {
        return false;
    }

    if (mr_bound_symbol_set_contains(set, symbol)) {
        return true;
    }

    if (!mr_reserve_items((void **)&set->items,
                       &set->capacity,
                       set->count + 1,
                       sizeof(*set->items))) {
        mr_set_error(context,
                      source_span,
                      NULL,
                      "Out of memory while collecting MIR lambda symbols.");
        return false;
    }

    set->items[set->count++] = symbol;
    return true;
}

bool mr_bound_symbol_set_clone(MirBuildContext *context,
                               const MirBoundSymbolSet *source,
                               MirBoundSymbolSet *clone,
                               AstSourceSpan source_span) {
    if (!clone) {
        return false;
    }

    memset(clone, 0, sizeof(*clone));
    if (!source || source->count == 0) {
        return true;
    }

    clone->items = malloc(source->count * sizeof(*clone->items));
    if (!clone->items) {
        mr_set_error(context,
                      source_span,
                      NULL,
                      "Out of memory while cloning MIR lambda symbols.");
        return false;
    }

    memcpy(clone->items, source->items, source->count * sizeof(*clone->items));
    clone->count = source->count;
    clone->capacity = source->count;
    return true;
}
