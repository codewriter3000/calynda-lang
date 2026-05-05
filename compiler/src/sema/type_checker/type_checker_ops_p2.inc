            if (!target_info) {
                return false;
            }
            target_type = tc_type_check_source_type(target_info);
            symbol = root_scope && target_type.name
                ? scope_lookup_local(root_scope, target_type.name)
                : NULL;
            if (!symbol && target_type.name) {
                symbol = symbol_table_find_import(checker->symbols, target_type.name);
            }
            if ((strcmp(expression->as.member.member, "tag") == 0 ||
                 strcmp(expression->as.member.member, "payload") == 0) &&
                symbol && symbol->kind == SYMBOL_KIND_UNION) {
                return false;
            }
            if (strcmp(expression->as.member.member, "length") == 0 &&
                tc_checked_type_has_length_member(target_type)) {
                return false;
            }
            if (target_type.kind == CHECKED_TYPE_EXTERNAL) {
                return true;
            }
            return tc_expression_is_assignment_target(checker, expression->as.member.target,
                                                      root_symbol);
        }

    case AST_EXPR_GROUPING:
        return tc_expression_is_assignment_target(checker,
                                                  expression->as.grouping.inner,
                                                  root_symbol);

    case AST_EXPR_DISCARD:
        return true;

    default:
        return false;
    }
}
