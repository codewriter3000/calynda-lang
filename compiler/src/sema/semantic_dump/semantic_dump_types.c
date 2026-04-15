#include "semantic_dump_internal.h"

bool sd_append_ast_type(SemanticDumpBuilder *builder,
                        const AstType *type,
                        bool is_inferred) {
    size_t i;

    if (is_inferred) {
        return sd_builder_append(builder, "var");
    }

    if (!type) {
        return sd_builder_append(builder, "<none>");
    }

    switch (type->kind) {
    case AST_TYPE_VOID:
        if (!sd_builder_append(builder, "void")) {
            return false;
        }
        break;
    case AST_TYPE_PRIMITIVE:
        if (!sd_builder_append(builder, sd_primitive_type_name(type->primitive))) {
            return false;
        }
        break;
    case AST_TYPE_ARR:
        if (!sd_builder_append(builder, "arr")) {
            return false;
        }
        if (type->generic_args.count > 0) {
            if (!sd_builder_append(builder, "<")) {
                return false;
            }
            for (i = 0; i < type->generic_args.count; i++) {
                if (i > 0 && !sd_builder_append(builder, ", ")) {
                    return false;
                }
                if (type->generic_args.items[i].kind == AST_GENERIC_ARG_WILDCARD) {
                    if (!sd_builder_append(builder, "?")) {
                        return false;
                    }
                } else if (type->generic_args.items[i].type) {
                    if (!sd_append_ast_type(builder, type->generic_args.items[i].type, false)) {
                        return false;
                    }
                }
            }
            if (!sd_builder_append(builder, ">")) {
                return false;
            }
        }
        break;
    case AST_TYPE_NAMED:
        if (!sd_builder_append(builder, type->name ? type->name : "?")) {
            return false;
        }
        break;
    }

    for (i = 0; i < type->dimension_count; i++) {
        if (type->dimensions[i].has_size && type->dimensions[i].size_literal) {
            if (!sd_builder_appendf(builder, "[%s]", type->dimensions[i].size_literal)) {
                return false;
            }
        } else if (!sd_builder_append(builder, "[]")) {
            return false;
        }
    }

    return true;
}

bool sd_append_checked_type(SemanticDumpBuilder *builder, CheckedType type) {
    size_t i;

    switch (type.kind) {
    case CHECKED_TYPE_INVALID:
        return sd_builder_append(builder, "<invalid>");
    case CHECKED_TYPE_VOID:
        return sd_builder_append(builder, "void");
    case CHECKED_TYPE_NULL:
        return sd_builder_append(builder, "null");
    case CHECKED_TYPE_EXTERNAL:
        return sd_builder_append(builder, "<external>");
    case CHECKED_TYPE_VALUE:
        if (!sd_builder_append(builder, sd_primitive_type_name(type.primitive))) {
            return false;
        }
        for (i = 0; i < type.array_depth; i++) {
            if (!sd_builder_append(builder, "[]")) {
                return false;
            }
        }
        return true;
    case CHECKED_TYPE_NAMED:
        if (!sd_builder_append(builder, type.name ? type.name : "?")) {
            return false;
        }
        if (type.generic_arg_count > 0) {
            if (type.name && strcmp(type.name, "arr") == 0 && type.generic_arg_count == 1) {
                if (!sd_builder_append(builder, "<?>")) {
                    return false;
                }
            } else {
                if (!sd_builder_appendf(builder, "<...%zu>", type.generic_arg_count)) {
                    return false;
                }
            }
        }
        for (i = 0; i < type.array_depth; i++) {
            if (!sd_builder_append(builder, "[]")) {
                return false;
            }
        }
        return true;
    case CHECKED_TYPE_TYPE_PARAM:
        return sd_builder_append(builder, type.name ? type.name : "?");
    }

    return false;
}

bool sd_append_effective_symbol_type(SemanticDumpBuilder *builder,
                                     const Symbol *symbol,
                                     const TypeChecker *checker) {
    const TypeCheckInfo *info;

    if (!symbol) {
        return sd_builder_append(builder, "<unknown>");
    }

    info = checker ? type_checker_get_symbol_info(checker, symbol) : NULL;
    if (info) {
        return sd_append_checked_type(builder, info->type);
    }

    if (symbol->kind == SYMBOL_KIND_PACKAGE || symbol->kind == SYMBOL_KIND_IMPORT) {
        return sd_builder_append(builder, "<external>");
    }

    if (!symbol->is_inferred_type && symbol->declared_type) {
        return sd_append_ast_type(builder, symbol->declared_type, false);
    }

    return sd_builder_append(builder, "<untyped>");
}

bool sd_append_callable_signature(SemanticDumpBuilder *builder,
                                  const TypeCheckInfo *info) {
    size_t i;

    if (!info || !info->is_callable) {
        return true;
    }

    if (!sd_builder_append(builder, " callable=(")) {
        return false;
    }

    if (!info->parameters) {
        if (!sd_builder_append(builder, "...")) {
            return false;
        }
    } else {
        for (i = 0; i < info->parameters->count; i++) {
            if (i > 0 && !sd_builder_append(builder, ", ")) {
                return false;
            }
            if (!sd_append_ast_type(builder, &info->parameters->items[i].type, false)) {
                return false;
            }
            if (info->parameters->items[i].is_varargs) {
                if (!sd_builder_append(builder, "...")) {
                    return false;
                }
            }
        }
    }

    if (!sd_builder_append(builder, ") -> ")) {
        return false;
    }

    return sd_append_checked_type(builder, info->callable_return_type);
}

bool sd_append_symbol_reference(SemanticDumpBuilder *builder,
                                const Symbol *symbol,
                                const TypeChecker *checker) {
    const TypeCheckInfo *info;

    if (!symbol) {
        return sd_builder_append(builder, "<unknown>");
    }

    if (!sd_builder_append(builder, symbol_kind_name(symbol->kind))) {
        return false;
    }

    if (symbol->name && !sd_builder_appendf(builder, " %s", symbol->name)) {
        return false;
    }

    if (symbol->qualified_name && !sd_builder_appendf(builder, " qualified=%s", symbol->qualified_name)) {
        return false;
    }

    if (!sd_builder_append(builder, " type=")) {
        return false;
    }
    if (!sd_append_effective_symbol_type(builder, symbol, checker)) {
        return false;
    }

    info = checker ? type_checker_get_symbol_info(checker, symbol) : NULL;
    if (info && !sd_append_callable_signature(builder, info)) {
        return false;
    }

    return true;
}
