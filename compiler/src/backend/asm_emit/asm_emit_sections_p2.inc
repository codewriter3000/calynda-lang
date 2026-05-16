static bool ae_emit_static_array_global_symbol(FILE *out,
                                               const MachineStaticArrayBinding *binding) {
    char *sanitized;
    char *symbol;
    bool ok;

    if (!out || !binding || !binding->global_name) {
        return false;
    }

    sanitized = ae_sanitize_symbol(binding->global_name);
    if (!sanitized) {
        return false;
    }

    symbol = ae_copy_format("calynda_global_%s", sanitized);
    free(sanitized);
    if (!symbol) {
        return false;
    }

    ok = ae_emit_line(out,
                      ".globl %s\n"
                      "%s:\n"
                      "    .quad .Larr_obj_%zu\n",
                      symbol,
                      symbol,
                      binding->object_index);
    free(symbol);
    return ok;
}

bool ae_emit_data(FILE *out, const AsmEmitContext *context) {
    size_t i;

    if (!context) {
        return false;
    }
    if (context->global_symbol_count == 0 && context->string_literal_count == 0 &&
        context->type_descriptor_count == 0 &&
        (!context->program || context->program->static_array_binding_count == 0)) {
        return true;
    }
    if (!ae_emit_line(out, ".data\n")) {
        return false;
    }
    for (i = 0; i < context->string_literal_count; i++) {
        if (!ae_emit_line(out,
                       "    .balign 8\n"
                       "%s:\n"
                       "    .long %u\n"
                       "    .long %u\n"
                       "    .quad 0\n"
                       "    .quad %zu\n"
                       "    .quad %s\n",
                       context->string_literals[i].object_label,
                       CALYNDA_RT_OBJECT_MAGIC,
                       CALYNDA_RT_OBJECT_STRING,
                       context->string_literals[i].length,
                       context->string_literals[i].bytes_label)) {
            return false;
        }
        if (!ae_emit_line(out, "%s:\n", context->string_literals[i].bytes_label)) {
            return false;
        }
        if (context->string_literals[i].length == 0) {
            if (!ae_emit_line(out, "    .byte 0\n")) {
                return false;
            }
        } else {
            size_t j;

            if (!ae_emit_line(out, "    .byte ")) {
                return false;
            }
            for (j = 0; j < context->string_literals[i].length; j++) {
                if (j > 0 && !ae_emit_line(out, ", ")) {
                    return false;
                }
                if (!ae_emit_line(out,
                               "%u",
                               (unsigned int)(unsigned char)context->string_literals[i].text[j])) {
                    return false;
                }
            }
            if (!ae_emit_line(out, ", 0\n")) {
                return false;
            }
        }
    }
    if (!ae_emit_type_descriptors(out, context)) {
        return false;
    }
    if (context->program) {
        for (i = 0; i < context->program->static_array_binding_count; i++) {
            if (!ae_emit_static_array_global_symbol(out,
                                                    &context->program->static_array_bindings[i])) {
                return false;
            }
        }
    }
    for (i = 0; i < context->global_symbol_count; i++) {
        if (ae_find_static_array_binding(context, context->global_symbols[i].name)) {
            continue;
        }

        char *sanitized = ae_sanitize_symbol(context->global_symbols[i].name);

        if (!sanitized) {
            return false;
        }
        if (!ae_emit_line(out, ".globl %s\n%s:\n", context->global_symbols[i].symbol, context->global_symbols[i].symbol)) {
            free(sanitized);
            return false;
        }
        if (context->global_symbols[i].has_store) {
            if (!ae_emit_line(out, "    .quad 0\n")) {
                free(sanitized);
                return false;
            }
        } else {
            bool is_external_callable =
                ae_starts_with(context->global_symbols[i].name, "__calynda_") ||
                strcmp(context->global_symbols[i].name, "malloc") == 0 ||
                strcmp(context->global_symbols[i].name, "calloc") == 0 ||
                strcmp(context->global_symbols[i].name, "realloc") == 0 ||
                strcmp(context->global_symbols[i].name, "free") == 0;

            if (!ae_emit_line(out,
                              is_external_callable ? "    .quad %s\n"
                                                   : "    .quad __calynda_pkg_%s\n",
                              is_external_callable ? context->global_symbols[i].name : sanitized)) {
                free(sanitized);
                return false;
            }
        }
        free(sanitized);
    }
    return true;
}
