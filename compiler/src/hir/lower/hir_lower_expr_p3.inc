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

#include "hir_lower_expr_p4.inc"
