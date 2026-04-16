#include "hir_dump_internal.h"

bool hir_dump_expression_ext(FILE *out, const HirExpression *expression, int indent) {
    size_t i;

    if (!expression) {
        return false;
    }

    hir_dump_write_indent(out, indent);
    switch (expression->kind) {
    case HIR_EXPR_INDEX:
        fprintf(out, "Index type=");
        if (!hir_dump_write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        hir_dump_write_span(out, expression->source_span);
        fputc('\n', out);
        hir_dump_write_indent(out, indent + 2);
        fprintf(out, "Target:\n");
        if (!hir_dump_expression(out, expression->as.index.target, indent + 4)) {
            return false;
        }
        hir_dump_write_indent(out, indent + 2);
        fprintf(out, "Index:\n");
        return hir_dump_expression(out, expression->as.index.index, indent + 4);
    case HIR_EXPR_MEMBER:
        fprintf(out, "Member name=%s type=", expression->as.member.member);
        if (!hir_dump_write_checked_type(out, expression->type)) {
            return false;
        }
        if (expression->is_callable) {
            fprintf(out, " callable=");
            if (!hir_dump_write_callable_signature(out, &expression->callable_signature)) {
                return false;
            }
        }
        fprintf(out, " span=");
        hir_dump_write_span(out, expression->source_span);
        fputc('\n', out);
        return hir_dump_expression(out, expression->as.member.target, indent + 2);
    case HIR_EXPR_CAST:
        fprintf(out, "Cast target=");
        if (!hir_dump_write_checked_type(out, expression->as.cast.target_type)) {
            return false;
        }
        fprintf(out, " span=");
        hir_dump_write_span(out, expression->source_span);
        fputc('\n', out);
        return hir_dump_expression(out, expression->as.cast.expression, indent + 2);
    case HIR_EXPR_ARRAY_LITERAL:
        fprintf(out, "ArrayLiteral type=");
        if (!hir_dump_write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        hir_dump_write_span(out, expression->source_span);
        fputc('\n', out);
        for (i = 0; i < expression->as.array_literal.element_count; i++) {
            hir_dump_write_indent(out, indent + 2);
            fprintf(out, "Element %zu:\n", i + 1);
            if (!hir_dump_expression(out,
                    expression->as.array_literal.elements[i], indent + 4)) {
                return false;
            }
        }
        return true;
    case HIR_EXPR_DISCARD:
        fprintf(out, "Discard type=");
        if (!hir_dump_write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        hir_dump_write_span(out, expression->source_span);
        fputc('\n', out);
        return true;
    case HIR_EXPR_POST_INCREMENT:
        fprintf(out, "PostIncrement type=");
        if (!hir_dump_write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        hir_dump_write_span(out, expression->source_span);
        fputc('\n', out);
        return hir_dump_expression(out, expression->as.post_increment.operand, indent + 2);
    case HIR_EXPR_POST_DECREMENT:
        fprintf(out, "PostDecrement type=");
        if (!hir_dump_write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        hir_dump_write_span(out, expression->source_span);
        fputc('\n', out);
        return hir_dump_expression(out, expression->as.post_decrement.operand, indent + 2);
    case HIR_EXPR_MEMORY_OP:
        {
            const char *op_name;
            size_t mem_i;
            switch (expression->as.memory_op.kind) {
            case HIR_MEMORY_MALLOC:  op_name = "malloc";  break;
            case HIR_MEMORY_CALLOC:  op_name = "calloc";  break;
            case HIR_MEMORY_REALLOC: op_name = "realloc"; break;
            case HIR_MEMORY_FREE:    op_name = "free";    break;
            case HIR_MEMORY_DEREF:   op_name = "deref";   break;
            case HIR_MEMORY_ADDR:    op_name = "addr";    break;
            case HIR_MEMORY_OFFSET:  op_name = "offset";  break;
            case HIR_MEMORY_STORE:   op_name = "store";   break;
            case HIR_MEMORY_CLEANUP: op_name = "cleanup"; break;
            case HIR_MEMORY_STACKALLOC: op_name = "stackalloc"; break;
            default:                 op_name = "unknown"; break;
            }
            fprintf(out, "MemoryOp op=%s type=", op_name);
            if (!hir_dump_write_checked_type(out, expression->type)) {
                return false;
            }
            fprintf(out, " span=");
            hir_dump_write_span(out, expression->source_span);
            fputc('\n', out);
            for (mem_i = 0; mem_i < expression->as.memory_op.argument_count; mem_i++) {
                if (!hir_dump_expression(out, expression->as.memory_op.arguments[mem_i],
                                         indent + 2)) {
                    return false;
                }
            }
            return true;
        }
    default:
        return false;
    }

    return false;
}
