#include "runtime_abi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const CodegenRegister runtime_abi_helper_arg_regs[];
extern const size_t runtime_abi_helper_arg_reg_count;
extern const RuntimeAbiHelperSignature runtime_abi_helpers[];
extern const size_t runtime_abi_helper_count;

static bool write_helper_signature(FILE *out, const RuntimeAbiHelperSignature *signature);

bool runtime_abi_dump_surface(FILE *out, CodegenTargetKind target) {
    size_t i;

    if (!out || target != CODEGEN_TARGET_X86_64_SYSV_ELF) {
        return false;
    }

    fprintf(out,
            "RuntimeAbi target=%s env=%s helper_args=[",
            codegen_target_name(target),
            codegen_register_name(CODEGEN_REG_R15));
    for (i = 0; i < runtime_abi_helper_arg_reg_count; i++) {
        if (i > 0 && fputs(", ", out) == EOF) {
            return false;
        }
        if (fputs(codegen_register_name(runtime_abi_helper_arg_regs[i]), out) == EOF) {
            return false;
        }
    }
    if (fputs("]\n", out) == EOF) {
        return false;
    }
    if (fputs("  entry captures=r15[value-word]\n", out) == EOF) {
        return false;
    }

    for (i = 0; i < runtime_abi_helper_count; i++) {
        if (!write_helper_signature(out, &runtime_abi_helpers[i])) {
            return false;
        }
    }

    return !ferror(out);
}

char *runtime_abi_dump_surface_to_string(CodegenTargetKind target) {
    FILE *stream;
    char *buffer;
    long size;
    size_t read_size;

    stream = tmpfile();
    if (!stream) {
        return NULL;
    }

    if (!runtime_abi_dump_surface(stream, target) || fflush(stream) != 0 ||
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

static bool write_helper_signature(FILE *out, const RuntimeAbiHelperSignature *signature) {
    size_t i;

    if (!out || !signature) {
        return false;
    }

    fprintf(out,
            "  helper %s return=%s args=[",
            signature->name,
            runtime_abi_return_kind_name(signature->return_kind));
    for (i = 0; i < signature->argument_count; i++) {
        if (i > 0 && fputs(", ", out) == EOF) {
            return false;
        }
        fprintf(out,
                "%s=%s",
                codegen_register_name(signature->arguments[i].reg),
                runtime_abi_argument_role_name(signature->arguments[i].role));
    }
    if (fputs("]", out) == EOF) {
        return false;
    }
    if (signature->pack_kind != RUNTIME_ABI_PACK_NONE) {
        fprintf(out, " pack=%s", runtime_abi_pack_kind_name(signature->pack_kind));
    }
    if (fputc('\n', out) == EOF) {
        return false;
    }

    return true;
}
