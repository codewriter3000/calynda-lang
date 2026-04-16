#include "ast_dump_internal.h"

bool ast_dump_statement(AstDumpBuilder *builder, const AstStatement *statement,
                        int indent) {
    if (!statement) {
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "<null statement>") &&
               ast_dump_builder_finish_line(builder);
    }

    switch (statement->kind) {
    case AST_STMT_LOCAL_BINDING:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "LocalBindingStmt name=") &&
               ast_dump_builder_append(builder,
                              statement->as.local_binding.name
                                  ? statement->as.local_binding.name
                                  : "<null>") &&
               ast_dump_builder_append(builder, " type=") &&
               ast_dump_type(builder, &statement->as.local_binding.declared_type,
                         statement->as.local_binding.is_inferred_type) &&
               ast_dump_builder_append(builder,
                              statement->as.local_binding.is_final
                                  ? " final=true"
                                  : " final=false") &&
               (!statement->as.local_binding.is_internal ||
                ast_dump_builder_append(builder, " internal=true")) &&
               ast_dump_builder_finish_line(builder) &&
               ast_dump_expression_label(builder, indent + 1, "Initializer",
                                     statement->as.local_binding.initializer);

    case AST_STMT_RETURN:
        if (!(ast_dump_builder_start_line(builder, indent) &&
              ast_dump_builder_append(builder, "ReturnStmt") &&
              ast_dump_builder_finish_line(builder))) {
            return false;
        }

        if (statement->as.return_expression) {
            return ast_dump_expression_label(builder, indent + 1, "Value",
                                         statement->as.return_expression);
        }

        return true;

    case AST_STMT_EXIT:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "ExitStmt") &&
               ast_dump_builder_finish_line(builder);

    case AST_STMT_THROW:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "ThrowStmt") &&
               ast_dump_builder_finish_line(builder) &&
               ast_dump_expression_label(builder, indent + 1, "Value",
                                     statement->as.throw_expression);

    case AST_STMT_EXPRESSION:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "ExpressionStmt") &&
               ast_dump_builder_finish_line(builder) &&
               ast_dump_expression_label(builder, indent + 1, "Expression",
                                     statement->as.expression);

    case AST_STMT_MANUAL: {
        const char *label = statement->as.manual.is_checked
                            ? "ManualStmt checked" : "ManualStmt";
        if (!(ast_dump_builder_start_line(builder, indent) &&
              ast_dump_builder_append(builder, label) &&
              ast_dump_builder_finish_line(builder))) {
            return false;
        }
        return ast_dump_block_label(builder, indent + 1, "Body",
                                    statement->as.manual.body);
    }
    }

    return false;
}

bool ast_dump_block(AstDumpBuilder *builder, const AstBlock *block, int indent) {
    size_t i;

    if (!block) {
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "<null block>") &&
               ast_dump_builder_finish_line(builder);
    }

    if (!(ast_dump_builder_start_line(builder, indent) &&
          ast_dump_builder_append(builder, "Block") &&
          ast_dump_builder_finish_line(builder))) {
        return false;
    }

    for (i = 0; i < block->statement_count; i++) {
        if (!ast_dump_statement(builder, block->statements[i], indent + 1)) {
            return false;
        }
    }

    return true;
}
