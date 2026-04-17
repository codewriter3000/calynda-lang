#include "hir_internal.h"

#include <stdlib.h>
#include <string.h>

static bool hr_init_helper_callable(HirBuildContext *context,
                                    HirExpression *symbol_expr,
                                    const char *name,
                                    CheckedType return_type,
                                    const CheckedType *parameter_types,
                                    size_t parameter_count,
                                    AstSourceSpan source_span) {
    size_t i;

    (void)context;

    if (!symbol_expr || !name) {
        return false;
    }

    symbol_expr->kind = HIR_EXPR_SYMBOL;
    symbol_expr->type = return_type;
    symbol_expr->is_callable = true;
    symbol_expr->source_span = source_span;
    symbol_expr->as.symbol.name = ast_copy_text(name);
    symbol_expr->as.symbol.kind = SYMBOL_KIND_IMPORT;
    symbol_expr->as.symbol.type = return_type;
    symbol_expr->as.symbol.source_span = source_span;
    symbol_expr->callable_signature.return_type = return_type;
    symbol_expr->callable_signature.parameter_count = parameter_count;
    symbol_expr->callable_signature.has_parameter_types = true;

    if (!symbol_expr->as.symbol.name) {
        return false;
    }

    if (parameter_count == 0) {
        return true;
    }

    symbol_expr->callable_signature.parameter_types =
        calloc(parameter_count, sizeof(*symbol_expr->callable_signature.parameter_types));
    if (!symbol_expr->callable_signature.parameter_types) {
        return false;
    }

    for (i = 0; i < parameter_count; i++) {
        symbol_expr->callable_signature.parameter_types[i] = parameter_types[i];
    }

    return true;
}

static HirExpression *hr_make_helper_symbol(HirBuildContext *context,
                                            const char *name,
                                            CheckedType return_type,
                                            const CheckedType *parameter_types,
                                            size_t parameter_count,
                                            AstSourceSpan source_span) {
    HirExpression *symbol_expr = hr_expression_new(HIR_EXPR_SYMBOL);

    if (!symbol_expr) {
        return NULL;
    }

    if (!hr_init_helper_callable(context, symbol_expr, name, return_type,
                                 parameter_types, parameter_count, source_span)) {
        hir_expression_free(symbol_expr);
        return NULL;
    }

    return symbol_expr;
}

static HirExpression *hr_make_helper_call(HirBuildContext *context,
                                          const AstExpression *expression,
                                          const char *helper_name,
                                          CheckedType return_type,
                                          HirExpression **arguments,
                                          size_t argument_count) {
    HirExpression *call_expr;
    HirExpression *callee;
    size_t i;
    /* Support up to 4 parameters (current max for alpha.2 helpers). */
    CheckedType parameter_types[4];

    if (!expression || (!arguments && argument_count != 0)) {
        return NULL;
    }

    for (i = 0; i < argument_count && i < 4; i++) {
        parameter_types[i] = arguments[i]->type;
    }

    callee = hr_make_helper_symbol(context,
                                   helper_name,
                                   return_type,
                                   argument_count > 0 ? parameter_types : NULL,
                                   argument_count,
                                   expression->source_span);
    if (!callee) {
        hr_set_error(context,
                     expression->source_span,
                     NULL,
                     "Out of memory while lowering helper call.");
        return NULL;
    }

    call_expr = hr_expression_new(HIR_EXPR_CALL);
    if (!call_expr) {
        hir_expression_free(callee);
        hr_set_error(context,
                     expression->source_span,
                     NULL,
                     "Out of memory while lowering helper call.");
        return NULL;
    }

    call_expr->type = return_type;
    call_expr->source_span = expression->source_span;
    call_expr->as.call.callee = callee;
    for (i = 0; i < argument_count; i++) {
        if (!hr_append_argument(&call_expr->as.call, arguments[i])) {
            while (i > 0) {
                i--;
                call_expr->as.call.arguments[i] = NULL;
            }
            hir_expression_free(call_expr);
            hr_set_error(context,
                         expression->source_span,
                         NULL,
                         "Out of memory while lowering helper call.");
            return NULL;
        }
    }

    return call_expr;
}

static bool hr_is_builtin_array_call(HirBuildContext *context,
                                     const AstExpression *expression,
                                     const char **helper_name_out) {
    const AstExpression *callee;

    if (!context || !expression || expression->kind != AST_EXPR_CALL ||
        !expression->as.call.callee || expression->as.call.arguments.count != 1) {
        return false;
    }

    callee = expression->as.call.callee;
    if (callee->kind != AST_EXPR_IDENTIFIER ||
        symbol_table_resolve_identifier(context->symbols, callee) != NULL ||
        !callee->as.identifier) {
        return false;
    }

    if (strcmp(callee->as.identifier, "car") == 0) {
        if (helper_name_out) {
            *helper_name_out = "__calynda_rt_array_car";
        }
        return true;
    }

    if (strcmp(callee->as.identifier, "cdr") == 0) {
        if (helper_name_out) {
            *helper_name_out = "__calynda_rt_array_cdr";
        }
        return true;
    }

    return false;
}

static bool hr_is_builtin_mutex_new_call(const AstExpression *expression) {
    return expression &&
           expression->kind == AST_EXPR_CALL &&
           expression->as.call.callee &&
           expression->as.call.callee->kind == AST_EXPR_MEMBER &&
           expression->as.call.arguments.count == 0 &&
           expression->as.call.callee->as.member.target &&
           expression->as.call.callee->as.member.target->kind == AST_EXPR_IDENTIFIER &&
           expression->as.call.callee->as.member.target->as.identifier &&
           strcmp(expression->as.call.callee->as.member.target->as.identifier, "Mutex") == 0 &&
           strcmp(expression->as.call.callee->as.member.member, "new") == 0;
}

/*
 * Detect Atomic.new(value) — syntactic: callee is `Atomic.new`, one argument.
 * The type checker may not know about Atomic yet; we pattern-match the AST
 * node directly so HIR lowering can redirect to the runtime helper.
 */
static bool hr_is_builtin_atomic_new_call(const AstExpression *expression) {
    return expression &&
           expression->kind == AST_EXPR_CALL &&
           expression->as.call.callee &&
           expression->as.call.callee->kind == AST_EXPR_MEMBER &&
           expression->as.call.arguments.count == 1 &&
           expression->as.call.callee->as.member.target &&
           expression->as.call.callee->as.member.target->kind == AST_EXPR_IDENTIFIER &&
           expression->as.call.callee->as.member.target->as.identifier &&
           strcmp(expression->as.call.callee->as.member.target->as.identifier, "Atomic") == 0 &&
           strcmp(expression->as.call.callee->as.member.member, "new") == 0;
}

static bool hr_is_builtin_thread_or_mutex_call(HirBuildContext *context,
                                               const AstExpression *expression,
                                               const char **helper_name_out) {
    const AstExpression *member_target;
    const TypeCheckInfo *target_info;
    CheckedType target_type;

    if (!expression || expression->kind != AST_EXPR_CALL ||
        !expression->as.call.callee ||
        expression->as.call.callee->kind != AST_EXPR_MEMBER) {
        return false;
    }

    if (hr_is_builtin_mutex_new_call(expression)) {
        if (helper_name_out) {
            *helper_name_out = "__calynda_rt_mutex_new";
        }
        return true;
    }

    /* Atomic.new(value) handled separately in hr_lower_expression */

    member_target = expression->as.call.callee->as.member.target;
    target_info = type_checker_get_expression_info(context->checker, member_target);
    if (!target_info) {
        return false;
    }
    target_type = target_info->type;

    /* Thread methods */
    if (target_type.kind == CHECKED_TYPE_NAMED && target_type.name &&
        strcmp(target_type.name, "Thread") == 0) {
        if (expression->as.call.arguments.count == 0) {
            if (strcmp(expression->as.call.callee->as.member.member, "join") == 0) {
                if (helper_name_out) {
                    *helper_name_out = "__calynda_rt_thread_join";
                }
                return true;
            }
            if (strcmp(expression->as.call.callee->as.member.member, "cancel") == 0) {
                if (helper_name_out) {
                    *helper_name_out = "__calynda_rt_thread_cancel";
                }
                return true;
            }
        }
    }

    /* Mutex methods */
    if (target_type.kind == CHECKED_TYPE_NAMED && target_type.name &&
        strcmp(target_type.name, "Mutex") == 0) {
        if (expression->as.call.arguments.count == 0) {
            if (strcmp(expression->as.call.callee->as.member.member, "lock") == 0) {
                if (helper_name_out) {
                    *helper_name_out = "__calynda_rt_mutex_lock";
                }
                return true;
            }
            if (strcmp(expression->as.call.callee->as.member.member, "unlock") == 0) {
                if (helper_name_out) {
                    *helper_name_out = "__calynda_rt_mutex_unlock";
                }
                return true;
            }
        }
    }

    /* Future methods (alpha.2) */
    if (target_type.kind == CHECKED_TYPE_NAMED && target_type.name &&
        strcmp(target_type.name, "Future") == 0) {
        if (expression->as.call.arguments.count == 0) {
            if (strcmp(expression->as.call.callee->as.member.member, "get") == 0) {
                if (helper_name_out) {
                    *helper_name_out = "__calynda_rt_future_get";
                }
                return true;
            }
            if (strcmp(expression->as.call.callee->as.member.member, "cancel") == 0) {
                if (helper_name_out) {
                    *helper_name_out = "__calynda_rt_future_cancel";
                }
                return true;
            }
        }
    }

    /* Atomic methods (alpha.2): load/store/exchange dispatched on Atomic type */
    if (target_type.kind == CHECKED_TYPE_NAMED && target_type.name &&
        strcmp(target_type.name, "Atomic") == 0) {
        if (expression->as.call.arguments.count == 0 &&
            strcmp(expression->as.call.callee->as.member.member, "load") == 0) {
            if (helper_name_out) {
                *helper_name_out = "__calynda_rt_atomic_load";
            }
            return true;
        }
        if (expression->as.call.arguments.count == 1 &&
            strcmp(expression->as.call.callee->as.member.member, "store") == 0) {
            if (helper_name_out) {
                *helper_name_out = "__calynda_rt_atomic_store";
            }
            return true;
        }
        if (expression->as.call.arguments.count == 1 &&
            strcmp(expression->as.call.callee->as.member.member, "exchange") == 0) {
            if (helper_name_out) {
                *helper_name_out = "__calynda_rt_atomic_exchange";
            }
            return true;
        }
    }

    return false;
}

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
        strcmp(callee->as.member.member, "print") != 0) {
        return false;
    }

    if (expression->as.call.arguments.count > 1) {
        hr_set_error(context,
                     expression->source_span,
                     &target_symbol->declaration_span,
                     "Freestanding boot() code supports io.stdlib.print with 0 or 1 argument, but got %zu.",
                     expression->as.call.arguments.count);
        return true;
    }

    helper_name = expression->as.call.arguments.count == 0
        ? "__calynda_rt_stdlib_print0"
        : "__calynda_rt_stdlib_print1";
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

HirExpression *hr_lower_expression(HirBuildContext *context,
                                   const AstExpression *expression) {
    const TypeCheckInfo *info;
    HirExpression *hir_expression;

    if (!expression) {
        return NULL;
    }

    if (expression->kind == AST_EXPR_GROUPING) {
        return hr_lower_expression(context, expression->as.grouping.inner);
    }

    info = type_checker_get_expression_info(context->checker, expression);
    if (!info) {
        hr_set_error(context,
                     expression->source_span,
                     NULL,
                     "Internal error: missing type info for expression during HIR lowering.");
        return NULL;
    }

    if (expression->kind == AST_EXPR_MEMORY_OP) {
        return hr_lower_memory_expression(context, expression, info);
    }

    if (expression->kind == AST_EXPR_CALL) {
        const char *helper_name = NULL;

        if (hr_is_builtin_array_call(context, expression, &helper_name)) {
            HirExpression *argument =
                hr_lower_expression(context, expression->as.call.arguments.items[0]);
            HirExpression *arguments[1];

            if (!argument) {
                return NULL;
            }
            arguments[0] = argument;
            return hr_make_helper_call(context,
                                       expression,
                                       helper_name,
                                       info->type,
                                       arguments,
                                       1);
        }
    }

    if (expression->kind == AST_EXPR_SPAWN) {
        HirExpression *argument = hr_lower_expression(context, expression->as.spawn.callable);
        HirExpression *arguments[1];
        const char *helper_name = "__calynda_rt_thread_spawn";

        if (!argument) {
            return NULL;
        }
        if (info->type.kind == CHECKED_TYPE_NAMED &&
            info->type.name &&
            strcmp(info->type.name, "Future") == 0) {
            helper_name = "__calynda_rt_future_spawn";
        }
        arguments[0] = argument;
        return hr_make_helper_call(context,
                                   expression,
                                   helper_name,
                                   info->type,
                                   arguments,
                                   1);
    }

    if (hr_is_builtin_mutex_new_call(expression)) {
        return hr_make_helper_call(context,
                                   expression,
                                   "__calynda_rt_mutex_new",
                                   info->type,
                                   NULL,
                                   0);
    }

    if (hr_is_builtin_atomic_new_call(expression)) {
        HirExpression *argument =
            hr_lower_expression(context, expression->as.call.arguments.items[0]);
        HirExpression *arguments[1];

        if (!argument) {
            return NULL;
        }
        arguments[0] = argument;
        return hr_make_helper_call(context,
                                   expression,
                                   "__calynda_rt_atomic_new",
                                   info->type,
                                    arguments,
                                    1);
    }

    if (expression->kind == AST_EXPR_CALL) {
        HirExpression *freestanding_import_call = NULL;

        if (hr_try_lower_freestanding_import_member_call(context,
                                                         expression,
                                                         info,
                                                         &freestanding_import_call)) {
            return freestanding_import_call;
        }
    }

    if (expression->kind == AST_EXPR_CALL && expression->as.call.callee &&
        expression->as.call.callee->kind == AST_EXPR_MEMBER) {
        const char *helper_name = NULL;
        if (hr_is_builtin_thread_or_mutex_call(context, expression, &helper_name)) {
            /* Determine how many arguments the helper takes:
             *   - target value is always argument[0]
             *   - single extra call argument (if any) is argument[1] */
            size_t extra_arg_count = expression->as.call.arguments.count;
            HirExpression *arg_target;
            HirExpression *arguments[2];

            arg_target = hr_lower_expression(context,
                                             expression->as.call.callee->as.member.target);
            if (!arg_target) {
                return NULL;
            }
            arguments[0] = arg_target;

            if (extra_arg_count == 1) {
                HirExpression *extra_arg =
                    hr_lower_expression(context,
                                        expression->as.call.arguments.items[0]);
                if (!extra_arg) {
                    hir_expression_free(arg_target);
                    return NULL;
                }
                arguments[1] = extra_arg;
                return hr_make_helper_call(context,
                                           expression,
                                           helper_name,
                                           info->type,
                                           arguments,
                                           2);
            }

            return hr_make_helper_call(context,
                                       expression,
                                       helper_name,
                                       info->type,
                                       arguments,
                                       1);
        }
    }

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        hir_expression = hr_expression_new(expression->as.literal.kind == AST_LITERAL_TEMPLATE
                                               ? HIR_EXPR_TEMPLATE
                                               : HIR_EXPR_LITERAL);
        break;
    case AST_EXPR_IDENTIFIER:
        hir_expression = hr_expression_new(HIR_EXPR_SYMBOL);
        break;
    case AST_EXPR_LAMBDA:
        hir_expression = hr_expression_new(HIR_EXPR_LAMBDA);
        break;
    case AST_EXPR_ASSIGNMENT:
        hir_expression = hr_expression_new(HIR_EXPR_ASSIGNMENT);
        break;
    case AST_EXPR_TERNARY:
        hir_expression = hr_expression_new(HIR_EXPR_TERNARY);
        break;
    case AST_EXPR_BINARY:
        hir_expression = hr_expression_new(HIR_EXPR_BINARY);
        break;
    case AST_EXPR_UNARY:
        hir_expression = hr_expression_new(HIR_EXPR_UNARY);
        break;
    case AST_EXPR_CALL:
        hir_expression = hr_expression_new(HIR_EXPR_CALL);
        break;
    case AST_EXPR_INDEX:
        hir_expression = hr_expression_new(HIR_EXPR_INDEX);
        break;
    case AST_EXPR_MEMBER:
        hir_expression = hr_expression_new(HIR_EXPR_MEMBER);
        break;
    case AST_EXPR_CAST:
        hir_expression = hr_expression_new(HIR_EXPR_CAST);
        break;
    case AST_EXPR_ARRAY_LITERAL:
        hir_expression = hr_expression_new(HIR_EXPR_ARRAY_LITERAL);
        break;
    case AST_EXPR_DISCARD:
        hir_expression = hr_expression_new(HIR_EXPR_DISCARD);
        break;
    case AST_EXPR_POST_INCREMENT:
        hir_expression = hr_expression_new(HIR_EXPR_POST_INCREMENT);
        break;
    case AST_EXPR_POST_DECREMENT:
        hir_expression = hr_expression_new(HIR_EXPR_POST_DECREMENT);
        break;
    case AST_EXPR_GROUPING:
    default:
        hir_expression = NULL;
        break;
    }

    if (!hir_expression) {
        hr_set_error(context,
                     expression->source_span,
                     NULL,
                     "Out of memory while lowering HIR expressions.");
        return NULL;
    }

    hir_expression->type = info->type;
    hir_expression->is_callable = info->is_callable;
    hir_expression->source_span = expression->source_span;
    if (info->is_callable &&
        !hr_lower_callable_signature(context, info, &hir_expression->callable_signature)) {
        hir_expression_free(hir_expression);
        return NULL;
    }

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
    case AST_EXPR_IDENTIFIER:
    case AST_EXPR_LAMBDA:
    case AST_EXPR_CALL:
    case AST_EXPR_MEMBER:
    case AST_EXPR_ARRAY_LITERAL:
        return hr_lower_expr_complex(context, expression, hir_expression, info);

    case AST_EXPR_ASSIGNMENT:
        hir_expression->as.assignment.operator = expression->as.assignment.operator;
        hir_expression->as.assignment.target = hr_lower_expression(context,
                                                                   expression->as.assignment.target);
        hir_expression->as.assignment.value = hr_lower_expression(context,
                                                                  expression->as.assignment.value);
        break;

    case AST_EXPR_TERNARY:
        hir_expression->as.ternary.condition = hr_lower_expression(context,
                                                                   expression->as.ternary.condition);
        hir_expression->as.ternary.then_branch = hr_lower_expression(context,
                                                                     expression->as.ternary.then_branch);
        hir_expression->as.ternary.else_branch = hr_lower_expression(context,
                                                                     expression->as.ternary.else_branch);
        break;

    case AST_EXPR_BINARY:
        hir_expression->as.binary.operator = expression->as.binary.operator;
        hir_expression->as.binary.left = hr_lower_expression(context, expression->as.binary.left);
        hir_expression->as.binary.right = hr_lower_expression(context, expression->as.binary.right);
        break;

    case AST_EXPR_UNARY:
        hir_expression->as.unary.operator = expression->as.unary.operator;
        hir_expression->as.unary.operand = hr_lower_expression(context,
                                                               expression->as.unary.operand);
        break;

    case AST_EXPR_INDEX:
        hir_expression->as.index.target = hr_lower_expression(context, expression->as.index.target);
        hir_expression->as.index.index = hr_lower_expression(context, expression->as.index.index);
        break;

    case AST_EXPR_CAST:
        hir_expression->as.cast.target_type = info->type;
        hir_expression->as.cast.expression = hr_lower_expression(context,
                                                                 expression->as.cast.expression);
        break;

    case AST_EXPR_DISCARD:
        break;

    case AST_EXPR_POST_INCREMENT:
        hir_expression->as.post_increment.operand =
            hr_lower_expression(context, expression->as.post_increment.operand);
        break;

    case AST_EXPR_POST_DECREMENT:
        hir_expression->as.post_decrement.operand =
            hr_lower_expression(context, expression->as.post_decrement.operand);
        break;

    case AST_EXPR_GROUPING:
    default:
        hir_expression_free(hir_expression);
        return NULL;
    }

    if ((expression->kind == AST_EXPR_ASSIGNMENT &&
         (!hir_expression->as.assignment.target || !hir_expression->as.assignment.value)) ||
        (expression->kind == AST_EXPR_TERNARY &&
         (!hir_expression->as.ternary.condition ||
          !hir_expression->as.ternary.then_branch ||
          !hir_expression->as.ternary.else_branch)) ||
        (expression->kind == AST_EXPR_BINARY &&
         (!hir_expression->as.binary.left || !hir_expression->as.binary.right)) ||
        (expression->kind == AST_EXPR_UNARY && !hir_expression->as.unary.operand) ||
        (expression->kind == AST_EXPR_INDEX &&
         (!hir_expression->as.index.target || !hir_expression->as.index.index)) ||
        (expression->kind == AST_EXPR_CAST && !hir_expression->as.cast.expression) ||
        (expression->kind == AST_EXPR_POST_INCREMENT &&
         !hir_expression->as.post_increment.operand) ||
        (expression->kind == AST_EXPR_POST_DECREMENT &&
         !hir_expression->as.post_decrement.operand)) {
        hir_expression_free(hir_expression);
        return NULL;
    }

    return hir_expression;
}
