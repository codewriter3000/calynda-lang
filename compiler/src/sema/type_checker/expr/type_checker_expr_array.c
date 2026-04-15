#include "type_checker_internal.h"

bool tc_check_array_literal(TypeChecker *checker,
                            const AstExpression *expression,
                            TypeCheckInfo *info) {
    CheckedType element_type = tc_checked_type_invalid();
    size_t i;

    if (expression->as.array_literal.elements.count == 0) {
        tc_set_error_at(checker, expression->source_span, NULL,
                        "Cannot infer the element type of an empty array literal.");
        return false;
    }
    for (i = 0; i < expression->as.array_literal.elements.count; i++) {
        const TypeCheckInfo *element_info = tc_check_expression(checker,
                                                                expression->as.array_literal.elements.items[i]);
        CheckedType candidate_type;

        if (!element_info) {
            return false;
        }

        candidate_type = tc_type_check_source_type(element_info);
        if (candidate_type.kind == CHECKED_TYPE_VOID ||
            candidate_type.kind == CHECKED_TYPE_NULL) {
            tc_set_error_at(checker,
                            expression->as.array_literal.elements.items[i]->source_span,
                            NULL,
                            "Array literal elements cannot have type %s.",
                            candidate_type.kind == CHECKED_TYPE_VOID
                                ? "void"
                                : "null");
            return false;
        }

        if (i == 0) {
            element_type = candidate_type;
        } else if (!tc_merge_types_for_inference(element_type,
                                                 candidate_type,
                                                 &element_type)) {
            char left_text[64], right_text[64];
            checked_type_to_string(element_type, left_text, sizeof(left_text));
            checked_type_to_string(candidate_type, right_text, sizeof(right_text));
            tc_set_error_at(checker,
                            expression->as.array_literal.elements.items[i]->source_span,
                            &expression->as.array_literal.elements.items[0]->source_span,
                            "Array literal elements must have compatible types, but got %s and %s.",
                            left_text,
                            right_text);
            return false;
        }
    }

    if (element_type.kind != CHECKED_TYPE_VALUE) {
        tc_set_error_at(checker, expression->source_span, NULL,
                        "Array literal element type cannot be inferred from external values.");
        return false;
    }

    *info = tc_type_check_info_make(
        tc_checked_type_value(element_type.primitive, element_type.array_depth + 1));
    return true;
}
