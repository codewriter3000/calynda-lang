#include "hir_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void hir_program_init(HirProgram *program) {
    if (!program) {
        return;
    }

    memset(program, 0, sizeof(*program));
}

void hir_program_free(HirProgram *program) {
    size_t i;

    if (!program) {
        return;
    }

    if (program->has_package) {
        hr_free_named_symbol(&program->package);
    }

    for (i = 0; i < program->import_count; i++) {
        hr_free_named_symbol(&program->imports[i]);
    }
    free(program->imports);

    for (i = 0; i < program->top_level_count; i++) {
        HirTopLevelDecl *decl = program->top_level_decls[i];

        if (!decl) {
            continue;
        }

        if (decl->kind == HIR_TOP_LEVEL_BINDING) {
            free(decl->as.binding.name);
            hr_free_callable_signature(&decl->as.binding.callable_signature);
            if (decl->as.binding.initializer) {
                hir_expression_free(decl->as.binding.initializer);
            }
        } else if (decl->kind == HIR_TOP_LEVEL_UNION) {
            size_t v;
            for (v = 0; v < decl->as.union_decl.generic_param_count; v++) {
                free(decl->as.union_decl.generic_params[v]);
            }
            free(decl->as.union_decl.generic_params);
            for (v = 0; v < decl->as.union_decl.variant_count; v++) {
                free(decl->as.union_decl.variants[v].name);
            }
            free(decl->as.union_decl.variants);
            free(decl->as.union_decl.name);
        } else if (decl->kind == HIR_TOP_LEVEL_ASM) {
            size_t p;
            free(decl->as.asm_decl.name);
            for (p = 0; p < decl->as.asm_decl.parameter_count; p++) {
                free(decl->as.asm_decl.parameter_names[p]);
            }
            free(decl->as.asm_decl.parameter_names);
            free(decl->as.asm_decl.parameter_types);
            free(decl->as.asm_decl.body);
        } else {
            hr_free_parameter_list(&decl->as.start.parameters);
            if (decl->as.start.body) {
                hir_block_free(decl->as.start.body);
            }
        }

        free(decl);
    }
    free(program->top_level_decls);
    memset(program, 0, sizeof(*program));
}

const HirBuildError *hir_get_error(const HirProgram *program) {
    if (!program || !program->has_error) {
        return NULL;
    }

    return &program->error;
}

bool hir_format_error(const HirBuildError *error,
                      char *buffer,
                      size_t buffer_size) {
    int written;

    if (!error || !buffer || buffer_size == 0) {
        return false;
    }

    if (hr_source_span_is_valid(error->primary_span)) {
        if (error->has_related_span && hr_source_span_is_valid(error->related_span)) {
            written = snprintf(buffer,
                               buffer_size,
                               "%d:%d: %s Related location at %d:%d.",
                               error->primary_span.start_line,
                               error->primary_span.start_column,
                               error->message,
                               error->related_span.start_line,
                               error->related_span.start_column);
        } else {
            written = snprintf(buffer,
                               buffer_size,
                               "%d:%d: %s",
                               error->primary_span.start_line,
                               error->primary_span.start_column,
                               error->message);
        }
    } else {
        written = snprintf(buffer, buffer_size, "%s", error->message);
    }

    return written >= 0 && (size_t)written < buffer_size;
}

bool hir_build_program(HirProgram *program,
                       const AstProgram *ast_program,
                       const SymbolTable *symbols,
                       const TypeChecker *checker) {
    HirBuildContext context;
    const TypeCheckError *checker_error;
    size_t i;

    if (!program || !ast_program || !symbols || !checker) {
        return false;
    }

    hir_program_free(program);
    hir_program_init(program);

    memset(&context, 0, sizeof(context));
    context.program = program;
    context.ast_program = ast_program;
    context.symbols = symbols;
    context.checker = checker;

    checker_error = type_checker_get_error(checker);
    if (checker_error != NULL) {
        hr_set_error(&context,
                     checker_error->primary_span,
                     checker_error->has_related_span
                         ? &checker_error->related_span
                         : NULL,
                     "%s",
                     checker_error->message);
        return false;
    }

    if (ast_program->has_package) {
        const Symbol *package_symbol = symbol_table_get_package(symbols);

        if (!package_symbol) {
            hr_set_error(&context,
                         ast_program->package_name.tail_span,
                         NULL,
                         "Internal error: missing package symbol during HIR lowering.");
            return false;
        }

        program->has_package = true;
        program->package.symbol = package_symbol;
        program->package.name = ast_copy_text(ast_program->package_name.segments[
            ast_program->package_name.count - 1]);
        program->package.qualified_name = hr_qualified_name_to_string(&ast_program->package_name);
        program->package.source_span = ast_program->package_name.tail_span;
        if (!program->package.name || !program->package.qualified_name) {
            hr_set_error(&context,
                         ast_program->package_name.tail_span,
                         NULL,
                         "Out of memory while lowering HIR package metadata.");
            return false;
        }
    }

    for (i = 0; i < ast_program->import_count; i++) {
        const Symbol *import_symbol = symbol_table_get_import(symbols, i);
        HirNamedSymbol import_entry;

        if (!import_symbol) {
            hr_set_error(&context,
                         ast_program->imports[i].module_name.tail_span,
                         NULL,
                         "Internal error: missing import symbol during HIR lowering.");
            return false;
        }

        memset(&import_entry, 0, sizeof(import_entry));
        import_entry.symbol = import_symbol;
        import_entry.name = ast_copy_text(ast_program->imports[i].module_name.segments[
            ast_program->imports[i].module_name.count - 1]);
        import_entry.qualified_name = hr_qualified_name_to_string(&ast_program->imports[i].module_name);
        import_entry.source_span = ast_program->imports[i].module_name.tail_span;
        if (!import_entry.name || !import_entry.qualified_name ||
            !hr_append_named_symbol(&program->imports,
                                    &program->import_count,
                                    &program->import_capacity,
                                    import_entry)) {
            hr_free_named_symbol(&import_entry);
            hr_set_error(&context,
                         ast_program->imports[i].module_name.tail_span,
                         NULL,
                         "Out of memory while lowering HIR imports.");
            return false;
        }
    }

    return hr_lower_top_level_decls(&context);
}
