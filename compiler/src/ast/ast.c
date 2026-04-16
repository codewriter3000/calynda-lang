#include "ast_internal.h"

bool ast_reserve_items(void **items, size_t *capacity,
                       size_t needed, size_t item_size) {
    size_t new_capacity;
    void *resized;

    if (needed <= *capacity) {
        return true;
    }

    new_capacity = (*capacity == 0) ? 4 : *capacity;
    while (new_capacity < needed) {
        if (new_capacity > SIZE_MAX / 2) {
            new_capacity = needed;
            break;
        }
        new_capacity *= 2;
    }

    if (item_size != 0 && new_capacity > SIZE_MAX / item_size) {
        return false;
    }

    resized = realloc(*items, new_capacity * item_size);
    if (!resized) {
        return false;
    }

    *items = resized;
    *capacity = new_capacity;
    return true;
}

char *ast_copy_text(const char *text) {
    if (!text) {
        return NULL;
    }
    return ast_copy_text_n(text, strlen(text));
}

char *ast_copy_text_n(const char *text, size_t length) {
    char *copy;

    if (!text || length == SIZE_MAX) {
        return NULL;
    }

    copy = malloc(length + 1);
    if (!copy) {
        return NULL;
    }

    if (length > 0) {
        memcpy(copy, text, length);
    }
    copy[length] = '\0';
    return copy;
}

void ast_qualified_name_init(AstQualifiedName *name) {
    if (!name) {
        return;
    }
    memset(name, 0, sizeof(*name));
}

void ast_qualified_name_free(AstQualifiedName *name) {
    size_t i;

    if (!name) {
        return;
    }

    for (i = 0; i < name->count; i++) {
        free(name->segments[i]);
    }

    free(name->segments);
    memset(name, 0, sizeof(*name));
}

bool ast_qualified_name_append(AstQualifiedName *name, const char *segment) {
    char *copy;

    if (!name || !segment) {
        return false;
    }

    copy = ast_copy_text(segment);
    if (!copy) {
        return false;
    }

    if (!ast_reserve_items((void **)&name->segments, &name->capacity,
                           name->count + 1, sizeof(*name->segments))) {
        free(copy);
        return false;
    }

    name->segments[name->count++] = copy;
    return true;
}

void ast_type_init_void(AstType *type) {
    if (!type) {
        return;
    }
    memset(type, 0, sizeof(*type));
    type->kind = AST_TYPE_VOID;
}

void ast_type_init_primitive(AstType *type, AstPrimitiveType primitive) {
    if (!type) {
        return;
    }
    memset(type, 0, sizeof(*type));
    type->kind = AST_TYPE_PRIMITIVE;
    type->primitive = primitive;
}

void ast_type_init_arr(AstType *type) {
    if (!type) {
        return;
    }
    memset(type, 0, sizeof(*type));
    type->kind = AST_TYPE_ARR;
}

void ast_type_init_named(AstType *type, const char *name) {
    if (!type) {
        return;
    }
    memset(type, 0, sizeof(*type));
    type->kind = AST_TYPE_NAMED;
    type->name = ast_copy_text(name);
}

void ast_type_init_thread(AstType *type) {
    if (!type) {
        return;
    }
    memset(type, 0, sizeof(*type));
    type->kind = AST_TYPE_THREAD;
}

void ast_type_init_mutex(AstType *type) {
    if (!type) {
        return;
    }
    memset(type, 0, sizeof(*type));
    type->kind = AST_TYPE_MUTEX;
}

void ast_type_free(AstType *type) {
    size_t i;

    if (!type) {
        return;
    }

    for (i = 0; i < type->dimension_count; i++) {
        free(type->dimensions[i].size_literal);
    }

    free(type->name);
    free(type->dimensions);
    ast_generic_arg_list_free(&type->generic_args);
    memset(type, 0, sizeof(*type));
}

bool ast_type_add_generic_arg(AstType *type, AstGenericArg *arg) {
    if (!type || !arg) {
        return false;
    }

    if (!ast_reserve_items((void **)&type->generic_args.items,
                           &type->generic_args.capacity,
                           type->generic_args.count + 1,
                           sizeof(*type->generic_args.items))) {
        return false;
    }

    type->generic_args.items[type->generic_args.count++] = *arg;
    memset(arg, 0, sizeof(*arg));
    return true;
}

void ast_generic_arg_list_free(AstGenericArgList *list) {
    size_t i;

    if (!list) {
        return;
    }

    for (i = 0; i < list->count; i++) {
        if (list->items[i].type) {
            ast_type_free(list->items[i].type);
            free(list->items[i].type);
        }
    }

    free(list->items);
    memset(list, 0, sizeof(*list));
}

bool ast_type_add_dimension(AstType *type, bool has_size, const char *size_literal) {
    AstArrayDimension dimension;

    if (!type) {
        return false;
    }

    memset(&dimension, 0, sizeof(dimension));
    dimension.has_size = has_size;
    if (has_size) {
        if (!size_literal) {
            return false;
        }

        dimension.size_literal = ast_copy_text(size_literal);
        if (!dimension.size_literal) {
            return false;
        }
    }

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
    }

    free(list->items);
    memset(list, 0, sizeof(*list));
}
