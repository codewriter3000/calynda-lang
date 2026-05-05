    if (!ast_reserve_items((void **)&type->dimensions, &type->dimension_capacity,
                           type->dimension_count + 1, sizeof(*type->dimensions))) {
        free(dimension.size_literal);
        return false;
    }

    type->dimensions[type->dimension_count++] = dimension;
    return true;
}

void ast_parameter_list_init(AstParameterList *list) {
    if (!list) {
        return;
    }
    memset(list, 0, sizeof(*list));
}

void ast_parameter_list_free(AstParameterList *list) {
    size_t i;

    if (!list) {
        return;
    }

    for (i = 0; i < list->count; i++) {
        ast_type_free(&list->items[i].type);
        free(list->items[i].name);
        ast_expression_free(list->items[i].default_expr);
    }

    free(list->items);
    memset(list, 0, sizeof(*list));
}
