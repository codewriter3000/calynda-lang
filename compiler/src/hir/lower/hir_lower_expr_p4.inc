static bool hr_program_has_boot_entry(const HirBuildContext *context) {
    size_t i;

    if (!context || !context->ast_program) {
        return false;
    }

    for (i = 0; i < context->ast_program->top_level_count; i++) {
        const AstTopLevelDecl *decl = context->ast_program->top_level_decls[i];

        if (decl &&
            decl->kind == AST_TOP_LEVEL_START &&
            decl->as.start_decl.is_boot) {
            return true;
        }
    }

    return false;
}

static bool hr_try_lower_freestanding_import_member_call(HirBuildContext *context,
                                                         const AstExpression *expression,
                                                         const TypeCheckInfo *info,
                                                         HirExpression **lowered_out) {
    const AstExpression *callee;
    const AstExpression *target;
    const Symbol *target_symbol;
    const char *helper_name;
    HirExpression *arguments[1];

    if (lowered_out) {
        *lowered_out = NULL;
    }
    if (!context || !expression || !info || !lowered_out ||
        expression->kind != AST_EXPR_CALL ||
        !hr_program_has_boot_entry(context)) {
        return false;
    }

    callee = expression->as.call.callee;
    if (!callee || callee->kind != AST_EXPR_MEMBER) {
        return false;
    }

    target = callee->as.member.target;
    if (!target || target->kind != AST_EXPR_IDENTIFIER) {
        return false;
    }

    target_symbol = symbol_table_resolve_identifier(context->symbols, target);
    if (!target_symbol ||
        (target_symbol->kind != SYMBOL_KIND_IMPORT &&
         target_symbol->kind != SYMBOL_KIND_PACKAGE) ||
        !target_symbol->qualified_name ||
        strcmp(target_symbol->qualified_name, "io.stdlib") != 0 ||
        (strcmp(callee->as.member.member, "print") != 0 &&
         strcmp(callee->as.member.member, "input") != 0)) {
        return false;
    }

    if (expression->as.call.arguments.count > 1) {
        const char *fn_name = callee->as.member.member;
        hr_set_error(context,
                     expression->source_span,
                     &target_symbol->declaration_span,
                     "Freestanding boot() code supports io.stdlib.%s with 0 or 1 argument, but got %zu.",
                     fn_name ? fn_name : "?",
                     expression->as.call.arguments.count);
        return true;
    }

    if (strcmp(callee->as.member.member, "input") == 0) {
        helper_name = expression->as.call.arguments.count == 0
            ? "__calynda_rt_stdlib_input0"
            : "__calynda_rt_stdlib_input1";
    } else {
        helper_name = expression->as.call.arguments.count == 0
            ? "__calynda_rt_stdlib_print0"
            : "__calynda_rt_stdlib_print1";
    }
    if (expression->as.call.arguments.count == 0) {
        *lowered_out = hr_make_helper_call(context,
                                           expression,
                                           helper_name,
                                           info->type,
                                           NULL,
                                           0);
        return true;
    }

    arguments[0] = hr_lower_expression(context, expression->as.call.arguments.items[0]);
    if (!arguments[0]) {
        return true;
    }

    *lowered_out = hr_make_helper_call(context,
                                       expression,
                                       helper_name,
                                       info->type,
                                       arguments,
                                       1);
    return true;
}

#include "hir_lower_expr_p5.inc"
