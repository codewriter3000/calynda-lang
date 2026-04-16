#include "ast_dump_internal.h"

bool ast_dump_top_level_decl(AstDumpBuilder *builder, const AstTopLevelDecl *decl,
                             int indent) {
    if (!decl) {
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "<null top-level declaration>") &&
               ast_dump_builder_finish_line(builder);
    }

    switch (decl->kind) {
    case AST_TOP_LEVEL_START:
        if (!(ast_dump_builder_start_line(builder, indent) &&
              ast_dump_builder_append(builder, "StartDecl") &&
              ast_dump_builder_finish_line(builder) &&
              ast_dump_parameter_list(builder, &decl->as.start_decl.parameters, indent + 1))) {
            return false;
        }

        if (decl->as.start_decl.body.kind == AST_LAMBDA_BODY_BLOCK) {
            return ast_dump_block_label(builder, indent + 1, "BodyBlock",
                                    decl->as.start_decl.body.as.block);
        }

        return ast_dump_expression_label(builder, indent + 1, "BodyExpr",
                                     decl->as.start_decl.body.as.expression);

    case AST_TOP_LEVEL_BINDING:
        if (!(ast_dump_builder_start_line(builder, indent) &&
              ast_dump_builder_append(builder, "BindingDecl name=") &&
              ast_dump_builder_append(builder,
                             decl->as.binding_decl.name ? decl->as.binding_decl.name : "<null>") &&
              ast_dump_builder_append(builder, " type=") &&
              ast_dump_type(builder, &decl->as.binding_decl.declared_type,
                        decl->as.binding_decl.is_inferred_type) &&
              ast_dump_builder_append(builder, " modifiers=") &&
              ast_dump_modifiers(builder, decl->as.binding_decl.modifiers,
                             decl->as.binding_decl.modifier_count) &&
              ast_dump_builder_finish_line(builder))) {
            return false;
        }

        return ast_dump_expression_label(builder, indent + 1, "Initializer",
                                     decl->as.binding_decl.initializer);

    case AST_TOP_LEVEL_UNION: {
        size_t m;

        if (!ast_dump_builder_start_line(builder, indent) ||
            !ast_dump_builder_append(builder, "UnionDecl name=") ||
            !ast_dump_builder_append(builder,
                            decl->as.union_decl.name ? decl->as.union_decl.name : "<null>")) {
            return false;
        }
        for (m = 0; m < decl->as.union_decl.modifier_count; m++) {
            const char *mod_name = NULL;
            switch (decl->as.union_decl.modifiers[m]) {
            case AST_MODIFIER_PUBLIC:   mod_name = "public";   break;
            case AST_MODIFIER_PRIVATE:  mod_name = "private";  break;
            case AST_MODIFIER_EXPORT:   mod_name = "export";   break;
            case AST_MODIFIER_FINAL:    mod_name = "final";    break;
            case AST_MODIFIER_STATIC:   mod_name = "static";   break;
            case AST_MODIFIER_INTERNAL: mod_name = "internal"; break;
            case AST_MODIFIER_THREAD_LOCAL: mod_name = "thread_local"; break;
            }
            if (mod_name && (!ast_dump_builder_append(builder, " ") ||
                             !ast_dump_builder_append(builder, mod_name))) {
                return false;
            }
        }
        return ast_dump_builder_finish_line(builder);
    }

    case AST_TOP_LEVEL_ASM: {
        char len_buf[32];

        if (!ast_dump_builder_start_line(builder, indent) ||
            !ast_dump_builder_append(builder, "AsmDecl name=") ||
            !ast_dump_builder_append(builder,
                            decl->as.asm_decl.name ? decl->as.asm_decl.name : "<null>") ||
            !ast_dump_builder_append(builder, " return=")) {
            return false;
        }
        if (!ast_dump_type(builder, &decl->as.asm_decl.return_type, false)) {
            return false;
        }
        if (!ast_dump_builder_append(builder, " modifiers=") ||
            !ast_dump_modifiers(builder, decl->as.asm_decl.modifiers,
                             decl->as.asm_decl.modifier_count)) {
            return false;
        }
        if (!ast_dump_builder_finish_line(builder)) {
            return false;
        }
        if (!ast_dump_parameter_list(builder, &decl->as.asm_decl.parameters, indent + 1)) {
            return false;
        }
        snprintf(len_buf, sizeof(len_buf), "%zu", decl->as.asm_decl.body_length);
        if (!(ast_dump_builder_start_line(builder, indent + 1) &&
              ast_dump_builder_append(builder, "AsmBody length=") &&
              ast_dump_builder_append(builder, len_buf) &&
              ast_dump_builder_finish_line(builder))) {
            return false;
        }
        return true;
    }

    case AST_TOP_LEVEL_LAYOUT: {
        size_t fi;
        if (!ast_dump_builder_start_line(builder, indent) ||
            !ast_dump_builder_append(builder, "LayoutDecl name=") ||
            !ast_dump_builder_append(builder,
                            decl->as.layout_decl.name ? decl->as.layout_decl.name : "<null>") ||
            !ast_dump_builder_finish_line(builder)) {
            return false;
        }
        for (fi = 0; fi < decl->as.layout_decl.field_count; fi++) {
            if (!ast_dump_builder_start_line(builder, indent + 1) ||
                !ast_dump_builder_append(builder, "Field name=") ||
                !ast_dump_builder_append(builder,
                    decl->as.layout_decl.fields[fi].name ?
                    decl->as.layout_decl.fields[fi].name : "<null>") ||
                !ast_dump_builder_finish_line(builder)) {
                return false;
            }
        }
        return true;
    }
    case AST_TOP_LEVEL_TYPE_ALIAS:
        if (!(ast_dump_builder_start_line(builder, indent) &&
              ast_dump_builder_append(builder, "TypeAliasDecl name=") &&
              ast_dump_builder_append(builder,
                             decl->as.type_alias_decl.name ? decl->as.type_alias_decl.name : "<null>") &&
              ast_dump_builder_append(builder, " type=") &&
              ast_dump_type(builder, &decl->as.type_alias_decl.target_type, false) &&
              ast_dump_builder_append(builder, " modifiers=") &&
              ast_dump_modifiers(builder, decl->as.type_alias_decl.modifiers,
                             decl->as.type_alias_decl.modifier_count) &&
              ast_dump_builder_finish_line(builder))) {
            return false;
        }
        return true;
    }

    return false;
}

bool ast_dump_program_node(AstDumpBuilder *builder, const AstProgram *program, int indent) {
    size_t i;

    if (!program) {
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "<null program>") &&
               ast_dump_builder_finish_line(builder);
    }

    if (!(ast_dump_builder_start_line(builder, indent) &&
          ast_dump_builder_append(builder, "Program") &&
          ast_dump_builder_finish_line(builder))) {
        return false;
    }

    if (program->has_package) {
        if (!(ast_dump_builder_start_line(builder, indent + 1) &&
              ast_dump_builder_append(builder, "PackageDecl: ") &&
              ast_dump_qualified_name(builder, &program->package_name) &&
              ast_dump_builder_finish_line(builder))) {
            return false;
        }
    }

    for (i = 0; i < program->import_count; i++) {
        const AstImportDecl *imp = &program->imports[i];

        if (!(ast_dump_builder_start_line(builder, indent + 1) &&
              ast_dump_builder_append(builder, "ImportDecl: "))) {
            return false;
        }

        switch (imp->kind) {
        case AST_IMPORT_PLAIN:
            if (!(ast_dump_qualified_name(builder, &imp->module_name) &&
                  ast_dump_builder_finish_line(builder))) {
                return false;
            }
            break;
        case AST_IMPORT_ALIAS:
            if (!(ast_dump_qualified_name(builder, &imp->module_name) &&
                  ast_dump_builder_append(builder, " as ") &&
                  ast_dump_builder_append(builder, imp->alias ? imp->alias : "<null>") &&
                  ast_dump_builder_finish_line(builder))) {
                return false;
            }
            break;
        case AST_IMPORT_WILDCARD:
            if (!(ast_dump_qualified_name(builder, &imp->module_name) &&
                  ast_dump_builder_append(builder, ".*") &&
                  ast_dump_builder_finish_line(builder))) {
                return false;
            }
            break;
        case AST_IMPORT_SELECTIVE: {
            size_t j;
            if (!(ast_dump_qualified_name(builder, &imp->module_name) &&
                  ast_dump_builder_append(builder, ".{"))) {
                return false;
            }
            for (j = 0; j < imp->selected_count; j++) {
                if (j > 0 && !ast_dump_builder_append(builder, ", ")) {
                    return false;
                }
                if (!ast_dump_builder_append(builder, imp->selected_names[j])) {
                    return false;
                }
            }
            if (!(ast_dump_builder_append(builder, "}") &&
                  ast_dump_builder_finish_line(builder))) {
                return false;
            }
            break;
        }
        }
    }

    for (i = 0; i < program->top_level_count; i++) {
        if (!ast_dump_top_level_decl(builder, program->top_level_decls[i], indent + 1)) {
            return false;
        }
    }

    return true;
}
