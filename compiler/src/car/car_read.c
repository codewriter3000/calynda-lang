#define _POSIX_C_SOURCE 200809L

#include "car.h"
#include "parser.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t read_uint32_le(const uint8_t *data) {
    return (uint32_t)data[0]
         | ((uint32_t)data[1] << 8)
         | ((uint32_t)data[2] << 16)
         | ((uint32_t)data[3] << 24);
}

static bool car_copy_type(AstType *dst, const AstType *src) {
    size_t i;

    if (!dst || !src) {
        return false;
    }

    memset(dst, 0, sizeof(*dst));
    dst->kind = src->kind;
    dst->primitive = src->primitive;
    if (src->name) {
        dst->name = ast_copy_text(src->name);
        if (!dst->name) {
            return false;
        }
    }
    for (i = 0; i < src->dimension_count; i++) {
        if (!ast_type_add_dimension(dst,
                                    src->dimensions[i].has_size,
                                    src->dimensions[i].size_literal)) {
            ast_type_free(dst);
            return false;
        }
    }
    for (i = 0; i < src->generic_args.count; i++) {
        AstGenericArg arg;

        memset(&arg, 0, sizeof(arg));
        arg.kind = src->generic_args.items[i].kind;
        if (arg.kind == AST_GENERIC_ARG_TYPE) {
            arg.type = calloc(1, sizeof(*arg.type));
            if (!arg.type || !car_copy_type(arg.type, src->generic_args.items[i].type)) {
                if (arg.type) {
                    ast_type_free(arg.type);
                    free(arg.type);
                }
                ast_type_free(dst);
                return false;
            }
        }
        if (!ast_type_add_generic_arg(dst, &arg)) {
            if (arg.type) {
                ast_type_free(arg.type);
                free(arg.type);
            }
            ast_type_free(dst);
            return false;
        }
    }

    return true;
}

static bool car_copy_parameter_list(AstParameterList *dst,
                                    const AstParameterList *src) {
    size_t i;

    if (!dst || !src) {
        return false;
    }

    ast_parameter_list_init(dst);
    for (i = 0; i < src->count; i++) {
        AstParameter parameter;

        memset(&parameter, 0, sizeof(parameter));
        parameter.is_varargs = src->items[i].is_varargs;
        parameter.name_span = src->items[i].name_span;
        if (!car_copy_type(&parameter.type, &src->items[i].type)) {
            ast_parameter_list_free(dst);
            return false;
        }
        if (src->items[i].name) {
            parameter.name = ast_copy_text(src->items[i].name);
            if (!parameter.name) {
                ast_type_free(&parameter.type);
                ast_parameter_list_free(dst);
                return false;
            }
        }
        if (!ast_parameter_list_append(dst, &parameter)) {
            ast_type_free(&parameter.type);
            free(parameter.name);
            ast_parameter_list_free(dst);
            return false;
        }
    }

    return true;
}

static bool car_exported_symbol_append(CarExportedSymbol **symbols,
                                       size_t *count,
                                       size_t *capacity,
                                       CarExportedSymbol *symbol) {
    CarExportedSymbol *resized;
    size_t new_capacity;

    if (!symbols || !count || !capacity || !symbol) {
        return false;
    }
    if (*count < *capacity) {
        (*symbols)[(*count)++] = *symbol;
        memset(symbol, 0, sizeof(*symbol));
        return true;
    }

    new_capacity = *capacity == 0 ? 4 : *capacity * 2;
    resized = realloc(*symbols, new_capacity * sizeof(*resized));
    if (!resized) {
        return false;
    }
    *symbols = resized;
    *capacity = new_capacity;
    (*symbols)[(*count)++] = *symbol;
    memset(symbol, 0, sizeof(*symbol));
    return true;
}

static bool car_qualified_name_matches(const AstQualifiedName *name,
                                       const char *package_name) {
    size_t i;
    size_t offset = 0;

    if (!name || !package_name) {
        return false;
    }
    if (name->count == 0) {
        return package_name[0] == '\0';
    }

    for (i = 0; i < name->count; i++) {
        size_t length = strlen(name->segments[i]);

        if (strncmp(package_name + offset, name->segments[i], length) != 0) {
            return false;
        }
        offset += length;
        if (i + 1 < name->count) {
            if (package_name[offset] != '.') {
                return false;
            }
            offset++;
        }
    }

    return package_name[offset] == '\0';
}

static bool car_decl_is_exported(const AstModifier *modifiers, size_t modifier_count) {
    return ast_decl_has_modifier(modifiers, modifier_count, AST_MODIFIER_EXPORT);
}

static bool car_collect_program_exports(const AstProgram *program,
                                        const char *package_name,
                                        CarExportedSymbol **symbols,
                                        size_t *count,
                                        size_t *capacity) {
    size_t i;

    if (!program || !package_name || !symbols || !count || !capacity) {
        return false;
    }
    if (!program->has_package ||
        !car_qualified_name_matches(&program->package_name, package_name)) {
        return true;
    }

    for (i = 0; i < program->top_level_count; i++) {
        const AstTopLevelDecl *decl = program->top_level_decls[i];
        CarExportedSymbol symbol;

        if (!decl) {
            continue;
        }

        memset(&symbol, 0, sizeof(symbol));
        symbol.package_name = ast_copy_text(package_name);
        if (!symbol.package_name) {
            return false;
        }

        switch (decl->kind) {
        case AST_TOP_LEVEL_BINDING:
            if (!car_decl_is_exported(decl->as.binding_decl.modifiers,
                                      decl->as.binding_decl.modifier_count)) {
                car_exported_symbol_free(&symbol);
                continue;
            }
            symbol.name = ast_copy_text(decl->as.binding_decl.name);
            if (!symbol.name ||
                !car_copy_type(&symbol.declared_type, &decl->as.binding_decl.declared_type)) {
                car_exported_symbol_free(&symbol);
                return false;
            }
            if (decl->as.binding_decl.initializer &&
                decl->as.binding_decl.initializer->kind == AST_EXPR_LAMBDA) {
                symbol.kind = CAR_EXPORTED_SYMBOL_CALLABLE;
                if (!car_copy_type(&symbol.return_type, &decl->as.binding_decl.declared_type) ||
                    !car_copy_parameter_list(&symbol.parameters,
                                             &decl->as.binding_decl.initializer->as.lambda.parameters)) {
                    car_exported_symbol_free(&symbol);
                    return false;
                }
            } else {
                symbol.kind = CAR_EXPORTED_SYMBOL_VALUE;
            }
            break;

        case AST_TOP_LEVEL_ASM:
            if (!car_decl_is_exported(decl->as.asm_decl.modifiers,
                                      decl->as.asm_decl.modifier_count)) {
                car_exported_symbol_free(&symbol);
                continue;
            }
            symbol.kind = CAR_EXPORTED_SYMBOL_CALLABLE;
            symbol.name = ast_copy_text(decl->as.asm_decl.name);
            if (!symbol.name ||
                !car_copy_type(&symbol.return_type, &decl->as.asm_decl.return_type) ||
                !car_copy_parameter_list(&symbol.parameters,
                                         &decl->as.asm_decl.parameters)) {
                car_exported_symbol_free(&symbol);
                return false;
            }
            break;

#include "car_read_p2.inc"
