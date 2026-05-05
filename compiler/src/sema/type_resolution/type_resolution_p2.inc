    case RESOLVED_TYPE_NAMED:
        written = snprintf(buffer, buffer_size, "%s", type.name ? type.name : "?");
        if (written < 0 || (size_t)written >= buffer_size) {
            return false;
        }
        if (type.generic_arg_count > 0) {
            written += snprintf(buffer + written, buffer_size - (size_t)written,
                                "<...%zu>", type.generic_arg_count);
            if (written < 0 || (size_t)written >= buffer_size) {
                return false;
            }
        }
        for (i = 0; i < type.array_depth; i++) {
            written += snprintf(buffer + written, buffer_size - (size_t)written, "[]");
            if (written < 0 || (size_t)written >= buffer_size) {
                return false;
            }
        }
        return true;
    }

    return false;
}
