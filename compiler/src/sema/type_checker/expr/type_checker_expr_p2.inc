            info = *value_info;
            info.type = target_info->type;
            if (value_info->is_callable) {
                info.callable_return_type = target_info->type;
            }
        }
        break;

    default:
        return tc_check_expression_ext(checker, expression);
    }

    return tc_store_expression_info(checker, expression, info);
}
