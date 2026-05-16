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
        return tr_append_array_extents_to_string(type, buffer, buffer_size, &written);
    }

    return false;
}
