#include "hir_dump_internal.h"

bool hir_dump_program(FILE *out, const HirProgram *program) {
    size_t i;

    if (!out || !program) {
        return false;
    }

    fprintf(out, "HirProgram\n");
    if (program->has_package) {
        fprintf(out, "  Package: name=%s qualified=%s span=", program->package.name,
                program->package.qualified_name);
        hir_dump_write_span(out, program->package.source_span);
        fputc('\n', out);
    } else {
        fprintf(out, "  Package: <none>\n");
    }

    if (program->import_count == 0) {
        fprintf(out, "  Imports: []\n");
    } else {
        fprintf(out, "  Imports:\n");
        for (i = 0; i < program->import_count; i++) {
            fprintf(out, "    Import name=%s qualified=%s span=",
                    program->imports[i].name,
                    program->imports[i].qualified_name);
            hir_dump_write_span(out, program->imports[i].source_span);
            fputc('\n', out);
        }
    }

    fprintf(out, "  TopLevel:\n");
    for (i = 0; i < program->top_level_count; i++) {
        const HirTopLevelDecl *decl = program->top_level_decls[i];

        if (decl->kind == HIR_TOP_LEVEL_BINDING) {
            fprintf(out, "    Binding name=%s type=", decl->as.binding.name);
            if (!hir_dump_write_checked_type(out, decl->as.binding.type)) {
                return false;
            }
            fprintf(out, " final=%s", decl->as.binding.is_final ? "true" : "false");
            if (decl->as.binding.is_exported) {
                fprintf(out, " exported");
            }
            if (decl->as.binding.is_static) {
                fprintf(out, " static");
            }
            if (decl->as.binding.is_internal) {
                fprintf(out, " internal");
            }
            if (decl->as.binding.is_callable) {
                fprintf(out, " callable=");
                if (!hir_dump_write_callable_signature(out, &decl->as.binding.callable_signature)) {
                    return false;
                }
            }
            fprintf(out, " span=");
            hir_dump_write_span(out, decl->as.binding.source_span);
            fputc('\n', out);
            fprintf(out, "      Init:\n");
            if (!hir_dump_expression(out, decl->as.binding.initializer, 8)) {
                return false;
            }
        } else if (decl->kind == HIR_TOP_LEVEL_UNION) {
            size_t j;

            fprintf(out, "    Union name=%s", decl->as.union_decl.name);
            if (decl->as.union_decl.is_exported) {
                fprintf(out, " exported");
            }
            if (decl->as.union_decl.is_static) {
                fprintf(out, " static");
            }
            if (decl->as.union_decl.generic_param_count > 0) {
                fprintf(out, " generics=<");
                for (j = 0; j < decl->as.union_decl.generic_param_count; j++) {
                    if (j > 0) fprintf(out, ", ");
                    fprintf(out, "%s", decl->as.union_decl.generic_params[j]);
                }
                fprintf(out, ">");
            }
            fprintf(out, " span=");
            hir_dump_write_span(out, decl->as.union_decl.source_span);
            fputc('\n', out);
            for (j = 0; j < decl->as.union_decl.variant_count; j++) {
                fprintf(out, "      Variant name=%s", decl->as.union_decl.variants[j].name);
                if (decl->as.union_decl.variants[j].has_payload) {
                    fprintf(out, " has_payload=true");
                }
                fputc('\n', out);
            }
        } else if (decl->kind == HIR_TOP_LEVEL_ASM) {
            size_t j;

            fprintf(out, "    Asm name=%s return=", decl->as.asm_decl.name);
            if (!hir_dump_write_checked_type(out, decl->as.asm_decl.return_type)) {
                return false;
            }
            if (decl->as.asm_decl.is_exported) {
                fprintf(out, " exported");
            }
            if (decl->as.asm_decl.is_static) {
                fprintf(out, " static");
            }
            if (decl->as.asm_decl.is_internal) {
                fprintf(out, " internal");
            }
            fprintf(out, " params=%zu body_length=%zu",
                    decl->as.asm_decl.parameter_count,
                    decl->as.asm_decl.body_length);
            fprintf(out, " span=");
            hir_dump_write_span(out, decl->as.asm_decl.source_span);
            fputc('\n', out);
            for (j = 0; j < decl->as.asm_decl.parameter_count; j++) {
                fprintf(out, "      Param name=%s\n",
                        decl->as.asm_decl.parameter_names[j]);
            }
        } else {
            size_t j;

            fprintf(out, "    Start span=");
            hir_dump_write_span(out, decl->as.start.source_span);
            fputc('\n', out);
            fprintf(out, "      Parameters:\n");
            for (j = 0; j < decl->as.start.parameters.count; j++) {
                fprintf(out, "        Param name=%s type=", decl->as.start.parameters.items[j].name);
                if (!hir_dump_write_checked_type(out, decl->as.start.parameters.items[j].type)) {
                    return false;
                }
                if (decl->as.start.parameters.items[j].is_varargs) {
                    fprintf(out, " varargs");
                }
                fprintf(out, " span=");
                hir_dump_write_span(out, decl->as.start.parameters.items[j].source_span);
                fputc('\n', out);
            }
            fprintf(out, "      Body:\n");
            if (!hir_dump_block(out, decl->as.start.body, 8)) {
                return false;
            }
        }
    }

    return !ferror(out);
}

char *hir_dump_program_to_string(const HirProgram *program) {
    FILE *stream;
    char *buffer;
    long size;
    size_t read_size;

    if (!program) {
        return NULL;
    }

    stream = tmpfile();
    if (!stream) {
        return NULL;
    }

    if (!hir_dump_program(stream, program) || fflush(stream) != 0 ||
        fseek(stream, 0, SEEK_END) != 0) {
        fclose(stream);
        return NULL;
    }

    size = ftell(stream);
    if (size < 0 || fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream);
        return NULL;
    }

    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(stream);
        return NULL;
    }

    read_size = fread(buffer, 1, (size_t)size, stream);
    fclose(stream);
    if (read_size != (size_t)size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}
